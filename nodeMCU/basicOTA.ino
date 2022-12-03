#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <gpio.h>
#include <uart.h>

#include "commands.h"

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

const char *ssid = STASSID;
const char *password = STAPSK;

// Nome do ESP na rede
const char *host = "ESP-10.0.0.109";

// Definições de rede
IPAddress local_IP(10, 0, 0, 109);
IPAddress gateway(10, 0, 0, 1);
IPAddress subnet(255, 255, 0, 0);

bool lstate = LOW;

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
    // setDigitalSensor,
};
sensor DS1 = {
    DIGITAL,
    readDigitalSensor,
    0,
    D1,
    // setDigitalSensor,
};
sensor AS0 = {
    ANALOG,
    readAnalogSensor,
    0,
    A0,
    // setDigitalSensor,
};

void ota_startup()
{
  // Configuração do IP fixo no roteador, se não conectado, imprime mensagem de falha
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    ets_uart_printf("STA Failed to configure");
  }
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
#ifdef __TESTING__
  *addr = *addr - '0';
#endif
  if (*addr >= limit)
  {
    return false;
  }
  return true;
}

void setup()
{
  uart0 = uart_init(UART0, BAUDUART0, UART_8N1, 0, 1, 10, 0);
  uart_write(uart0, "\nBooting\r\n", 6);
#ifdef __OTA__
  ota_startup();
#endif
  setupSensorMaps();
  uart_write(uart0, "\nReady\r\n", 6);
  // pinMode(14, OUTPUT);
}

char *recByte = (char *)malloc(sizeof(char) * 2);
int addr = 0;

void loop()
{
#ifdef __OTA__
  ArduinoOTA.handle();
#endif
  while ((int)uart_rx_available(uart0) >= 2)
  {
    if (uart_read(uart0, recByte, 2) == 2)
    {
      addr = recByte[1];
#ifdef __TESTING__
      addr = addr - '0';
#endif
      switch (recByte[0])
      {
      case NODE_STATUS:
        uart_write_char(uart0, NODE_NORMAL);
        break;
      case READ_ANALOG:
        if (addr >= analog_sensors.installed)
        {
          break;
        }
        uart_write_char(uart0, ANALOG_READ);
        uart_write_char(uart0, analog_sensors.sensors[addr]->read(A0));
        break;
      case READ_DIGITAL:
        if (addr >= digital_sensors.installed)
        {
          break;
        }
        uart_write_char(uart0, DIGITAL_READ);
#ifdef __TESTING__
        uart_write_char(uart0, digital_sensors.sensors[addr]->read(digital_sensors.sensors[addr]->pin) + '0');
#else
        uart_write_char(uart0, digital_sensors.sensors[addr]->read(digital_sensors.sensors[addr]->pin));
#endif
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
#ifdef __TESTING__
        ets_uart_printf("[ NONE ] Skipping ...");
#else
        uart_write_char(uart0, NODE_SKIP);
        uart_write_char(uart0, NODE_SKIP);
#endif
        break;
      }
#ifdef __TESTING__
      uart_write_char(uart0, '\n');
#endif
    }
  }
}
