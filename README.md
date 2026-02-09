## CGIT, custom version control system

Object based storage system where we have a central directory ".cgit" which stores snapshots and certain places.  
.cgit/objects stores hashes based on its hash and stores compressed hashes at .cgit/objects/xx/xxx...  
The compression is done using the DEFLATE compression algorithm and hashing using SHA1  
A regular workflow looks like this  
init => commit-all "message" => log => checkpoint 'hash'  


init makes the .cgit directory in your tree  
commit-all recursively snapshots the tree and stores it at xx/xxx.. in .cgit/objects, more on the formats in ./structure_and_formats/  
log prints all the commits  
checkpoint 'hash' jumps to a previous snapshot  
and restore the present working directory and restores the previous commit which is stored in .cgit/refs/master  

---

## Functions implemented / Syntax

- cgit init                     => initialises the repo, makes the cgit file
- cgit hash-object \<hash\>     => snapshots a single file
- cgit commit-all \<hash\>      => snapshots the entire working directory
- cgit log                      => prints all the commits along with their messages
- cgit unhash-object \<hash\>   => uncompresses a file and stores and prints it to stdout
- cgit checkout \<commit hash\> => reverts back to that snapshot
- cgit restore                  => restores the latest snapshot, erases whats in your pwd

---

Everyone is advised to read the_gap.md before using

--- 

# Download and usage

- use the make build file to build the output and symlink to usr/bin
- to test it out, try the tests/ repository right away after cding into tests/
- The application is mostly memory safe and is verified by valgrind
- Use the functions in utils/ to get hash to use unhash-object

---

# Future scope

Hopefully will make the storage system in itself difference based rather than snapshot based to preserve even more memory in areas with high computation. Unlike git.  
Implement Clone and Networking features. 

---