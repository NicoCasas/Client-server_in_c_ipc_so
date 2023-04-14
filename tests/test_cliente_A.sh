#!/bin/bash

COMANDO="journalctl -u ondemand.service"
TEST_DIR=./test_A
DELETE_LOG_FLAG=1   #Si esta en 1, borra los logs al final de la ejecucion

# Ponemos logs a cero
mkdir ${TEST_DIR} 2>/dev/null
rm -f ${TEST_DIR}/*.log
rm -f ${TEST_DIR}/*.txt

# Ponemos el comando en el file que va a leer el cliente_A como input
echo -e "${COMANDO}\n" > ${TEST_DIR}/comandos_cliente_A.txt

# Iniciar el servidor
../build/Servidor > "${TEST_DIR}/servidor.log" 2>&1 & SV_PID=$!

sleep 0.5

# Iniciamos el cliente_A
../build/Cliente_A  < ${TEST_DIR}/comandos_cliente_A.txt > "${TEST_DIR}/cliente_A.log" 2>/dev/null & CL_A_PID=$!

# Damos tiempo a que termine la ejecucion
sleep 1

# Terminamos los procesos
kill -SIGINT ${CL_A_PID} 2>/dev/null
kill -SIGINT ${SV_PID} 2>/dev/null

# Limpiamos para tener solo el output del comando
sed 's/Cliente_A: //; /^[A-Z]/ d; /^$/ d' ${TEST_DIR}/cliente_A.log > ${TEST_DIR}/sv_result.txt
${COMANDO} > ${TEST_DIR}/journal_result.txt

# Comparamos los resultados
diff ${TEST_DIR}/sv_result.txt ${TEST_DIR}/journal_result.txt

# Imprimimos el resultado
[ $(echo $?) == 0 ] && echo "CLIENTE_A : TEST PASSED" || echo "CLIENTE_A : TEST FAILED"

# Borramos los logs y txts
[ ${DELETE_LOG_FLAG} == 1 ] && (rm ${TEST_DIR}/*.txt ; rm ${TEST_DIR}/*.log ; rm -r ${TEST_DIR})