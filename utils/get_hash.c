#include "../headers/main.h"

void util_get_h(char *path){
    char hash[40];
    get_hash_from_path(path, hash);
}

int main(int argc, char* argv[]){
    util_get_h(argv[1]);
}