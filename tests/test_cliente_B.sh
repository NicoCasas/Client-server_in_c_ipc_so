#!/bin/bash

COMANDO="journalctl -u ondemand.service"
TEST_DIR=./test_B
COMPRESS_PATH=${TEST_DIR}/compress_files/

DELETE_LOG_FLAG=1   #Si esta en 1, borra los logs al final de la ejecucion

# Ponemos logs a cero
mkdir ${TEST_DIR} 2>/dev/null
rm -f ${TEST_DIR}/*.log
rm -f ${TEST_DIR}/*.txt
rm -f ${COMPRESS_PATH}/*

# Ponemos el comando en el file que va a leer el cliente_B como input
echo -e "${COMANDO}\n" > ${TEST_DIR}/comandos_cliente_B.txt

# Iniciar el servidor
../build/Servidor > "${TEST_DIR}/servidor.log" 2>&1 & SV_PID=$!

sleep 0.5

# Iniciamos el cliente_B
export COMPRESS_PATH
../build/Cliente_B  < ${TEST_DIR}/comandos_cliente_B.txt > "${TEST_DIR}/cliente_B.log" 2<&1 & CL_B_PID=$!

# Damos tiempo a que termine la ejecucion
sleep 1

# Terminamos los procesos
kill -SIGINT ${CL_B_PID} 2>/dev/null
kill -SIGINT ${SV_PID} 2>/dev/null

# Limpiamos para tener solo el output del comando
gunzip ${COMPRESS_PATH}*.gz
${COMANDO} > ${TEST_DIR}/journal_result.txt

# Comparamos los resultados
diff ${COMPRESS_PATH}* ${TEST_DIR}/journal_result.txt

# Imprimimos el resultado
[ $(echo $?) == 0 ] && echo "CLIENTE_B : TEST PASSED" || echo "CLIENTE_B : TEST FAILED"

# Borramos los logs y txts
[ ${DELETE_LOG_FLAG} == 1 ] && (rm ${TEST_DIR}/*.txt ; rm ${TEST_DIR}/*.log ; rm -r ${TEST_DIR};)