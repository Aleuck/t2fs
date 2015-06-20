gcc -m32 -c ./src/*.c -Wall
ar crs ./lib/libt2fs.a *.o ./lib/apidisk.o
gcc -m32 -o teste ./src/main.c -L./lib/ -lt2fs -Wall
#./teste
