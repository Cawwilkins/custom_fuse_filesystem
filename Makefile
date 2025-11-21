parser: parser.c
	gcc parser.c -o parser

build_img: generate-simplefs.c
	gcc generate-simplefs.c -o generator && ./generator

run_parser: parser
	./parser

driver: simplefs_main.c
	gcc -g -Wall -Wextra -pedantic -Werror -std=gnu11 simplefs_main.c `pkg-config fuse3 --libs --cflags` -o simplefs_main
