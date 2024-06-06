/*
MIT License | Copyright (c) 2024 Imad Laggoune

>> main.cpp
*/

#include "../deps/crow/crow_all.h"
#include "../deps/sqlite-amalgamation/sqlite3.h"

#include <iostream>
#include <string>


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

int request_sqlite_callback(void* args, int size, char** content, char** name)
{

    std::string* result = (std::string*)(((void**)args)[0]);
    int* index = (int*)(((void**)args)[1]);

    result->append("\n\t\"");
    result->append(std::to_string(*index));
    ++*index;
    result->append("\" : {");

    for (int i = 0; i < size; ++i) {
        result->append("\n\t\t\"");
        result->append(name[i]);
        result->append("\" : \"");
        result->append(content[i]);
        result->append("\",");
    }
    result->pop_back();
    result->append("\n\t\t},");

    return 0;
}

const char* url_decoder(std::string& url_encoded_string)
{

    int size = url_encoded_string.length();

    for (int i = 0; i < size; ++i) {
        if (url_encoded_string[i] == '%') {
            if (url_encoded_string[i + 1] == '2') {

                if (url_encoded_string[i + 2] == '0') {
                    url_encoded_string[i] = ' ';
                }

                if (url_encoded_string[i + 2] == '2') {
                    url_encoded_string[i] = '"';
                }
                url_encoded_string.erase(url_encoded_string.begin() + i + 1, url_encoded_string.begin() + i + 3);
                size -= 2;
            }
        }
    }
    return url_encoded_string.c_str();
}

int request_sqlite(sqlite3*& db, std::string sql, std::string& result, int& crow_err)
{

    const char* sql_c = url_decoder(sql);
    char* sqlite_err = nullptr;
    int err = 0;
    int index = 0;

    void* arguments[2] = { &result, &index };

    result = "{";
    err = sqlite3_exec(db, sql_c, request_sqlite_callback, &arguments, &sqlite_err);
    if (result != "{")
        result.pop_back();
    result += "\n}";

    if (err == SQLITE_OK) {
        return 0;
    } else {
        std::cout << "[SQLITE3][ERROR] " << sqlite_err << "\n";
        crow_err = 1;
        result = sqlite_err;
        return 1;
    }
}

int main(int argc, char** argv)
{

    std::string PATH; // NEED absolute path, TODO: Fix this
    int PORT = 12000;
    // argc-=1;
    for (int i = 1; i < argc; ++i) {
        std::cout << argv[i] << "\n";
        if (!strcmp(argv[i], "-d")) {
            PATH = argv[i + 1];
            ++i;
        }

        if (!strcmp(argv[i], "-p")) {
            PORT = std::__cxx11::stoi(argv[i + 1]); // Maybe we should find another solution
            ++i;
        }

        if (!strcmp(argv[i], "-h")) {
            std::cout << "CrowSqlite 0.1v\nMinimal Sqlite Server\n\nArgument List:\n\n\t "
                         "-d\t\t Absolute path to the database file\n\t -p\t\t Port Number "
                         "(Default: 12000)\n\t -h\t\t Print help text\n\n";
            return 0;
        }
    }

    std::cout << "Initalizing CrowSqlite 0.1v";
    if (PATH.empty()) {
        std::cout << "ERROR: No path for database file specified, please use "
                     "argument -d PATH.\n";
        return 1;
    }
    
    sqlite3* db;
    std::vector<sqlite3_stmt*> statements;

    if (sqlite3_open_v2(PATH.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to open Database File");
    }

    crow::SimpleApp crowsqlite;
    crowsqlite.loglevel(crow::LogLevel::Critical);
    crowsqlite.port(PORT);
    

    CROW_ROUTE(crowsqlite, "/pre/<string>")
    ([&](std::string sql) {
        sqlite3_stmt* statement;
        const char* sql_c = url_decoder(sql);
        std::string result = "";

        int err = sqlite3_prepare(db, sql_c, -1, &statement, nullptr);
        if (err != SQLITE_OK) {
            result = "Sqlite3 Error code: " + std::to_string(err);
            return result;
        } else {
            statements.push_back(statement);
            return result; // Still can't find a good return statement
        }
    });

    CROW_ROUTE(crowsqlite, "/use/<int>")
    ([&](int index) {
        int err;
        std::string result = "{";
        int index_c = 0;
        do {
            err = sqlite3_step(statements[index]);
            if (err == SQLITE_ROW) {
                int size = sqlite3_column_count(statements[index]);

                result += "\n\t\"";
                result += std::to_string(index_c);
                result += "\" : {";
                for (int i = 0; i < size; ++i) {
                    result += "\n\t\t\"";
                    result += sqlite3_column_name(statements[index], i);
                    result += "\" : \"";
                    result += (const char*)sqlite3_column_text(statements[index], i);
                    result += "\",";
                }
                result.pop_back();
                result += "\n\t\t},";
            }
            ++index_c;
        } while (err != SQLITE_DONE);
        if (result != "{")
            result.pop_back();
        result += "\n}";

        sqlite3_reset(statements[index]);
        return result;
    });

    CROW_ROUTE(crowsqlite, "/sql/<string>")
    ([&](std::string sql) {
        std::string result = "";
        int crow_err = 0;
        request_sqlite(db, sql, result, crow_err);
        if (!crow_err) {
            return result;
        } else {
            result = "";
            return result;
        }
    });
    crowsqlite.multithreaded().run();
    
    int stmt_size = statements.size();
    for (int i = 0; i < stmt_size ; ++i){
        sqlite3_finalize(statements[i]);
    }
    statements.clear();
    sqlite3_close_v2(db);

    return 0;
}