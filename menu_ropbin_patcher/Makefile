all: menu_ropbin_patcher.exe

menu_ropbin_patcher.exe: main.c ../app_targets/app_targets.h
	gcc -o main.o -c main.c
	gcc -o menu_ropbin_patcher.exe main.o

clean:
	@rm -f main.o menu_ropbin_patcher.exe
	@echo "all cleaned up !"
