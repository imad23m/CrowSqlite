/*
MIT License | Copyright (c) 2024 Imad Laggoune

>> main.cpp
*/

#include "../deps/crow/crow_all.h"
#include "../deps/sqlite-amalgamation/sqlite3.h"

#define CSQL_CONSOLE_PRINT // Enables Console logging | useful if you don't care about console logs
#define CSQL_HTTP_LOGGER // Enables the view of logs using /logs url
#define CSQL_EXEC_SQL // Enables execution of SQL queries using /sql
#define CSQL_PREPARE_SQL // Enables Preparing and using Sqlite3 statements

#define RESPONSE_HEADERS(res)                               \
    {                                                       \
        res.set_header("Access-Control-Allow-Origin", "*"); \
    }

#ifdef CSQL_CONSOLE_PRINT

#include <iostream>
#define log_print(message)                 \
    {                                      \
        std::cout << message << std::endl; \
    }
#elif
#define log_print(message) \
    {                      \
    }
#endif

#include <string>

class http_logger : public crow::ILogHandler {
public:
    static inline std::string logs;
    void log(std::string message, crow::LogLevel level)
    {
        log_print(message);
        logs += message;
        logs += "\n";
    }
};

#ifdef CSQL_HTTP_LOGGER
#define http_logger_f()                 \
    {                                   \
        http_logger l;                  \
        crow::logger::setHandler(&l);   \
        CROW_ROUTE(crowsqlite, "/logs") \
        ([&]() { return l.logs; });     \
    }
#elif
#define http_logger_f() \
    {                   \
    }
#endif

#ifdef CSQL_EXEC_SQL
#define sql()                                   \
    {                                           \
        CROW_ROUTE(crowsqlite, "/sql/<string>") \
        ([&](std::string sql) {std::string result = "";int crow_err = 0;request_sqlite(db, sql, result, crow_err);  \
        if (crow_err) {result = "";} crow::response res; RESPONSE_HEADERS(res); res.write(result); return res; });             \
    }
#elif
#define sql() \
    {         \
    }
#endif

#ifdef CSQL_PREPARE_SQL
#define prepare()                               \
    {                                           \
        CROW_ROUTE(crowsqlite, "/pre/<string>") \
        ([&](std::string sql) {sqlite3_stmt* statement;const char* sql_c = url_decoder(sql);std::string result = "";int err = sqlite3_prepare(db, sql_c, -1, &statement, nullptr); crow::response res; RESPONSE_HEADERS(res);if (err != SQLITE_OK) {result = "Sqlite3 Error code: " + std::to_string(err); res.write(result); return res;} else {statements.push_back(statement); res.write(result); return res; }});             \
    }
#define use()                                \
    {                                        \
        CROW_ROUTE(crowsqlite, "/use/<int>") \
        ([&](int index) {int err;std::string result = "{";int index_c = 0;do {err = sqlite3_step(statements[index]);if (err == SQLITE_ROW) {int size = sqlite3_column_count(statements[index]);result += "\n\t\"";result += std::to_string(index_c);result += "\" : {";for (int i = 0; i < size; ++i) {result += "\n\t\t\"";result += sqlite3_column_name(statements[index], i);result += "\" : \"";result += (const char*)sqlite3_column_text(statements[index], i);result += "\",";}result.pop_back();result += "\n\t\t},";}++index_c;} while (err != SQLITE_DONE);if (result != "{")result.pop_back();result += "\n}";sqlite3_reset(statements[index]);return result; });                \
    }
#define clean()                           \
    {                                     \
        CROW_ROUTE(crowsqlite, "/clean/") \
        ([&]() {crow::response res; RESPONSE_HEADERS(res); int stmt_size = statements.size(); if(stmt_size == 0) return res; for (int i = 0; i < stmt_size; ++i) {sqlite3_finalize(statements[i]);}statements.clear();return res; });                      \
    }
#elif
#define prepare() \
    {             \
    }
#define use() \
    {         \
    }
#define clean() \
    {           \
    }
#endif

/*
int request_sqlite_callback(void* args, int size, char** content, char** name){

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
        std::string error = "[SQLITE3][ERROR] ";
        error += sqlite_err;
        log_print(error.c_str());

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
        if (!strcmp(argv[i], "-d")) {
            PATH = argv[i + 1];
            ++i;
        }

        if (!strcmp(argv[i], "-p")) {
            PORT = std::__cxx11::stoi(argv[i + 1]); // Maybe we should find another solution
            ++i;
        }

        if (!strcmp(argv[i], "-h")) {
            //  std::cout << "CrowSqlite 0.3v\nMinimal Sqlite Server\n\nArgument List:\n\n\t "
            //             "-d\t\t Absolute path to the database file\n\t -p\t\t Port Number "
            //           "(Default: 12000)\n\t -h\t\t Print help text\n" << std::endl;
            log_print("CrowSqlite 0.3v\nMinimal Sqlite Server\n\nArgument List:\n\n\t -d\t\t Absolute path to the database file\n\t -p\t\t Port Number (Default: 12000)\n\t -h\t\t Print help text\n");
            return 0;
        }
    }
    log_print("Initalizing CrowSqlite 0.3v");
    // std::cout << "Initalizing CrowSqlite 0.3v" << std::endl;

    if (PATH.empty()) {
        log_print("ERROR: No path for database file specified, please use argument -d PATH.");
        // std::cout << "ERROR: No path for database file specified, please use argument -d PATH." << std::endl;
        return 1;
    }

    sqlite3* db;
    std::vector<sqlite3_stmt*> statements;

    if (sqlite3_open_v2(PATH.c_str(), &db, SQLITE_OPEN_READWRITE, nullptr) != SQLITE_OK) {
        log_print("Failed to open Database File");
        //  throw std::runtime_error("Failed to open Database File");
        return 1;
    }

    crow::SimpleApp crowsqlite;

    prepare();
    use();
    clean();

    sql();
    http_logger_f();

    crowsqlite
        .loglevel(crow::LogLevel::Info)
        .port(PORT)
        .server_name("CrowSqlite")
        .multithreaded()
        .run();

    int stmt_size = statements.size();
    for (int i = 0; i < stmt_size; ++i) {
        sqlite3_finalize(statements[i]);
    }
    statements.clear();
    sqlite3_close_v2(db);

    return 0;
}