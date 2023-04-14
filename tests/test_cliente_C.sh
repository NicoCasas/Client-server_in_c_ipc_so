#!/bin/bash

COMANDO="journalctl -u ondemand.service"
TEST_DIR=./test_C
DELETE_LOG_FLAG=1   #Si esta en 1, borra los logs al final de la ejecucion

# Ponemos logs a cero
mkdir ${TEST_DIR} 2>/dev/null
rm -f ${TEST_DIR}/*.log
rm -f ${TEST_DIR}/*.txt

# Iniciar el servidor
../build/Servidor > "${TEST_DIR}/servidor.log" 2>&1 & SV_PID=$!

sleep 0.5

# Iniciamos el cliente_C
../build/Cliente_C > "${TEST_DIR}/cliente_C.log" 2>&1 & CL_C_PID=$!

# Calculamos valores reales
MEMORIA_LIBRE=$(cat /proc/meminfo | grep MemFree | awk '{print $2}')
CARGA=$(cat /proc/loadavg | awk '{print $3}')
CARGA_NORMALIZADA=$(echo "0$(echo "scale=6 ; ${CARGA}/4" | bc )")

# Damos tiempo a que termine la ejecucion
sleep 1

# Terminamos los procesos
kill -SIGINT ${CL_C_PID} 2>/dev/null
kill -SIGINT ${SV_PID} 2>/dev/null

# Limpiamos para tener solo el output del comando
CL_MEMORIA_LIBRE=$(cat test_C/cliente_C.log | grep Memoria | tail -n 1 | awk '{print $3}')
CL_CARGA_NORMALIZADA=$(cat test_C/cliente_C.log | grep Carga | tail -n 1 | awk '{print $3}')

# Comparamos los resultados
#echo "carga_n=${CARGA_NORMALIZADA}"
#echo "carga_cl_n=${CL_CARGA_NORMALIZADA}"

#echo "mem=${MEMORIA_LIBRE}"
#echo "mem=${CL_MEMORIA_LIBRE}"

MARGEN_ERROR=1000
ERROR=$(( ${CL_MEMORIA_LIBRE} - ${MEMORIA_LIBRE} )) 

[ ${ERROR} > 0 ] || ERROR=-${ERROR}

# Imprimimos el resultado
[ ${CARGA_NORMALIZADA} == ${CL_CARGA_NORMALIZADA} ] && [ ${ERROR} -le ${MARGEN_ERROR} ] && echo "CLIENTE_C : TEST PASSED" || echo "CLIENTE_C : TEST FAILED"

# Borramos los logs y txts
[ ${DELETE_LOG_FLAG} == 1 ] && (rm -f ${TEST_DIR}/*.txt ; rm -f ${TEST_DIR}/*.log ; rm -r ${TEST_DIR})