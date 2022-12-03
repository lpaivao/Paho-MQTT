SERIAL_PORT=/dev/ttyAMA0
FQBN=esp8266:esp8266:nodemcuv2


compile: basicOTA.ino
	arduino-cli compile -v -b $(FQBN) $<

upload: basicOTA.ino
	arduino-cli upload -v -p $(SERIAL_PORT) -b $(FQBN) $<

all: compile upload