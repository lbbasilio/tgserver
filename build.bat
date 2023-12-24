rm main.exe bin\main.o
gcc -g -c -o bin\http_parser.o http_parser\http_parser.c
gcc -g -c -o bin\main.o main.c
gcc -g -o main.exe bin\main.o bin\http_parser.o -lws2_32
