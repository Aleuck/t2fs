gcc -m32 -c ./src/*.c -Wall
ar crs libt2fs.a *.o apidisk.o
gcc -m32 -o teste ./src/main.c -L./ -lt2fs -Wall
./teste
