BASEGCC=gcc -o $@ $< -lpaho-mqtt3c -lpaho-mqtt3as -Wall

sub: mqtt_sub.c
	$(BASEGCC)

pub: mqtt_pub.c
	$(BASEGCC)
