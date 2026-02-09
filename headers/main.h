#ifndef MAIN_H
#define main_h

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <openssl/sha.h>
#include <errno.h>
#include <zlib.h>
#include <dirent.h>

typedef enum{
    cmd_init,
    cmd_hash_object,
    cmd_unhash_object,
    cmd_commit_all,
    cmd_log,
    cmd_cgit_checkout,
    cmd_restore,
    cmd_error
} type;

type get_command(char* a, int argc);

int cgit_init();
int cgit_hash_object(char* path);
int cgit_unhash_object(char* hash);
int cgit_commit_all(char* message);
int cgit_log();
int cgit_checkout(char* hash);
int cgit_restore();

#endif