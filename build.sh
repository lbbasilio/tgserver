#!/bin/sh
rm main bin/*
gcc -g -c http_parser/http_parser.c -o bin/http_parser.o
gcc -g -c linux_main.c -o bin/linux_main.o
gcc -g bin/linux_main.o bin/http_parser.o -o main
