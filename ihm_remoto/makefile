BASEGCC=gcc -o $@ $<
LIBS=-lpaho-mqtt3c -lpaho-mqtt3as -pthread


all: remote

remote: main.c
	$(BASEGCC) $(LIBS)