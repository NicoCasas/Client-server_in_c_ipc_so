# Se declara la version minima
cmake_minimum_required(VERSION 3.22)

# Setup del proyecto
project(
		Tp2
		VERSION 1.0.0
		DESCRIPTION "Tp2_IPC_Advanced"
		LANGUAGES C
)

# Ponemos version de C y flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra -Werror -Wconversion -std=gnu11")

# Agregamos ejecutables
add_executable(Servidor sources/servidor.c sources/variables_entorno.c sources/checksum.c sources/cJSON.c)
add_executable(Cliente_A sources/cliente_A.c sources/variables_entorno.c sources/checksum.c sources/cJSON.c)
add_executable(Cliente_B sources/cliente_B.c sources/variables_entorno.c sources/checksum.c sources/cJSON.c)
add_executable(Cliente_C sources/cliente_C.c sources/variables_entorno.c sources/checksum.c sources/cJSON.c)


# Headers
target_include_directories(Servidor PRIVATE "include/")
target_include_directories(Cliente_A PRIVATE "include/")
target_include_directories(Cliente_B PRIVATE "include/")
target_include_directories(Cliente_C PRIVATE "include/")

# Linkeamos librerias
target_link_libraries(Servidor -lcrypto)
target_link_libraries(Cliente_A -lcrypto)
target_link_libraries(Cliente_B -lcrypto)
target_link_libraries(Cliente_C -lcrypto)
