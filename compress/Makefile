all: compress.exe

compress.exe: lzss.c main.c
	gcc -D_GNU_SOURCE -o lzss.o -c lzss.c
	gcc -o main.o -c main.c
	gcc -o compress.exe lzss.o main.o

clean:
	@rm -f compress.o compress.exe
	@echo "all cleaned up !"
