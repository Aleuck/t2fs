gcc -m32 -c t2fs.c -Wall
ar crs libt2fs.a t2fs.o apidisk.o
gcc -m32 -o teste main.c -L./ -lt2fs -Wall
./teste
