#!/bin/bash

# Cantidad de clientes a crear
N_A=20
N_B=30
N_C=1000

# Definimos si vamos a borrar los logs al final de la ejecucion
BORRAR_LOGS=1

# Definimos los comandos de los clientes A y B
COMANDO_A="journalctl -u ondemand.service"
COMANDO_B="journalctl -u ondemand.service"


# Definimos carpetas para tests
TEST_DIR="./test_multiples_clientes"
COMPRESS_PATH="${TEST_DIR}/compress_files/"
echo "COMPRESS_PATH: ${COMPRESS_PATH}"
export COMPRESS_PATH

# Definimos el tiempo de ejecucion
TIEMPO_EJECUCION=2

# Ponemos los log a 0
rm -f ../log/mensajes.log
touch ../log/mensajes.log
mkdir ${TEST_DIR} 2>/dev/null
rm -f ${TEST_DIR}/*.log
mkdir ${COMPRESS_PATH} 2>/dev/null
rm -f ${COMPRESS_PATH}*

# Ponemos el comando en el file que van a leer los clientes A y B como input
echo -e "${COMANDO_A}\n" > ${TEST_DIR}/comandos_cliente_A.txt
echo -e "${COMANDO_B}\n" > ${TEST_DIR}/comandos_cliente_B.txt

# Iniciamos el sv
#../build/Servidor > /dev/null & SERV_PID=$!
../build/Servidor > ${TEST_DIR}/servidor.log 2>${TEST_DIR}/servidor_err.log & SERV_PID=$!

sleep 1

# Iniciamos los cl
for ((i = 0 ; i < N_A ; i++)) do
	../build/Cliente_A < ${TEST_DIR}/comandos_cliente_A.txt >/dev/null 2>&1 & CL_PID[$i]=$! 
done

for ((i = 0 ; i < N_B ; i++)) do
	../build/Cliente_B < ${TEST_DIR}/comandos_cliente_B.txt >/dev/null 2>&1 & CL_PID[(($i+N_A))]=$! 
done

for ((i = 0 ; i < N_C ; i++)) do
	../build/Cliente_C >/dev/null 2>&1 & CL_PID[(($i+N_A+N_B))]=$!
done

#>/dev/null 2>&1
#> "${TEST_DIR}/cliente_C_${i}.log"

# Esperamos
sleep ${TIEMPO_EJECUCION}

# Terminamos los procesos
for ((i = 0 ; i < $((N_A+N_B+N_C)) ; i++)) do
	kill -SIGINT ${CL_PID[$i]} 2>/dev/null
done
kill -SIGINT $SERV_PID 2>/dev/null

N_PROCESOS_CONECTADOS=$(grep pid ${TEST_DIR}/servidor.log | sort | uniq | wc | awk '{print $1}')
#echo "N_PROCESOS_CONECTADOS: ${N_PROCESOS_CONECTADOS}"

N_ARCHIVOS_COMPRIMIDOS=$(ls ${COMPRESS_PATH} | wc | awk '{print $1}')
#echo "N_ARCHIVOS_COMPRIMIDOS: ${N_ARCHIVOS_COMPRIMIDOS}"

[[ ${N_PROCESOS_CONECTADOS} == $((${N_A}+${N_B}+${N_C})) ]] && [[ ${N_ARCHIVOS_COMPRIMIDOS} == ${N_B} ]] && echo "MULTIPLES CLIENTES: TEST_PASSED" || echo "MULTIPLES CLIENTES: TEST FAILED"
[ ${BORRAR_LOGS} == 1 ] && (rm ${TEST_DIR}/*.log; rm ${TEST_DIR/*.txt}; rm -rf ${TEST_DIR}/ 2>/dev/null)

