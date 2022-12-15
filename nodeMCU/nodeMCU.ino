#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <gpio.h>
#include <uart.h>
#include <PubSubClient.h>
#include "commands.h"
#include "topicos.h"
#include <stdlib.h>

#define __OTA__

#ifndef STASSID
#define STASSID "INTELBRAS"
#define STAPSK "Pbl-Sistemas-Digitais"
#endif

#define BAUDUART0 115200
#define BAUDUART1 115200

#ifdef __TESTING__
#define LED_PIN 14
#else
#define LED_PIN LED_BUILTIN
#endif

#define USER "aluno"
#define PASSWORD "@luno*123"


#ifdef LABMODE
// Wifi e Broker configurações
const char *ssid = STASSID;
const char *password = STAPSK;

// const char *mqtt_server = "10.0.0.101";
#else
const char *mqtt_server = "test.mosquitto.org";
#endif
int s = 0;

const char *device_id = "esp0109";

// Define Client
WiFiClient espClient;
PubSubClient client(espClient);

// Nome do ESP na rede
const char *host = "ESP-10.0.0.110";

// Definições de rede
IPAddress local_IP(10, 0, 0, 110);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 0, 0);

bool lstate = LOW;
char *recByte = (char *)malloc(sizeof(char) * 2);
int addr = 0;

enum sensor_type
{
  DIGITAL,
  ANALOG
};

// Estrutura contendo as referencias para uso de um sensor, tipo, funcão de leitura e configuração
typedef struct
{
  enum sensor_type type;
  int (*read)(int), last_read, pin;
  const char *topic;
  // bool (*set)(int);
} sensor;

// Estrutura de mapeamento de sensores
typedef struct
{
  int total = 16, installed;
  enum sensor_type type;
  sensor *sensors[16];
} sensor_map;

uart_t *uart0;

sensor_map digital_sensors, analog_sensors;

int readDigitalSensor(int pin)
{
  return GPIO_INPUT_GET(pin);
}

int readAnalogSensor(int pin)
{
  return map(system_adc_read(), 0, 1023, 0, 254);
}

sensor DS0 = {
    DIGITAL,
    readDigitalSensor,
    0,
    D0,
    SENSOR_D0_TOPIC
    // setDigitalSensor,
};
sensor DS1 = {
    DIGITAL,
    readDigitalSensor,
    0,
    D1,
    SENSOR_D1_TOPIC
    // setDigitalSensor,
};
sensor AS0 = {
    ANALOG,
    readAnalogSensor,
    0,
    A0,
    SENSOR_A0_TOPIC
    // setDigitalSensor,
};
char msg[2];

void mainSwitch(char *recByte)
{
  addr = recByte[1];

  switch (recByte[0])
  {
  case NODE_STATUS:
    uart_write_char(uart0, NODE_NORMAL);
    msg[0] = NODE_NORMAL;
    msg[1] = NODE_NORMAL;
    client.publish(STATUS_NODEMCU, msg);

    break;
  case READ_ANALOG:
    if (addr >= analog_sensors.installed)
    {
      break;
    }
    // Montagem da mensagem de resposta a ser enviada
    msg[0] = ANALOG_READ;
    msg[1] = analog_sensors.sensors[addr]->read(A0);

    // Mensagem via UART
    uart_write_char(uart0, msg[0]);
    uart_write_char(uart0, msg[1]);

    // Mensagem via MQTT
    client.publish(analog_sensors.sensors[addr]->topic, msg);

    break;
  case READ_DIGITAL:
    if (addr >= digital_sensors.installed)
    {
      break;
    }

    msg[0] = DIGITAL_READ;
    msg[1] = digital_sensors.sensors[addr]->read(digital_sensors.sensors[addr]->pin);

    uart_write_char(uart0, msg[0]);
    uart_write_char(uart0, msg[1]);

    client.publish(digital_sensors.sensors[addr]->topic, msg);

    break;
  case LED_TOGGLE:
    if (addr >= 2)
    {
      break;
    }
    GPIO_OUTPUT_SET(LED_PIN, addr);
    uart_write_char(uart0, NODE_NORMAL);
    break;
  case '\r':
  case '\n':
    break;
  default:

    uart_write_char(uart0, NODE_SKIP);

    break;
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
#ifndef LABMODE
  uart_write(uart0, "Message arrived [", 17);
  uart_write(uart0, topic, 5);
  uart_write(uart0, "] ", 2);
  for (int i = 0; i < length; i++)
  {
    uart_write_char(uart0, (char)payload[i]);
  }
#endif
  // Serial.println();
  mainSwitch((char *)payload);
}

void reconnect()
{
  // Conectar ao MQTT caso não esteja conectado
  while (!client.connected())
  {
#ifdef LABMODE
    client.connect("esp0109", USER, PASSWORD);
#else
    client.connect("esp0109");
#endif
  }
}
void ota_startup()
{
  // Configuração do IP fixo no roteador, se não conectado, imprime mensagem de falha
  // if (!WiFi.config(local_IP, gateway, subnet)) {
  //   ets_uart_printf("STA Failed to configure");
  // }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    ets_uart_printf("Connection Failed! Rebooting...\n");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]()
                     {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    ets_uart_printf("Start updating %s\n", type); });
  ArduinoOTA.onEnd([]()
                   { ets_uart_printf("\nEnd\n"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { ets_uart_printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    ets_uart_printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      ets_uart_printf("Auth Failed\n");
    } else if (error == OTA_BEGIN_ERROR) {
      ets_uart_printf("Begin Failed\n");
    } else if (error == OTA_CONNECT_ERROR) {
      ets_uart_printf("Connect Failed\n");
    } else if (error == OTA_RECEIVE_ERROR) {
      ets_uart_printf("Receive Failed\n");
    } else if (error == OTA_END_ERROR) {
      ets_uart_printf("End Failed\n");
    } });
  ArduinoOTA.begin();
  ets_uart_printf("Ready\n");
  ets_uart_printf("IP address: %s\n", WiFi.localIP());
}

void setupSensorMaps()
{
  digital_sensors.type = DIGITAL;
  digital_sensors.installed = 2;
  digital_sensors.sensors[0] = &DS0;
  digital_sensors.sensors[1] = &DS1;
  for (int i = 2; i < digital_sensors.total; i++)
  {
    digital_sensors.sensors[i] = &DS0;
  }
  analog_sensors.type = ANALOG;
  analog_sensors.installed = 1;
  for (int i = 0; i < analog_sensors.total; i++)
  {
    analog_sensors.sensors[i] = &AS0;
  }
}

bool evalAddr(int *addr, int limit)
{
  if (*addr >= limit)
  {
    return false;
  }
  return true;
}

void setup()
{
  uart0 = uart_init(UART0, BAUDUART0, UART_8N1, 0, 1, 10, 0);
  uart_write(uart0, "\nBooting\n", 6);
#ifdef __OTA__
  ota_startup();
#endif
  setupSensorMaps();
  uart_write(uart0, "\nReady\n", 6);
  // pinMode(14, OUTPUT);

  client.setServer(mqtt_server, 1883); // change port number as mentioned in your cloudmqtt console
  client.setCallback(callback);

  uart_write(uart0, "Conexão MQTT\n", 13);
  // if (client.connect("esp0109", USER, PASSWORD))
  if (client.connect("esp0109"))
    uart_write(uart0, "MQTT OK\n", 8);

  client.subscribe(COMMAND_TO_ESP_TOPIC);
  char msg[] = {ANALOG_READ, 56};
  client.publish(SENSOR_A0_TOPIC, msg);

  char *recByte = (char *)malloc(sizeof(char) * 2);
  int addr = 0;
}

void loop()
{

  // Verifica se está conectado ao broker
  // if (!client.connected()) {
  //   reconnect();
  // }
  client.loop();

#ifdef __OTA__
  ArduinoOTA.handle();
#endif
  while ((int)uart_rx_available(uart0) >= 2)
  {
    if (uart_read(uart0, recByte, 2) == 2)
    {
      mainSwitch(recByte);
    }
  }
}