test: main.c thread.c platform.c
	gcc -g main.c thread.c platform.c -o test

clean:
	rm test
