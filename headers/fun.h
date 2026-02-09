#ifndef FUN_H
#define fun_h

int get_hash_from_path(char *path, char *hex); 
int get_raw_hash_from_path(char *path, unsigned char *raw_hash); 
void get_hash_from_raw_hash(unsigned char *raw_hash, char *hash);
int get_raw_parent_commit_hash(unsigned char* raw_out);

int make_file_compress_it(unsigned char* tree_payload, int total_len, unsigned char* tree_hash);

int build_tree(char *path, unsigned char *tree_hash);

int update_ref_master(char* str);
int update_log(char* str, char* message);

int read_commit(char* hash, unsigned char* parent_hash, unsigned char* tree_hash);
int unbuild_tree(unsigned char* tree_hash, char *path);
int wipe_working_tree(char* path);


#endif