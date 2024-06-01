/*
MIT License | Copyright (c) 2024 Imad Laggoune

>> main.cpp
*/

#include "../deps/crow/crow_all.h"
#include "../deps/sqlite-amalgamation/sqlite3.h"

#include <iostream>
#include <string>


sqlite3* db;
//char* sqlite_err = nullptr;
//std::string result;

int request_sqlite_callback(void* args, int size, char** content, char** name){

std::cout << size << " Size\n";
std::string* result = (std::string*)args;

result->append("{");
    for (int i = 0; i < size; ++i){
        result->append("\n\t\"");
        result->append(name[i]);
        result->append( "\" : \"");
        result->append( content[i]);
        result->append("\",");
    }
    result->pop_back();
    result ->append( "\n}");

    return 0;

}




const char* url_decoder(std::string& url_encoded_string){

    size_t p_position = url_encoded_string.find('%');
    if (p_position > url_encoded_string.size()){
        return url_encoded_string.c_str();
    }

    while(p_position < url_encoded_string.length()){
        std::cout << p_position << "\n";
        if (url_encoded_string[p_position+1] == '2'){
                if (url_encoded_string[p_position+2] == '0'){
                    url_encoded_string[p_position] = ' ';
                }

                if (url_encoded_string[p_position+2] == '2'){
                    url_encoded_string[p_position] = '"';
                }
                url_encoded_string.erase(url_encoded_string.begin()+p_position+1,url_encoded_string.begin()+p_position+3);
                p_position = url_encoded_string.find('%');
            }
    }

    return url_encoded_string.c_str();
    // TODO: Compare both in terms of performance.
    int size = url_encoded_string.length();

    for (int i = 0; i < size; ++i){

        if (url_encoded_string[i] == '%'){
            if (url_encoded_string[i+1] == '2'){

                if (url_encoded_string[i+1] == '0'){
                    url_encoded_string[i] = ' ';
                }

                if (url_encoded_string[i+1] == '2'){
                    url_encoded_string[i] = '"';
                }
                url_encoded_string.erase(url_encoded_string.begin()+i+1,url_encoded_string.begin()+i+2);
                size-= 2;


            }
        }
    }

    return url_encoded_string.c_str();
}

int request_sqlite(std::string sql, std::string &result, int &crow_err){

    const char* sql_c = url_decoder(sql);    
    char* sqlite_err = nullptr;
    
    std::cout << "Requesting: " << sql_c << "\n";
    int err = sqlite3_exec(db,sql_c,request_sqlite_callback,&result, &sqlite_err);
    std::cout << "Done\n";
        std::cout << result << "\n";
        

    if (err == SQLITE_OK){
        return 0;
    } else{
        std::cout << "[SQLITE3][ERROR] " << sqlite_err << "\n";
        crow_err = 1;
        result = sqlite_err;
        return 1;

    }
}




int main(){
    std::cout << "Initalizing CrowSqlite 0.1v-pre-alpha";

    if(sqlite3_open_v2("C:/Users/Work/Documents/CrowSqlite/test.db",&db,SQLITE_OPEN_READWRITE,nullptr) != SQLITE_OK){
        std::cout << sqlite3_open_v2("C:/Users/Work/Documents/CrowSqlite/test.db",&db,SQLITE_OPEN_READWRITE,nullptr);
        throw std::runtime_error("Failed to open Database File");
    }


    crow::SimpleApp crowsqlite;


    crowsqlite.port(12000);


    //crowsqlite.loglevel(crow::LogLevel::Debug);


    CROW_ROUTE(crowsqlite, "/api/<string>")([](std::string sql){        
        std::string result = "";
        int crow_err = 0;
        request_sqlite(sql, result, crow_err);
        if(!crow_err){
            std::cout << result.c_str() << "Done here";

            return result;
        }else{
            std::cout << result.c_str() << "Done here";
        return result;
        }
    });

    crowsqlite.multithreaded().run();
    return 0;

        
}