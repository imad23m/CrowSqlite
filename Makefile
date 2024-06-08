

CXX=g++
CC=gcc
OUTPUT=./build/crowsqlite.exe

CC_CXX_FLAGS= -O3 -std=c++23 # -march=native

SQLITE3_LIB=deps/sqlite-amalgamation/sqlite.lib
SQLITE3_LIB_NAME=sqlite
SQLITE3_LIB_DIR=deps/sqlite-amalgamation/

all: $(OUTPUT)

$(OUTPUT): $(SQLITE3_LIB)
	$(CXX) $(CC_CXX_FLAGS) -Ideps/asio/include src/main.cpp -o $(OUTPUT) -L$(SQLITE3_LIB_DIR) -l$(SQLITE3_LIB_NAME) -lws2_32 -lwsock32



$(SQLITE3_LIB):
	$(CC) $(CC_CXX_FLAGS) -c deps/sqlite-amalgamation/sqlite3.c -o $(SQLITE3_LIB)


clean:
	rm -f $(OUTPUT)