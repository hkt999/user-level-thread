test: main.c thread.c platform.c
	gcc -g main.c thread.c platform.c -o test -D_XOPEN_SOURCE

clean:
	rm test
