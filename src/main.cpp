/*
MIT License | Copyright (c) 2024 Imad Laggoune

>> main.cpp
*/

#include "../deps/crow/crow_all.h"
#include "../deps/sqlite-amalgamation/sqlite3.h"

#include <iostream>
#include <string>


//char* sqlite_err = nullptr;
//std::string result;

/*
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
}*/


int index = 0; // TODO: make it function varaible, probably std::pair?

int request_sqlite_callback(void* args, int size, char** content, char** name){

std::string* result = (std::string*)args;
    result->append("\n\t\"");
    result->append(std::to_string(index));
    ++index;
    result->append("\" : {");

    for (int i = 0; i < size; ++i){
        result->append("\n\t\t\"");
        result->append(name[i]);
        result->append( "\" : \"");
        result->append( content[i]);
        result->append("\",");
    }
    result->pop_back();
    result ->append( "\n\t\t},"); 

    return 0;
}




const char* url_decoder(std::string& url_encoded_string){
    
    
     ////////// This method is VERY slow compare to the one used now ///////////
    /*std::chrono::time_point t= std::chrono::high_resolution_clock::now();
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
        std::chrono::time_point e= std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration((e-t)) << " Time";
    return url_encoded_string.c_str();
*/
//////////////////////////////////////////////////////////////////////

// Retest this:
    int size = url_encoded_string.length();

    for (int i = 0; i < size; ++i){

        if (url_encoded_string[i] == '%'){
            if (url_encoded_string[i+1] == '2'){

                if (url_encoded_string[i+2] == '0'){
                    url_encoded_string[i] = ' ';
                }

                if (url_encoded_string[i+2] == '2'){
                    url_encoded_string[i] = '"';
                }
                url_encoded_string.erase(url_encoded_string.begin()+i+1,url_encoded_string.begin()+i+3);
                size-= 2;
            }
        }
    }

    return url_encoded_string.c_str();
}

int request_sqlite(sqlite3* &db, std::string sql, std::string &result, int &crow_err){
    index = 0;

    const char* sql_c = url_decoder(sql);    
    char* sqlite_err = nullptr;
    int err = 0;
    if(true){
        result = "{";
        err = sqlite3_exec(db,sql_c,request_sqlite_callback,&result, &sqlite_err);
        if (result != "{")
             result.pop_back();
        result += "\n}";
        }
    else
        err = sqlite3_exec(db,sql_c,request_sqlite_callback,&result, &sqlite_err);        

    if (err == SQLITE_OK){
        return 0;
    } else{
        std::cout << "[SQLITE3][ERROR] " << sqlite_err << "\n";
        crow_err = 1;
        result = sqlite_err;
        return 1;

    }
}




int main(int argc, char** argv){

    std::string PATH; // NEED absolute path, TODO: Fix this
    argc-=1;
    for (int i = 1 ; i < argc ; ++i){
        if (!strcmp(argv[i],"-p")){
            PATH = argv[i+1];
            ++i;
        }
    }

    std::cout << "Initalizing CrowSqlite 0.1v-alpha";
    sqlite3* db;

    if(sqlite3_open_v2(PATH.c_str(),&db,SQLITE_OPEN_READWRITE,nullptr) != SQLITE_OK){
        throw std::runtime_error("Failed to open Database File");
    }


    crow::SimpleApp crowsqlite;


    crowsqlite.port(12000);


    //crowsqlite.loglevel(crow::LogLevel::Debug);


    CROW_ROUTE(crowsqlite, "/api/<string>")([&](std::string sql){        
        std::string result = "";
        int crow_err = 0;
        request_sqlite(db, sql, result, crow_err);        
        if(!crow_err){
            return result;
        }else{
            result = "";
            return result; // Should we send the message or not?
        }
    });

    crowsqlite.multithreaded().run();
    return 0;

        
}