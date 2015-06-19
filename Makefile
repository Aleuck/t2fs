CC=gcc
LIB_DIR=./lib
BIN_DIR=./bin
SRC_DIR=./src

all: main

$(BIN_DIR)/main: $(LIB_DIR)/libt2fs.a
	$(CC) -o $(BIN_DIR)/main $(SRC_DIR)/main.c $(LIB_DIR)/libt2fs.a

$(LIB_DIR)/libt2fs.a: $(SRC_DIR)/t2fs.o
	ar crs $(LIB_DIR)/libt2fs.a $(SRC_DIR)/t2fs.o $(SRC_DIR)/apidisk.o

$(SRC_DIR)/t2fs.o:
	$(CC) -o $(SRC_DIR)/t2fs.o -c $(SRC_DIR)/t2fs.c

clean:
	rm $(LIB_DIR)/*.a $(SRC_DIR)/t2fs.o
