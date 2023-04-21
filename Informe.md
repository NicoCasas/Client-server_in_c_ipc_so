# Informe Lab 2: IPC Advanced - Desarrollo


## Tipos de sockets utilizados

En este laboratorio, los sockets empleados fueros los siguientes:
- Para la comunicación entre sv - cliente_A se utilizó AF_UNIX
- Para la comunicación entre sv - cliente_B se utilizó AF_INET  (IPv4)
- Para la comunicación entre sv - cliente_C se utilizó AF_INET6 (IPv6)

El establecimiento de la conexión es bastante parecido, y siguen los mismos pasos:

El servidor:
  - Crea el socket
  - Crea una direccion
  - Asocia la dirección al socket (bind)
  - Pone al socket en modo de escucha (listen)
  - Acepta conexiones

El cliente:
  - Crea el socket
  - Genera la dirección del servidor
  - Se conecta con el servidor con el método connect

Una vez aceptado el cliente por el servidor, la comunicación es full duplex.

## Gestión de conexiones por parte del servidor

Para la gestión de las conexiones el servidor implementa 3 listas, una para cada tipo de cliente donde se almacena los file descriptors obtenidos al momento de aceptar la conexion.

Además, el servidor hace uso de la función epoll. Epoll es una función que espera y retorna cuando en alguno de los file descriptor a los que escucha se genera un evento.
En este caso, contiene los sfd de los listeners (los sfd que están en escucha y aceptan conexiones) y los de los clientes.

Luego, hay dos escenarios posibles:
  - El sfd que retorna epoll es un listener -> Se acepta la conexion y se agrega el nuevo sfd obtenido a epoll y a la lista de clientes correspondiente.
  - El sfd que retorna epoll no es un listener -> Se busca en las listas a qué tipo de cliente corresponde, y se actúa en consecuencia. 

La maxima cantidad de clientes soportados al mismo tiempo es de 65536 clientes y se hace uso de setrlimit para setear este valor. No se puede usar valgrind con esta consideración (ver apartado Cppcheck y Valgrind).

## Comunicaciones

Para mantener homogeneidad en cuanto al paso de mensajes entre los mensajes en formato string y en formato binario, se emplea una interfaz que encapsula a los datos. Trabaja todo como datos en binario.

La misma tiene una estructura que consta de:
 - 4 bytes para indicar el tipo de mensajes. 
 - 4 bytes de orden.
 - 4 bytes para indicar el largo de la informacion.
 - los datos.
 - checksum, que se calcula sobre todo lo anterior.

Dicha interfaz fue escrita por mí para otro trabajo práctico. Por eso es que el campo tipo no tiene mucha importancia en este caso particular, ya que el origen del mensaje se encuentra en el json (se habla más adelante de esto). Hay métodos nuevos creados para hacer lo posible de pensarlo como una capa de abstracción. Por ejemplo los métodos send_data_msg y receive_data_msg permiten enviar y recibir información independientemente de su tamaño, ya que loparten y reconstruyen en origen y destino. Con el uso de esas dos funciones, se puede abstraer de esta capa. Es la que implementa la validación de checksum. Usa sha-256, de la openssl/evp, se puede cambiar el tipo de checksum modificando 2 macros de checksum.h. 

## JSON

Para los mensajes, ya información que va a usar el programa, se emplea el formato json como indica la consigna.
Para los clientes, los mensajes se ordenan, a priori, con el siguiente formato:

    {
      "origen" : "cliente_x",
      "n_requests" : n,
      "pid" : pid,
      "requests" : [
                    "request_1" : "'request'",
                    ...
                   ]
     }

Para el servidor:

    {
      "origen" : "servidor",
      "n_responds" : n,
      "responds" : [
                    "respond_1" : "'respond'",
                    ...
                   ]
     }
     
Luego, los clientes A y B tienen un solo request, el comando journalctl a ejecutar, y los clientes C tienen 2, uno para mem_free y otro para norm_load_avg.

La idea de implementarlo así, es que siento que así sería más cómodo escalarlo si fuera necesario responder a más request por cliente o a otros tipos de requests.

En cJSON_custom, hay métodos que creé para facilitar el manejo de json, algunos más generales y otros más específicos del tp. Esto a su vez, es factible debido a la homogeneidad entre los formatos de los mensajes.

## Compresion

Para la compresión se usa la librería zlib.h. Los códigos principales referidos a este tema se incluyen en compress_and_decompress_file.c. Funciona como una interfaz. Está extraído y modificado de internet (se aclara la página como comentario en el código). Tuve que hacer un método extra para comprimir no de un archivo, si no de una variable. Más que hacer una función extra, en realidad es un wrapper de una función de la librería. Los clientes_B guardan el comprimido en la carpeta compress_files usando un nombre al azar que utiliza la función mkstemps.

## Journalctl

Aprovechando que xubuntu (la distribución que uso) tiene esta funcionalidad, lo que hago en el sv es hacer fork-exec redireccionando el std a un pipe que voy a leer desde proceso padre en paralelo, guardando el contenido en una variable alocada dinámicamente. Podría haber redirigido todo a un archivo y leerlo de ahí, pero me pareció más eficiente tener todo en una variable, usarla y liberarla, que escribir un archivo, leerlo y borrarlo. El tamaño máximo que se optó por ponerle a un retorno de esta función es de 256 MB para el caso de pasar el contenido tal cual (cliente A) y de 1 GB en caso de que vaya a ser comprimido (cliente_B). 

## Cppcheck y Valgrind

Los códigos no tienen errores de cppcheck ni valgrind. No está probado valgrind para contenidos de journalctl mayores a 2 MB.

Para usar con valgrind, hay que setear una variable de entorno como "VALGRIND=YES" ya sea en un archivo de configuracion o antes del programa. e.g.

    VALGRIND=YES valgrind ./Servidor

Esto se debe a que valgrind no se lleva bien con setrlimit (metodo usado para soportar más de 1024 conexiones), por lo que, de usar esta opcion, se deja el máximo por default (1024 file descriptors) pero se puede controlar con valgrind el manejo de memoria.

## Aviso ante error del servidor

Para esto, se usa una funcion at_exit que envia un mensaje de error recorriendo las listas de conexiones. Luego se cierran los sfd correspondientes y se eliminan las listas antes de terminar el programa.

## Variables de entorno

Se usa la misma logica empleada en el tp anterior

## Tests implementados

En la carpeta test se encuentran 4 scripts de bash que sirven de tests. Constan de lo siguiente:

- test_cliente_A.sh -> Lo que hace este test es levantar el servidor, levantar un cliente_A, pasarle un comando de journalctl, recibir la respuesta y compararla con la ejecucion del comando en una bash.
- test_cliente_B.sh -> Lo que hace este test es levantar el servidor, levantar un cliente_B, pasarle un comando de journalctl, recibir la respuesta comprimida, y por bash descomprimir el archivo para posteriormente compararlo con la ejecucion del comando.
- test_cliente_C.sh -> Mismo procedimiento que los anteriores, pero compara las respuestas dadas por el servidor con los valores suministrados por /proc/loadavg y /proc/meminfo. Cabe destacar que si bien para la memoria libre se considera un error, este test puede fallar si entre el pedido al servidor y la comprobacion contra el /proc, los valores varian un poco más de lo considerado.
- test_multiples_clientes -> Levanta N_A clientes_A, N_B clientes_B y N_C clientes_C. Luego, a través del parseo de la salida del servidor, se verifica que la cantidad de mensajes provenientes de clientes unicos (via pid) sea igual a la suma de N_A+N_B+N_C. Además, se controla que la cantidad de archivos comprimidos sea igual a N_B.
