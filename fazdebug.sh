gcc -g -m32 -c ./src/*.c -Wall
ar crs ./lib/libt2fs.a *.o ./lib/apidisk.o
gcc -g -m32 -o ./bin/main ./main.c -L./lib/ -lt2fs -Wall
