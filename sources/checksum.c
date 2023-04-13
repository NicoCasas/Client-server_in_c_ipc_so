#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <zip.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "checksum.h"

static ssize_t receive_offset(int sfd,char* msg, unsigned int* data_len_p);
static ssize_t receive_data_and_checksum(int sfd,char* msg, unsigned int* data_len_p);
static void    bin_cpy(char* dest, char* src, size_t len);
static void*   realloc_safe(void* ptr, size_t size);
static ssize_t send_safe(int fd, const void* buf, size_t n, int flags);

/* void separar_checksum(char* a_enviar,char* mensaje, unsigned char* checksum){
    size_t n;
    char* aux = index(a_enviar,'|');
    n = strlen(a_enviar)-strlen(aux);
    strncpy(mensaje,a_enviar,n);
    mensaje[n]='\0';
    aux += sizeof(char);
    strcpy((char *)checksum,aux);
} */

// https://linux.die.net/man/3/evp_digestinit
// https://wiki.openssl.org/index.php/EVP_Message_Digests
void obtener_checksum(const void *mensaje, size_t msg_len, unsigned char* checksum,unsigned int* md_len_p){
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;

    md = EVP_get_digestbyname(DIGEST_NAME);
    
    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, mensaje, msg_len);
    EVP_DigestFinal_ex(mdctx, checksum, md_len_p);
    EVP_MD_CTX_free(mdctx);

}

/* int comprobar_checksum(char* msg, size_t len,unsigned char* md_value){
    unsigned char new_md_value[DIGEST_SIZE];
    unsigned int md_len;
    memset(new_md_value,0,DIGEST_SIZE);
    obtener_checksum(msg,len-DIGEST_SIZE,new_md_value,&md_len);

    for(int i=0;i<DIGEST_SIZE;i++){
        if(md_value[i]!=new_md_value[i]){
            return 0;
        }
    }

    return 1;

}
 */

char* get_msg_to_transmit(unsigned int type,unsigned int n_order,unsigned int len_data,void* data, size_t* len_msg_p){
    unsigned char md_value[DIGEST_SIZE];
    size_t len;
    unsigned int md_len;
    
    len = len_data + 3*sizeof(unsigned int) + DIGEST_SIZE;
    char* msg = calloc(len,1);

    bin_cpy(msg,(char*)&type,4);
    bin_cpy(msg+4,(char*)&n_order,4);
    bin_cpy(msg+8,(char*)&len_data,4);
    bin_cpy(msg+12,(char*)data,len_data);

    memset(md_value,0,DIGEST_SIZE);
    obtener_checksum(msg,len-DIGEST_SIZE,md_value,&md_len);
    bin_cpy(msg+12+len_data,(char*)md_value,md_len);

    if(len_msg_p != NULL){
        *len_msg_p = len;
    }

    return msg;
}

void bin_cpy(char* dest, char* src, size_t len){
    for(size_t i = 0; i<len; i++){
        dest[i] = src[i]; 
    }
}

/**
 * Returns a pointer to a dinamically created msg_struct_t with its fields completed using msg
 * Msg has to have the form of 'type''n_order''len_data''data''crc_16_term'
 * The struct must be freed 
 * Data field is also dinamically initialized so it must be freed
*/
msg_struct_t* get_msg_struct_from_msg_received(char* msg){
    msg_struct_t* msg_struct = malloc(sizeof(msg_struct_t));

    bin_cpy((char*)&msg_struct->type,msg,sizeof(unsigned int));
    bin_cpy((char*)&msg_struct->n_order,msg+sizeof(unsigned int),sizeof(unsigned int));
    bin_cpy((char*)&msg_struct->len_data,msg+2*sizeof(unsigned int),sizeof(unsigned int));

    msg_struct->data = calloc(1,msg_struct->len_data);
    bin_cpy(msg_struct->data,msg+3*sizeof(unsigned int),msg_struct->len_data);

    bin_cpy((char*)&msg_struct->md_value,msg+3*sizeof(unsigned int)+msg_struct->len_data,DIGEST_SIZE);

    return msg_struct;
}


/**
 * Returns 1 (true) if checksum is correctly validated, 0 (false) otherwise.
*/
int is_checksum_ok(char* msg, size_t len,unsigned char* md_value){
    unsigned char new_md_value[DIGEST_SIZE];
    unsigned int md_len;
    memset(new_md_value,0,DIGEST_SIZE);
    obtener_checksum(msg,len-DIGEST_SIZE,new_md_value,&md_len);

    for(int i=0;i<DIGEST_SIZE;i++){
        if(md_value[i]!=new_md_value[i]){
            return 0;
        }
    }

    return 1;
}

char* reconstruir_mensaje(msg_struct_t* msg_struct, size_t* len){
    return get_msg_to_transmit  (msg_struct->type,msg_struct->n_order,
                                 msg_struct->len_data,msg_struct->data,
                                 len);
}

/**
 * Sends data via sfd regardless data size, using the checksum.c interface
*/
ssize_t send_data_msg(int sfd, void* data, size_t data_len, int flags){
    //char msg_buff[N_BYTES_TO_SEND];
    ssize_t bytes_sended = 0;
    char* msg_buff = NULL;
    unsigned int count = 0;
    size_t msg_len = 0;

    char* send_from = data;
    size_t remaining_data = data_len;

    while(remaining_data > MSG_DATA_SIZE){
        msg_buff = get_msg_to_transmit(1,count,MSG_DATA_SIZE,send_from,&msg_len);
        bytes_sended += send_safe(sfd,msg_buff,msg_len,flags);
        count++;
        send_from += MSG_DATA_SIZE; 
        remaining_data -= MSG_DATA_SIZE;
        free(msg_buff);
    };
    
    if(remaining_data==0) return bytes_sended;

    msg_buff = get_msg_to_transmit(0,count,(unsigned int)remaining_data,send_from,&msg_len);
    bytes_sended += send_safe(sfd,msg_buff,msg_len,0);
    
    free(msg_buff);
    return bytes_sended;
}

ssize_t send_safe(int fd, const void* buf, size_t n, int flags){
    ssize_t bytes_send = send(fd,buf,n,flags);
    if(bytes_send<0){
        perror("Error sending msg");
        exit(1);
    }
    return bytes_send;
}

/**
 * Returns a pointer to the complete data comming by socket regardless of the size of the message.
 * It's dynamically alocated so it must be freed
 * If sfd disconnects, NULL will be returned and *len_p will be equals to 0
 * If the message if corrupted i.e. checksum does not match, NULL be returned and *len_p 
 * will be equals to 1
*/
void* receive_data_msg(int sfd, size_t* len_p){
    char msg_buff[N_BYTES_TO_RECEIVE];
    char* complete_data = NULL;
    msg_struct_t* msg_struct = NULL;
    size_t total_size = 1;
    ssize_t bytes_received = 0;
    int last_frame=0;

    while(!last_frame){
        memset(msg_buff,0,N_BYTES_TO_RECEIVE);
        bytes_received = receive_msg(sfd,msg_buff);
    
        if(bytes_received == 0){
            *len_p = 0;
            return NULL;
        }
        msg_struct = get_msg_struct_from_msg_received(msg_buff);
        
        if(!is_checksum_ok(msg_buff,(size_t)bytes_received,msg_struct->md_value)){
            //Do something (or not) to take care off
            *len_p = 1;
            return NULL;
        }   

        complete_data = realloc_safe(complete_data,total_size+msg_struct->len_data);
        bin_cpy(complete_data+total_size-1,msg_struct->data,msg_struct->len_data);
        total_size+= msg_struct->len_data;

        last_frame = (msg_struct->len_data < MSG_DATA_SIZE) || (msg_struct->type == 0);

        free(msg_struct->data);
        free(msg_struct);
    }

    complete_data[total_size-1]='\0';
    *len_p = total_size-1;

    return complete_data;
}

void* realloc_safe(void* ptr, size_t size){
    void* aux = realloc(ptr,size);
    if(aux == NULL){
        perror("Error while reallocating");
        exit(1);
    }
    return aux;
}

ssize_t receive_msg(int sfd, char* msg){
    unsigned int data_len;
    ssize_t bytes_read_1,bytes_read_2;

    bytes_read_1 = receive_offset(sfd,msg,&data_len);
    if(bytes_read_1 == 0){
        return 0;
    }
    
    bytes_read_2 = receive_data_and_checksum(sfd,msg,&data_len);

    return (bytes_read_1 + bytes_read_2);
    
}

ssize_t receive_offset(int sfd,char* msg, unsigned int* data_len_p){
    ssize_t bytes_read = recv(sfd,msg,MSG_ADITIONAL_INFO_SIZE,0);
    if(bytes_read==-1){
        perror("Error receiving msg: ");
        exit(1);
    }
    else if(bytes_read == 0){
        return 0;
    }

    bin_cpy((char*)data_len_p,msg+POS_LEN_DATA,sizeof(unsigned int));
    
    return bytes_read;
}

ssize_t receive_data_and_checksum(int sfd,char* msg, unsigned int* data_len_p){
    ssize_t bytes_read = recv(sfd,msg+MSG_ADITIONAL_INFO_SIZE,(size_t)(*data_len_p+DIGEST_SIZE),0);
    if(bytes_read<1){
        perror("Error receiving msg: ");
        exit(1);
    }
    return bytes_read;
}

