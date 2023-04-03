#define CONFIG_FILE_PATH_DEFAULT                "../config/ipc.conf"

/*ENV NAMES OF PATHS*/
#define FIFO_PATH_ENV_NAME                               "FIFO_PATH"
#define MSGQ_PATH_ENV_NAME                               "MSGQ_PATH"
#define SHM_PATH_ENV_NAME                                 "SHM_PATH"
#define LOG_PATH_ENV_NAME                                 "LOG_PATH"

/*ENV NAMES OF SEMAPHORES FILES*/
#define SEM_LECTOR_F_ENV_NAME                     "SEM_LECTOR_FNAME"
#define SEM_ESCRITOR_F_ENV_NAME                 "SEM_ESCRITOR_FNAME"

/*DEFAULT PATHS*/
#define FIFO_PATH_DEFAULT                                   "./fifo"
#define MSGQ_PATH_DEFAULT                                   "./msgq"
#define SHM_PATH_DEFAULT                                     "./shm"
#define LOG_PATH_DEFAULT                       "../log/mensajes.log"

/*DEFAULT SEMAPHORES FILES*/
#define SEM_LECTOR_FNAME_DEFAULT                     "/lector_sv_cl"
#define SEM_ESCRITOR_FNAME_DEFAULT                 "/escritor_cl_sv"

/*FUNCTIONS DEFINITION*/
void comprobar_variables_entorno                        ();
void comprobar_variables_entorno_A                      ();
void comprobar_variables_entorno_B                      ();
void comprobar_variables_entorno_C                      ();
void comprobar_variables_entorno_log                    ();
void cargar_variables_de_entorno_de_archivo             (const char* path);