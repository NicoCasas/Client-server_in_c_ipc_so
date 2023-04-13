#define S_MAX 64
#define N_BYTES_TO_SEND                         512
#define N_BYTES_TO_RECEIVE                      512

#define DIGEST_NAME                             "sha256"
#define DIGEST_SIZE                             32

#define MSG_ADITIONAL_INFO_SIZE                 (3*sizeof(unsigned int))
#define MSG_OVERHEAD_SIZE                       (MSG_ADITIONAL_INFO_SIZE + DIGEST_SIZE)
#define MSG_DATA_SIZE                           (N_BYTES_TO_SEND - MSG_OVERHEAD_SIZE)

#define TYPE_LOG                 (unsigned int)               0
#define TYPE_PRODUCTOR_1         (unsigned int)               1
#define TYPE_PRODUCTOR_2         (unsigned int)               2
#define TYPE_PRODUCTOR_3         (unsigned int)               3
#define TYPE_VALIDACION_1        (unsigned int)     ((1<<28)+1)
#define TYPE_VALIDACION_2        (unsigned int)     ((1<<28)+2)

#define POS_TYPE                 (unsigned int)                                          0
#define POS_N_ORDER              (unsigned int)     (POS_TYPE     +   sizeof(unsigned int))
#define POS_LEN_DATA             (unsigned int)     (POS_N_ORDER  +   sizeof(unsigned int))
#define POS_DATA                 (unsigned int)     (POS_LEN_DATA +   sizeof(unsigned int))


typedef struct{
    unsigned int            type                    ;   //     4
    unsigned int            n_order                 ;   //     4
    unsigned int            len_data                ;   //     4
    char*                   data                    ;   //  4080
    unsigned char           md_value[32]            ;
} msg_struct_t;

char*           get_msg_to_transmit                 (unsigned int type,unsigned int n_order,unsigned int len_data, 
                                                     void* data, size_t* len_msg_p);
char*           reconstruir_mensaje                 (msg_struct_t* msg_struct, size_t* len);
msg_struct_t*   get_msg_struct_from_msg_received    (char* msg);

ssize_t         receive_msg                         (int sfd, char* msg);

int             is_checksum_ok                      (char* msg, size_t len,unsigned char* md_value);
void            obtener_checksum                    (const void *mensaje, size_t msg_len, unsigned char* checksum, 
                                                     unsigned int* md_len_p);
void*           receive_data_msg                    (int sfd, size_t* len_p);
ssize_t         send_data_msg                       (int sfd, void* data, size_t data_len, int flags);

/* 
Types:
    - 0 -> log
    - 1 -> productor1
    - 2 -> productor2
    - 3 -> productor3

    - (1<<28)+1 -> Msg de validacion por parte del dm
    - (1<<28)+2 -> Msg de respuesta de validacion por parte del cliente

 */    