# Paho-MQTT
TEC499 - Problema 3

- [Sérgio Pugliesi](github.com/ShinJaca)
- [Lucas de Paiva](github.com/lpaivao)
- [Adriel Oliveira](github.com/Pegasus77-Adriel)
## Dependências de bibliotecas

### WiringPi
- Documentação: https://github.com/WiringPi/WiringPi
- Como instalar: https://github.com/WiringPi/WiringPi/blob/master/INSTALL
- Relação de pinagem: - https://learn.sparkfun.com/tutorials/raspberry-gpio/gpio-pinout
### Paho MQTT
- Documentação: https://github.com/eclipse/paho.mqtt.c
- Como instalar:
 ````console
make
sudo make install
````

## Como rodar os programas


### IHM remoto (pasta ihm_remoto)
 ````console
make
sudo ./remote
````
### IHM Local (escolher uma das pastas)

#### Para pasta sbc_final
 ````console
make
sudo ./main
````
#### Para pasta sbc_auto
 ````console
make
sudo ./main
````
### Node (pasta nodeMCU)
- Enviar código para a ESP8266 através do ArduinoIDE
