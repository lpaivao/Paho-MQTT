# Paho-MQTT
TEC499 - Problema 3

- [Sérgio Pugliesi](github.com/ShinJaca)
- [Lucas de Paiva](github.com/lpaivao)
- [Adriel Oliveira](github.com/Pegasus77-Adriel)

## IHM local

### Máquina de estados
A interface homem-máquina implementa através de uma máquina de estados menus que servem para a navegação do usuário por dois botões e um DIP switch. Os tópicos a seguir mostram essa relação e como mudar os estados da máquina.

- Definição de entradas ----------------------------------------------------------------------------------- -----------------------------------------------------------------------------------
<div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/funcionalidades.png"/><br>
		<p>
		Entradas IHM local
		</p>
	</div>
  
 - Menu principal ----------------------------------------------------------------------------------- -----------------------------------------------------------------------------------
 <div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/menus.png"/><br>
		<p>
		Menu principal LCD
		</p>
	</div> 
 
- Menu para histórico ----------------------------------------------------------------------------------- -----------------------------------------------------------------------------------
 
 <div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/menu historico.png"/><br>
		<p>
		Entradas IHM local
		</p>
	</div>
 
   <div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/estados_botoes_historico.png"/><br>
		<p>
		Diagrama de estados do histórico
		</p>
	</div>
- Menu de solicitações ao Node ----------------------------------------------------------------------------------- -----------------------------------------------------------------------------------
 <div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/menu solicitacoes.png"/><br>
		<p>
		Entradas IHM local
		</p>
	</div>
 
  <div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/estados_botoes_solicitacao.png"/><br>
		<p>
		Diagrama de estados das solicitações
		</p>
	</div>
 
 
- Menu de confiurações de tempo ----------------------------------------------------------------------------------- -----------------------------------------------------------------------------------
 <div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/menu tempo.png"/><br>
		<p>
		Entradas IHM local
		</p>
	</div>
 
  <div id="image11" style="display: inline_block" align="center">
		<img src="/modelo/Diagrama de estados/estados_botoes_time.png"/><br>
		<p>
		Diagrama de estados de configuração de tempo
		</p>
	</div>
	
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

## Como compilar e rodar os programas


### IHM remoto (pasta ihm_remoto)
 ````console
make
sudo ./remote
````
### IHM Local (escolher uma das pastas)

- Para pasta sbc_final
 ````console
make
sudo ./main
````
- Para pasta sbc_auto
 ````console
make
sudo ./main
````
### Node (pasta nodeMCU)
- Enviar código para a ESP8266 através do ArduinoIDE
