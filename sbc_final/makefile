all: main

test: main run

main: main.o lista_encadeada.o map.o lcd.o
	gcc -o main main.o lista_encadeada.o map.o lcd.o -lpaho-mqtt3c -lwiringPi -lwiringPiDev -pthread -Wall 

main.o: main.c lista_encadeada.h
	gcc -o main.o main.c -c -Wall 

lista_encadeada.o: lista_encadeada.c lista_encadeada.h
	gcc -o lista_encadeada.o lista_encadeada.c -c -Wall 

lcd.o:lcd.s
	as -g -o lcd.o lcd.s
  
map.o:map.s
	as -g -o map.o map.s

clean:
	rm -rf *.o *~ main

run:
	sudo ./main