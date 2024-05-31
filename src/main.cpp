/*
MIT License | Copyright (c) 2024 Imad Laggoune

>> main.cpp
*/

#include "../deps/crow/crow_all.h"
#include "../deps/sqlite-amalgamation/sqlite3.h"

#include <iostream>
#include <string>


sqlite3* db;
char* sqlite_err = nullptr;
std::string result;

int request_sqlite_callback(void* args, int size, char** name, char** content){

result = "{";
    for (int i = 0; i < size; ++i){
        result+="\n\b";
        result+=name[i];
        result+= " : ";
        result+= content[i];
        result+=",";
    }
    result.pop_back();
    result += "\n}";

    return 0;

}


const char* url_decoder(std::string& url_encoded_string){
    int size = url_encoded_string.length();

    for (int i = 0; i < size; ++i){
        if (url_encoded_string[i] == '%'){
            if (url_encoded_string[i+1] == '2' && url_encoded_string[i+2] == '0'){
                url_encoded_string.erase(url_encoded_string.begin()+i+2);
                url_encoded_string.erase(url_encoded_string.begin()+i+1);
                url_encoded_string[i] = ' ';
                i+=3;
            }
        }
    }

    return url_encoded_string.c_str();
}

int request_sqlite(std::string sql){

    const char* sql_c = url_decoder(sql);    

    std::cout << "Requesting: " << sql_c << "\n";
    int err = sqlite3_exec(db,sql_c,request_sqlite_callback,nullptr, &sqlite_err);
    std::cout << "Done\n";
        std::cout << result << "\n";

    if (err == SQLITE_OK){
        return 0;
    } else{
        std::cout << "[SQLITE3][ERROR] " << sqlite_err << "\n";
        return 1;

    }
}




int main(){
    std::cout << "Initalizing CrowSqlite 0.1v-pre-alpha";

    if(!sqlite3_open_v2("../test.db",&db,SQLITE_OPEN_READWRITE,nullptr)){
        throw std::runtime_error("Failed to open Database File");
    }


    crow::SimpleApp crowsqlite;


    crowsqlite.port(12000);


    //crowsqlite.loglevel(crow::LogLevel::Debug);


    CROW_ROUTE(crowsqlite, "/api/<string>")([](std::string sql){        

        if(!request_sqlite(sql)){
            return (const char*)sqlite_err;
        }else{
        return result.c_str();
        }
    });

    crowsqlite.multithreaded().run();
    return 0;

        
}