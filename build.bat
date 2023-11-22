rm main.exe bin\strutils.o bin\main.o
gcc -g -c -I"cup/strutils" -o bin\strutils.o cup\strutils\strutils.c 
gcc -g -c -o bin\main.o main.c
gcc -g -o main.exe bin\main.o bin\strutils.o -lws2_32
