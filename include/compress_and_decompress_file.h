long file_size(char *filename);
int decompress_one_file(char *infilename, char *outfilename);
int compress_one_file(char *infilename, char *outfilename);
int compress_one_file_from_var(char* var, unsigned int len_var, char* outfilename);
