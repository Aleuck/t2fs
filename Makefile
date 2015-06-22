
CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src

all: $(BIN_DIR)/main

$(BIN_DIR)/main: $(LIB_DIR)/libt2fs.a
	$(CC) -m32 -o $(BIN_DIR)/main $(SRC_DIR)/main.c -L$(LIB_DIR)/ -lt2fs -Wall

$(LIB_DIR)/libt2fs.a: $(LIB_DIR)/t2fs.o $(LIB_DIR)/bitmap_operations.o $(LIB_DIR)/block_io.o $(LIB_DIR)/filepath_operations.o
	ar crs $(LIB_DIR)/libt2fs.a $(LIB_DIR)/*.o

$(LIB_DIR)/t2fs.o:
	$(CC) -m32 -o $(LIB_DIR)/t2fs.o -c $(SRC_DIR)/t2fs.c -Wall

$(LIB_DIR)/bitmap_operations.o: 
	$(CC) -m32 -o $(LIB_DIR)/bitmap_operations.o -c $(SRC_DIR)/bitmap_operations.c -Wall

$(LIB_DIR)/block_io.o: 
	$(CC) -m32 -o $(LIB_DIR)/block_io.o -c $(SRC_DIR)/block_io.c -Wall

$(LIB_DIR)/filepath_operations.o: 
	$(CC) -m32 -o $(LIB_DIR)/filepath_operations.o -c $(SRC_DIR)/filepath_operations.c -Wall

clean:
	rm $(BIN_DIR)/* $(LIB_DIR)/*.a $(LIB_DIR)/t2fs.o $(LIB_DIR)/block_io.o $(LIB_DIR)/bitmap_operations.o $(LIB_DIR)/filepath_operations.o