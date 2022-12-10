all: main

main: main.o lista_encadeada.o
	gcc -o main main.o lista_encadeada.o -lpaho-mqtt3c -Wall -pedantic

main.o: main.c lista_encadeada.h
	gcc -o main.o main.c -c -Wall -pedantic

lista_encadeada.o: lista_encadeada.c lista_encadeada.h
	gcc -o lista_encadeada.o lista_encadeada.c -c -Wall -pedantic

clean:
	rm -rf *.o *~ main