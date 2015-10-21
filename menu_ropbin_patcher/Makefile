all: menu_ropbin.exe

menu_ropbin.exe: main.c ../app_targets/app_targets.h
	gcc -o main.o -c main.c
	gcc -o menu_ropbin.exe main.o

clean:
	@rm -f main.o menu_ropbin.exe
	@echo "all cleaned up !"
