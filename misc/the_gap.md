## current issues 

 - Head is empty, will be used for brancing
 - No explicit branching, manual branching through commit log
 - The uncompression we're doing after fetching is based on guess work, inefficient in nature, need to address this by partial uncompression
 - micro-optimisation is not done, memory on stack is based on guesses because IT GETS POPPED DONT NEED TO OPTIMISE
 - unhash-object function is redundant, much better guess is done in helper functions in src/helper-functions.c, the logic is preliminary 
 - further staging functions required (rn no staging area is implemented)
 - cgit diff is under progress
 - cgit status shall be implemented at the earliest



 # any prs to address these are well appreciated