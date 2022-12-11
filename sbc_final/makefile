all: main

main: main.o lista_encadeada.o lcd.o
	gcc -o main main.o lista_encadeada.o -lpaho-mqtt3c -lwiringPi -lwiringPiDev -pthread -Wall -pedantic

main.o: main.c lista_encadeada.h
	gcc -o main.o main.c -c -Wall -pedantic

lista_encadeada.o: lista_encadeada.c lista_encadeada.h
	gcc -o lista_encadeada.o lista_encadeada.c -c -Wall -pedantic

lcd.o:lcd.s
	as -g -o lcd.o lcd.s
  
map.o:map.s
	as -g -o map.o map.s

clean:
	rm -rf *.o *~ main