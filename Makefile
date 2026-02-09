build:
	gcc src/*.c -o output/cgit -lcrypto -lz

clean:
	rm -f output/cgit

leaks:
	valgrind --leak-check=full ./output/cgit hash-object tests/hash_test_1.txt

remove_init:
	rm -rf .cgit

build_get_hash:
	gcc utils/get_hash.c src/cgit_commands.c src/helper_functions.c -o output/util_get_hash -lcrypto -lz

get_hash:
	./output/util_get_hash src/main.c