#include <stdio.h>
#include <stdlib.h>
#include "zlib.h"
#include "compress_and_decompress_file.h"
/**
 *  Extraido y ligeramente modificado (control de errores)
 *  de https://www.codeguru.com/cplusplus/zlib-add-industrial-strength-compression-to-your-c-c-apps/
*/

long file_size(char *filename){
    int e=0;
    FILE *pFile = fopen(filename, "rb");
    if(pFile==NULL){
        perror("Error while opening file: ");
        exit(1);
    }
    e = fseek (pFile, 0, SEEK_END);
    if(e==-1){
        perror("Error fseeking: ");
        exit(1);
    }
    long size = ftell(pFile);
    if(size == (long) -1){
        perror("Error while ftelling: ");
        exit(1);
    }
    if(fclose(pFile) !=0 ){
        perror("Error while closing file: ");
        exit(1);
    }
    return size;
}

/**
 * Decompress 'infilename' in 'outfilename'
 * Returns -1 in case of error, 0 in success
 * */
int decompress_one_file(char *infilename, char *outfilename)
{
    gzFile infile = gzopen(infilename, "rb");
    FILE *outfile = fopen(outfilename, "wb");
    if (!infile || !outfile) return -1;
    char buffer[128];
    int num_read = 0;
    while ((num_read = gzread(infile, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, (size_t) num_read, outfile);
    }
    gzclose(infile);
    fclose(outfile);
    return 0;
}

/**
 * Compress 'infilename' in 'outfilename'
 * Returns -1 in case of error, 0 in success
 * */
int compress_one_file(char *infilename, char *outfilename)
{
    FILE *infile = fopen(infilename, "rb");
    gzFile outfile = gzopen(outfilename, "wb");
    if (!infile || !outfile) return -1;
    char inbuffer[128];
    size_t num_read = 0;
    size_t num_write = 0;
    unsigned long total_read = 0, total_wrote = 0;
    while ((num_read = fread(inbuffer, 1, sizeof(inbuffer), infile)) > 0) {
        total_read += num_read;
        num_write = (size_t) gzwrite(outfile, inbuffer, (unsigned int)num_read);
        if(num_write==0){
            perror("Error while compressing");
            exit(1);
        }
        total_wrote+=(unsigned long) num_write;
    }
    fclose(infile);
    gzclose(outfile);
    printf("Read %ld bytes, Wrote %ld bytes, Compression factor %4.2f%%\n", total_read, file_size(outfilename), 
           (1.0-(double)file_size(outfilename)*1.0/(double)total_read)*100.0);
    return 0;
}

/**
 * Compress len_var bytes of var in 'outfilename'
 * Returns -1 in case of error, 0 in success
 * */
int compress_one_file_from_var(char *var, unsigned int len_var, char *outfilename)
{
    gzFile outfile = gzopen(outfilename, "wb");
    if (!outfile) return -1;
    
    int num_write = gzwrite(outfile,var,len_var);
    if(num_write==0){
        perror("Error while compressing");
        exit(1);
    }
    gzclose(outfile);
    
    printf("Read %d bytes, Wrote %ld bytes, Compression factor %4.2f%%\n", len_var, file_size(outfilename), 
           (1.0-(double)file_size(outfilename)*1.0/(double)num_write)*100.0);
    return 0;
}


/* int main(int argc, char **argv)
{
    if(argc < 4){
        perror("Missing arguments");
        exit(1);
    } 
    //compress_one_file("../log/activity.log","../log/activity_bueno.z");
    //decompress_one_file("../log/activity.z","../log/activity2.log");
    //decompress_one_file("../log/activity2.z","../log/activity2.log");
} */
