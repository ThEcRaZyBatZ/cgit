#include "../headers/main.h"

int main(int argc, char* argv[]){

    if(argc==1){
        printf("cgit init:          initialise the .cgit structure\n"
                "cgit hash-object:   to compress a single file into a blob\n"
                "cgit unhash-object: prints whats inside a blob to stdout\n"
                "cgit unhash-object: prints whats inside a blob to stdout\n"
                "cgit commit-all -m: hashes all of whats inside the pwd\n"
                "cgit log:           displays the commit log\n"
                "cgit checkout -ch:  reverts back to that commit hash, expands everything\n"
                "cgit restore:       reverts back to the latest commit\n"
            );
        return 0;
    }

    switch(get_command(argv[1],argc)){
        case cmd_init:{ //cgit init => makes the required files and folders
            if(cgit_init()==-1) return -1;
            break;
        }
        case cmd_hash_object:{ //cgit hash-object filepath => puts a compressed blob in .cgit/objects/ab/cd....38, total 40 char hash
            if(cgit_hash_object(argv[2])==-1) return -1;
            break;
        }
        case cmd_unhash_object:{ //cgit unhash-object hash => gives uncompressed blob (parsed)
            if(cgit_unhash_object(argv[2])==-1) return -1;
            break;
        }

        case cmd_commit_all:{//hashes all the files, directories and everything present in the file system and commits the root to objects
            if(cgit_commit_all(argv[2])==-1) return -1;
            break;
        }

        case cmd_log:{
            if(cgit_log()==-1) return -1;
            break;
        }

        case cmd_cgit_checkout:{
            if(cgit_checkout(argv[2])==-1) return -1;
            break;
        }

        case cmd_restore:{
            if(cgit_restore()==-1) return -1;
            break;
        }
        
        case cmd_error:{ //general error
            printf("syntax error\n");
            break;
        }
    }
}
