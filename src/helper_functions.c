#include "../headers/fun.h"
#include "../headers/main.h"


#define MAX_POPULANT_LEN 256
#define MAX_PATH_LEN 256
#define UNCOMPRESSED_GUESS 50 * 1024 * 1024

/*
CRAYON REMEMBER ONE THING
DO NOT MICRO OPTIMISE, FOCUS ON GETTING THINGS DONE, MF ITS A STACK.
WHATEVERS INSIDE GETS POPPED OUT AFTER THE FUNCTION CALLING IS DONE,
YOU ARE NOT LEAKING ANY MEMORY IF YOU DONT ALLOCATE IN THE FIRST PLACE,
PLS USE VALGRIND TO CHECK FOR LEAKS.
*/



type get_command(char* a,int argc){ //returns a type from the enum for the switchcase
    if(strcmp(a,"init")==0){
        if(argc!=2) return cmd_error;
        return cmd_init;
    }
    if(strcmp(a,"hash-object")==0){
        if(argc!=3) return cmd_error;
        return cmd_hash_object;
    }

    if(strcmp(a, "unhash-object")==0){
        if(argc!=3) return cmd_error;
        return cmd_unhash_object;
    }
    if(strcmp(a, "commit-all")==0){
        if(argc!=3) return cmd_error;
        return cmd_commit_all;
    }

    if(strcmp(a,"log")==0){
        if(argc!=2) return cmd_error;
        return cmd_log;
    }

    if(strcmp(a, "checkout")==0){
        if(argc!=3) return cmd_error;
        return cmd_cgit_checkout;
    }

    if(strcmp(a,"restore")==0){
        if(argc!=2) return cmd_error;
        return cmd_restore;
    }
    
    return cmd_error;
}

void get_raw_hash_from_hash(char *hash, unsigned char *raw_hash){ //get raw_hash from hash, hash has to be 41 and raw hash 20
    for(int i = 0; i < 20; i++){
        unsigned int byte;
        sscanf(hash + i*2, "%2x", &byte);
        raw_hash[i] = (unsigned char)byte;
    }
}

int get_raw_hash_from_path(char *path, unsigned char *raw_hash){ // raw_hash must be at least 20 bytes
    FILE *fp = fopen(path,"rb");
    if(fp == NULL){
        printf("Error opening file %s\n", path);
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long int f_size = ftell(fp);
    rewind(fp);

    char *file_bytes = malloc(f_size ? f_size : 1);
    if(!file_bytes){
        fclose(fp);
        printf("Memory alloc for file bytes failed\n");
        return -1;
    }

    size_t file_length = fread(file_bytes, 1, f_size, fp);
    if(file_length != f_size){
        printf("File read error in get_raw_hash_from_path\n");
        free(file_bytes);
        fclose(fp);
        return -1;
    }

    char header[64];
    int header_len = snprintf(header, sizeof(header), "blob %zu", file_length);

    size_t payload_len = header_len + 1 + file_length;
    char *payload = malloc(payload_len);
    if(!payload){
        free(file_bytes);
        fclose(fp);
        printf("Memory alloc for payload failed\n");
        return -1;
    }

    memcpy(payload, header, header_len);
    payload[header_len] = '\0';
    memcpy(payload + header_len + 1, file_bytes, file_length);

    SHA1((unsigned char*)payload, payload_len, raw_hash);

    free(file_bytes);
    free(payload);
    fclose(fp);
    return 0;
}

int get_hash_from_path(char *path, char *hex){ //gets hash of the file and puts it in hex which is always of 41 chars 

    unsigned char hash[20];
    get_raw_hash_from_path(path, hash);
    for(int i=0;i<20;i++) {
        sprintf(hex + i*2, "%02x", hash[i]);
    }
    hex[40] = '\0';
    printf("%s\n",hex);
    return 0;
}

void get_hash_from_raw_hash(unsigned char *raw_hash, char *hash){
    for(int i = 0; i < 20; i++){
        sprintf(hash + i*2, "%02x", raw_hash[i]);
    }
    hash[40] = '\0';
}

int make_file_compress_it(unsigned char* tree_payload, int total_len, unsigned char* tree_hash){ //inputs raw hash and payload and makes a file there in objects
    size_t compressed_payload_len = compressBound(total_len);
    unsigned char* compressed_payload = malloc(compressed_payload_len);
    if(compressed_payload==NULL){
        printf("Allocating memory to compressed payload failed\n");
        // free(tree_payload);
        return -1;
    }

    if(compress(compressed_payload,&compressed_payload_len, tree_payload, total_len)!=Z_OK){
        printf("Payload compression failed\n");
        //free(tree_payload);
        free(compressed_payload);   
        return -1;
    }

    //free(tree_payload);

    //make new file

    char hex[41];  // 40 chars + '\0'
    for(int i=0;i<20;i++) {
        sprintf(hex + i*2, "%02x", tree_hash[i]);
    }

    hex[40] = '\0';
    char obj_dir[64];
    sprintf(obj_dir, ".cgit/objects/%c%c",hex[0],hex[1]);
    
    if(mkdir(obj_dir,0777)!=0 && errno != EEXIST){
        printf("Making Hash Directory Failed\n");
        return -1;
    }

    char file_name[39];
    for(int i=0;i<38;i++){
        file_name[i]=hex[2+i];
    }
    file_name[38]='\0';

    char full_path[128];

    snprintf(full_path, sizeof(full_path), "%s/%s", obj_dir, file_name);
    FILE *new_file = fopen(full_path, "wb");
    if(new_file==NULL){
        printf("Making the Hash File Failed\n");
        return -1;
    }

    size_t end= fwrite(compressed_payload, 1, compressed_payload_len, new_file);
    fclose(new_file);

    if(end!=compressed_payload_len){
        printf("Entire File Has'nt been written\n");
        return -1;
    }

    free(compressed_payload);
    return 0;
}

int build_tree(char *path, unsigned char *tree_hash){ //builds a tree, hashes all of them and keeps it in .cgit/objects
    char *object_hash_array[MAX_POPULANT_LEN];
    int count = 0;

    DIR *dp = opendir(path);
    if(dp == NULL){
        printf("Error opening directory %s\n", path);
        return -1;
    }
    struct dirent *curr;
    while( (count<MAX_POPULANT_LEN) && ( (curr=readdir(dp)) !=NULL )){
        if(strcmp(curr->d_name,".")==0 || strcmp(curr->d_name,"..")==0) continue;
        if(strcmp(curr->d_name, ".cgit") == 0) continue;
        char curr_path[MAX_PATH_LEN];
        snprintf(curr_path, sizeof(curr_path), "%s/%s", path, curr->d_name);

        struct stat st;
        if(stat(curr_path, &st)!=0) continue;

        if(S_ISREG(st.st_mode)){ //100644 file_name\0[20 bytes of raw hash]
            unsigned char raw_hash[20];
            get_raw_hash_from_path(curr_path, raw_hash);
            cgit_hash_object(curr_path);

            char* payload = malloc(1024); //fixed 1024 allocate
            if(!payload) continue;

            int header_len = snprintf(payload, 1024, "100644 %s", curr->d_name);
            payload[header_len] = '\0';
            memcpy(payload + header_len + 1, raw_hash, 20);

            object_hash_array[count++]=payload;
        }

        else if(S_ISDIR(st.st_mode)){ //40000 dir_name\0[20 bytes of returned raw hash]
            unsigned char dir_hash[20];
            if(build_tree(curr_path, dir_hash)==-1) continue;

            char *payload = malloc(1024);
            if(!payload) continue;

            int header_len = snprintf(payload, 1024, "40000 %s", curr->d_name);
            payload[header_len] = '\0';
            memcpy(payload + header_len + 1, dir_hash, 20);
            object_hash_array[count++]=payload;

        }
        else{
            printf("File type not recognised: %s\n", curr->d_name);
            closedir(dp);
            return -1;
        }
    }
    closedir(dp);

    size_t total_len=0;
    for(int i=0; i<count; i++){
        total_len+=strlen(object_hash_array[i])+1+20;
    }

    char* tree_payload = malloc(total_len);
    if(!tree_payload) return -1;

    size_t offset=0;
    for(int i=0;i<count;i++){
        int header_len = strlen(object_hash_array[i]);
        memcpy(tree_payload + offset, object_hash_array[i], header_len +1 + 20);
        offset += header_len +1 +20;
        free(object_hash_array[i]);
    }

    char header[64];
    int header_len = snprintf(header, sizeof(header), "tree %zu", total_len);

    size_t full_len = header_len + 1 + total_len;

    char *full_payload = malloc(full_len);
    if(!full_payload){
        printf("Full Payload memory alloc failed\n");
        return -1;
    }

    memcpy(full_payload, header, header_len);
    full_payload[header_len] = '\0';
    memcpy(full_payload + header_len + 1, tree_payload, total_len);

    free(tree_payload);
    //compression
    //tree <tree_len>\0+payload 
    SHA1(full_payload, full_len, tree_hash);
    //compression logic
    if(make_file_compress_it(full_payload, full_len, tree_hash)!=0){
        free(full_payload);
        return -1;
    }
    free(full_payload);
    
    return 0;

}

int get_raw_parent_commit_hash(unsigned char* raw_out){
    FILE *fp = fopen(".cgit/refs/master","r");
    if(!fp){
        printf("Error Opening File\n");
        return -1;
    }
    char hex[41];
    if (fscanf(fp, "%40s", hex) != 1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    for(int i=0;i<20;i++){
        unsigned int byte;
        sscanf(hex + (i * 2), "%02x", &byte);
        raw_out[i] = (unsigned char)byte;
    }
    return 0;
}

int update_ref_master(char* str){
    FILE *fp= fopen(".cgit/refs/master","w");
    if(!fp){
        printf("Error opening file\n");
        return -1;
    }
    int n=fwrite(str, 1, strlen(str), fp);
    if(n!=strlen(str)){
        printf("File has'nt fully been written\n");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

//.cgit/logs/master
int update_log(char* str, char* message){
    message[strcspn(message, "\r\n")] = 0;
    str[strcspn(str, "\r\n")] = 0;
    FILE *fp = fopen(".cgit/logs/master", "a");
    if(!fp){
        printf("Opening File Failed\n");
        return -1;
    }
    fprintf(fp, "%s %s\n", str, message);
    fclose(fp);
    return 0;
}



/* 
100644 <filename>\0<20-byte blob hash> for files 
40000 <dirname>\0<20-byte tree hash> for directories 
tree <payload_size>\0<tree_payload> for trees 
*/
int uncompress_file_and_make_file(char* dest_path, unsigned char* raw_hash){ //expands and makes file at given path

    char hash[41]; //unsigned char* hash?
    get_hash_from_raw_hash(raw_hash, hash);

    char path[64];
    sprintf(path, ".cgit/objects/%c%c/%s",hash[0],hash[1],hash+2);

    FILE *fp=fopen(path, "rb");
    if(!fp){
        printf("Cannot Open File\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size_t total_compressed_len = ftell(fp);
    rewind(fp);

    char* compressed_bytes = malloc(total_compressed_len);

    if(!compressed_bytes){
        printf("Malloc failed for compressed bytes\n");
        fclose(fp);
        return -1;
    }

    size_t total_len_read = fread(compressed_bytes, 1, total_compressed_len, fp);
    
    if(total_len_read != total_compressed_len){
        printf("Full File hasnt been extracted into compressed bytes\n");
        free(compressed_bytes);
        fclose(fp);
        return -1;
    }

    fclose(fp);

    size_t uncompressed_length = UNCOMPRESSED_GUESS; //BIGGEST GUESS CAN OPTIMISE BY READING BLOB SIZE LATER, PARTIAL UNCOMPRESS NEEDED
    //10 MB MAX SIZE LEN

    unsigned char* payload = malloc(uncompressed_length);

    if(!payload){
        printf("Memory allocation for payload failed\n");
        return -1;
    }

    if(uncompress2(payload, &uncompressed_length, compressed_bytes, &total_compressed_len)!=Z_OK){
        printf("uncompression failed\n");
        free(payload);
        return -1;
    }

    free(compressed_bytes);

    FILE *new_file = fopen(dest_path, "wb");
    if(!new_file){
        printf("File cannot be made\n");
        free(payload);
        return -1;
    }
    

    int i=0;
    while(i<uncompressed_length && *(payload+i)!='\0') i++;

    fwrite(payload+i+1, 1, uncompressed_length - (i+1), new_file);

    free(payload);
    fclose(new_file);
    return 0;
    
}

int uncompress_general_payload(unsigned char** payload, unsigned char* raw_hash, size_t* total_out_len){ //puts uncompressed value in payload, USER MUST FREE PAYLOAD
    char hash[41]; //unsigned char* hash? yes i copypasted this fn from above
    get_hash_from_raw_hash(raw_hash, hash);

    char path[64];
    sprintf(path, ".cgit/objects/%c%c/%s",hash[0],hash[1],hash+2);

    FILE *fp=fopen(path, "rb");
    if(!fp){
        printf("Cannot Open File\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size_t total_compressed_len = ftell(fp);
    rewind(fp);

    unsigned char* compressed_bytes = malloc(total_compressed_len);

    if(!compressed_bytes){
        printf("Malloc failed for compressed bytes\n");
        fclose(fp);
        return -1;
    }

    size_t total_len_read = fread(compressed_bytes, 1, total_compressed_len, fp);
    
    if(total_len_read != total_compressed_len){
        printf("Full File hasnt been extracted into compressed bytes\n");
        free(compressed_bytes);
        fclose(fp);
        return -1;
    }

    fclose(fp);

    size_t uncompressed_length = UNCOMPRESSED_GUESS;

    *payload = malloc(uncompressed_length);
    if(!*payload){
        printf("Memory allocation for payload failed\n");
        return -1;
    }

    if(uncompress2(*payload, &uncompressed_length, compressed_bytes, &total_compressed_len)!=Z_OK){
        printf("uncompression failed\n");
        free(compressed_bytes);
        free(*payload);
        return -1;
    }
    free(compressed_bytes);
    *total_out_len = uncompressed_length; 

    return 0;
}

/*
commit <content_size>\0 
tree_hash (20 bytes) 
parent_hash (20 bytes, if 0000000000.. is the first init); 
message
*/

int read_commit(char* hash, unsigned char* parent_hash, unsigned char* tree_hash){

    unsigned char raw_hash[20];
    get_raw_hash_from_hash(hash, raw_hash);

    unsigned char* payload;
    size_t payload_len=0;
    

    if(uncompress_general_payload(&payload, raw_hash, &payload_len)==-1) return -1;

    int i=0;
    while(i<payload_len && *(payload+i)!='\0') i++;
    i++;

    memcpy(tree_hash, payload+i, 20);
    memcpy(parent_hash, payload+i+20, 20);

    free(payload);
    return 0;
}

int unbuild_tree(unsigned char* tree_hash, char *path){

    /* 
    100644 <filename>\0<20-byte blob hash> for files 
    40000 <dirname>\0<20-byte tree hash> for directories 
    tree <payload_size>\0<tree_payload> for trees 
    */
    unsigned char* main_payload;
    size_t main_payload_size;   
    if(uncompress_general_payload(&main_payload, tree_hash, &main_payload_size)==-1) return -1;
    

    int i=0;

    while(i < main_payload_size && main_payload[i] != '\0') i++;
    i++;  

    char mode[10];
    int mode_index=0;

    int file_name_index=0;
    char file_name[128];
    char file_path[2048];
    unsigned char raw_file_hash[20];

    int directory_name_index=0;
    char dir_name[128];
    char dir_path[2048];
    unsigned char raw_dir_hash[20];


    while(i+1 <main_payload_size){

        memset(mode, 0, 10);
        mode_index=0;

        while(i<main_payload_size && main_payload[i]!=' '){
             mode[mode_index]=main_payload[i];
             i++;
             mode_index++;
        }
        mode[mode_index]='\0';
        i++;

        if(strcmp(mode, "100644")==0){

            memset(file_name, 0, 128);
            file_name_index=0;

            while(i<main_payload_size && main_payload[i]!='\0'){
                file_name[file_name_index]=main_payload[i];
                i++;
                file_name_index++;
            }
            file_name[file_name_index]='\0';
            i++;

            memset(file_path, 0, 2048);
            sprintf(file_path, "%s/%s",path, file_name);

            memcpy(raw_file_hash, main_payload+i,20);
            i+=20;


            if(uncompress_file_and_make_file(file_path, raw_file_hash)==-1){
                free(main_payload);
                return -1;
            }

        }
        
        else if(strcmp(mode, "40000")==0){

            memset(dir_name,0,128);
            directory_name_index=0;

            while(i<main_payload_size && main_payload[i]!='\0'){
                dir_name[directory_name_index]=main_payload[i];
                directory_name_index++;
                i++;
            }
            i++;
            dir_name[directory_name_index]='\0';

            memset(dir_path, 0, 2048);
            sprintf(dir_path, "%s/%s", path, dir_name);

            mkdir(dir_path, 0777);

            memcpy(raw_dir_hash, main_payload+i, 20);
            i+=20;

            

            if(unbuild_tree(raw_dir_hash, dir_path)==-1){
                free(main_payload);
                return -1;
            }

        }
        else{
            printf("Format Error\n");
            free(main_payload);
            return -1;
        }
    }
    free(main_payload);
    return 0;
}

int wipe_working_tree(char* path){
    
    DIR* dp = opendir(path);
    if(dp == NULL){
        printf("Error Opening Directory");
        return -1;
    }

    struct dirent *curr;
    while( (curr=readdir(dp)) !=NULL){
        if(strcmp(curr -> d_name, ".")==0 || strcmp(curr -> d_name, "..")==0 || strcmp(curr -> d_name, ".cgit")==0) continue;

        char curr_path[MAX_PATH_LEN];
        sprintf(curr_path, "%s/%s", path, curr->d_name);

        struct stat st;
        if (lstat(curr_path, &st) != 0) continue;

        if(S_ISREG(st.st_mode)){
            unlink(curr_path);
        }
        else{
            wipe_working_tree(curr_path);
            rmdir(curr_path);
        }
    }
    closedir(dp);
    return 0;

}
