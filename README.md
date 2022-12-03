# Paho-MQTT
TEC499 - Problema 3


## Para pasta paho_test_fix
````console
make
./main
````

## Para pasta paho_test
- Para publisher
````console
gcc mqtt_pub.c -o pub -lpaho-mqtt3c -Wall
./pub
````

- Para subscriber
````console
gcc mqtt_sub.c -o sub -lpaho-mqtt3c -Wall
./sub
````
