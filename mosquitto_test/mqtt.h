#ifndef _mqtt_h
#define _mqtt_h
#include <mosquitto.h>

// Definicoes de topicos e host do broker utilizado
#define Host_broker "10.0.0.101"
#define TOPIC_D0 "sensor/digital/D0"
#define TOPIC_D1 "sensor/digital/D1"
#define TOPIC_A0 "sensor/digital/A0"

// Struct para cliente ouvinte mqtt
typedef struct Cliente
{
    char Nome[255];
    char Host[255];
    char Topico[255];
} Cliente;

// Struct para o cliente editor mqtt
typedef struct Publisher
{
    char Nome[255];
    char Host[255];
    char Topico[255];
    char Msg[300];
} Publisher;

// Prototipos de funcao
void publicar(Publisher pub);
void create_client(Cliente client);
void on_connect(struct mosquitto *mosq, void *obj, int rc);
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

#endif