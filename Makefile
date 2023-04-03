CFLAGS = -Wall -pedantic -Werror -Wextra -Wconversion -std=gnu11 -g -I$(INC_DIR)

SRC_DIR = ./sources/
INC_DIR = ./include/
OBJ_DIR = ./obj/
BIN_DIR = ./bin/

default: cliente_A

cliente_A: cliente_A.o variables_entorno.o
	$(CC) ${CFLAGS} -o $(BIN_DIR)cliente_A $(OBJ_DIR)cliente_A.o $(OBJ_DIR)variables_entorno.o

cliente_A.o: $(SRC_DIR)cliente_A.c
	$(CC) ${CFLAGS} -o $(OBJ_DIR)cliente_A.o -c $(SRC_DIR)cliente_A.c

variables_entorno.o: $(SRC_DIR)variables_entorno.c
	$(CC) ${CFLAGS} -o $(OBJ_DIR)variables_entorno.o -c $(SRC_DIR)variables_entorno.c

clean_obj:
	rm -f $(OBJ_DIR)*

clean:
	rm -f $(OBJ_DIR)* $(BIN_DIR)*