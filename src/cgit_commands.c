#include "../headers/fun.h"
#include "../headers/main.h"


int cgit_init(){ //Initialises all the files needed for .cgit

    if(mkdir(".cgit",0777)!=0){ //0777 Read/Write/Execute
        printf("Error making .cgit folder\n");
        return -1;
    }

    if(mkdir(".cgit/objects",0777)!=0){ 
        printf("Error making objects folder\n");
        return -1;
    }

    if(mkdir(".cgit/refs",0777)!=0){ 
        printf("Error making refs folder\n");
        return -1;
    }

    if(mkdir(".cgit/logs",0777)!=0){ 
        printf("Error making refs/master folder\n");
        return -1;
    }

    FILE *HEAD = fopen(".cgit/HEAD","w");
    if(HEAD==NULL){
        printf("Error Creating HEAD file\n");
        return -1;
    }
    
    FILE *master=fopen(".cgit/refs/master","w");
    if(master==NULL){
        printf("Error creating .cgit/refs/master");
        fclose(HEAD);
        return -1;
    }

    FILE *m=fopen(".cgit/logs/master","w");
    if(m==NULL){
        printf("Error creating .cgit/logs/master");
        fclose(HEAD);
        fclose(master);
        return -1;
    }

    fclose(m);
    fclose(HEAD);
    fclose(master);
    return 0;

}

int cgit_hash_object(char* path){ //hashes object @ path, compresses it and saves

    FILE *fp= fopen(path,"rb");

    if(fp==NULL){
        printf("Error getting file\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long int f_size = ftell(fp);
    rewind(fp);

    unsigned char* file_bytes= malloc(f_size? f_size :1);
    if(!file_bytes){
        fclose(fp);
        printf("Memory alloc for file bytes failed\n");
        return -1;
    }

    size_t file_length = fread(file_bytes, 1, f_size, fp);
    
    char header[64];
    int header_len = snprintf(header, sizeof(header), "blob %zu", file_length);
    
    size_t payload_len = header_len + 1 + file_length;

    unsigned char* payload = malloc(payload_len);
    if(!payload){
        free(file_bytes);
        fclose(fp);
        printf("Memory alloc for payload failed\n");
        return -1;
    }

    memcpy(payload, header, header_len);
    payload[header_len]='\0';
    memcpy(payload + header_len + 1, file_bytes, file_length);

    //sha1 logic

    unsigned char hash[20];
    SHA1(payload, payload_len, hash);

    free(file_bytes);
    fclose(fp);
    
    make_file_compress_it(payload, payload_len, hash);
    free(payload);
    return 0;
}

int cgit_unhash_object(char* hash){ //prints uncompressed value of whatevers there
    if(strlen(hash)!=40) return -1;
    char temp[39];
    memcpy(temp, hash + 2, 38);
    temp[38] = '\0';
    
    char full_file_path[64];

    sprintf(full_file_path, ".cgit/objects/%c%c/%s", hash[0], hash[1], temp);

    FILE *fp = fopen(full_file_path, "rb");
    if(fp==NULL){
        printf("Error Opening File\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long int f_size = ftell(fp);
    rewind(fp);

    char* file_bytes= malloc(f_size? f_size : 1);
    if(!file_bytes){
        fclose(fp);
        printf("Memory alloc for file bytes failed\n");
        return -1;
    }

    size_t file_length = fread(file_bytes, 1, f_size, fp);
    if (file_length != f_size) {
        printf("File read error in get_hash_hex\n");
        free(file_bytes);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    size_t uncompressed_length=f_size*20;
    unsigned char* uncompressed = malloc(uncompressed_length);
    if(uncompressed==NULL){
        printf("Malloc Failed in uncompressed\n");
        return -1;
    }

    if(uncompress2(uncompressed, &uncompressed_length, file_bytes, &file_length)!=Z_OK){
        printf("uncompression/ decompression IDFK I CANT GRAMMAR failed\n");
        free(uncompressed);
        return -1;
    };

    int i=0;
    while(i<uncompressed_length && *(uncompressed+i)!='\0') i++;
    // printf("%s", uncompressed+i+1);
    fwrite(uncompressed + i + 1, 1,uncompressed_length - (i + 1),stdout);

    free(uncompressed);
    free(file_bytes);

    return 0;
    //"objects/ee/eeeee..."
}

int cgit_commit_all(char* message){ //returns the hash of the root
    unsigned char raw_hash[20];
    if(build_tree(".", raw_hash)==-1) return -1;

    //get parent hash logic
    unsigned char raw_parent_commit_hash[20];
    //0 success, -1 fail
    int has_parent = (get_raw_parent_commit_hash(raw_parent_commit_hash) == 0);

    size_t content_size = 20 + 20 + strlen(message);

    char header[64];
    int header_len = snprintf(header, sizeof(header), "commit %zu", content_size);

    size_t total_len = header_len + 1 + content_size;
    char* payload = malloc(total_len);
    if(!payload){
        printf("Memory Alloc for Payload failed\n");
        return -1;
    }

    memcpy(payload, header, header_len);
    payload[header_len] = '\0';

    int offset = header_len + 1;
    memcpy(payload + offset, raw_hash, 20);
    offset += 20;

    if(has_parent) {
        memcpy(payload + offset, raw_parent_commit_hash, 20);
        offset += 20;
    }
    else{
        memset(raw_parent_commit_hash, 0, 20);
        memcpy(payload + offset, raw_parent_commit_hash, 20);
        offset += 20;
    }

    memcpy(payload + offset, message, strlen(message));
    
    /*
    TEMPLATE
    commit <content_size>\0
    tree_hash (20 bytes)
    parent_hash (20 bytes, optional)
    message
    */

    unsigned char final_raw_hash[20];
    SHA1((unsigned char*)payload, total_len, final_raw_hash);

    if(make_file_compress_it(payload, total_len, final_raw_hash) != 0) {
        return -1;
    }
    free(payload);

    unsigned char hash_str[41];
    get_hash_from_raw_hash(final_raw_hash, hash_str);


    //.cgit/refs/master
    if(update_ref_master(hash_str)==-1){
        printf("Writing into ref master failed");
        return -1;
    }

    //.cgit/logs/master
    if(update_log(hash_str, message)==-1){
        printf("Writing to log failed\n");
        return -1;
    }
    return 0;
}

int cgit_log(){
    FILE* fp= fopen(".cgit/logs/master","r");
    if(!fp){
        printf("cannot open log file\n");
        return -1;
    }

    char buf[1024 + 40 + 3];
    while(fgets(buf, sizeof(buf), fp)){
        fputs(buf, stdout);
    }

    fclose(fp);
    return 0;
}


int cgit_checkout(char* hash){ //go back to an old commit

    unsigned char tree_hash[20];
    unsigned char parent_hash[20];

    if(read_commit(hash, parent_hash, tree_hash)==-1) return -1;

    if(unbuild_tree(tree_hash, ".")==-1) return -1;
    return 0;
}

int cgit_restore(){ //go back to the latest commit

    FILE *fp = fopen(".cgit/refs/master", "r");

    if(!fp){
        printf("File Open Failed\n");
        return -1;
    }


    char hash[41];
    if(fscanf(fp, "%40s", hash) !=1){
        fclose(fp);
        printf("No saved commit");  
        return -1;
    }

    fclose(fp);
    hash[40]='\0';

    unsigned char parent_hash[20];
    unsigned char tree_hash[20];

    wipe_working_tree(".");

    if(read_commit(hash, parent_hash, tree_hash)==-1) return -1;

    if(unbuild_tree(tree_hash,".")==-1) return -1;    

    return 0;

}