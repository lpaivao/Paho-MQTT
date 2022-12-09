#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MQTTClient.h>
#include <unistd.h>
#include <stdbool.h>

// Arquivo com as definições de tópicos
#include "../topicos.h"

#include "../nodeMCU/commands.h"

#define IP "test.mosquitto.org:1883" // mudar para "tcp://10.0.0.101:1883"

#define CLIENTID "MQTTCClientID"
#define USER "aluno"
#define PASSWORD "@luno*123"

#define QOS 1

// Variaveis para configurar mqtt
MQTTClient client; // cliente
int rc;

void publish(MQTTClient client, char *topic, char *payload);

volatile MQTTClient_deliveryToken deliveredtoken;

char delayTime = 5;

// Confirmação da mensagem
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

// Retorno da mensagem de publicação, utilizada pela função MQTTClient_setCallbacks();
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char *payloadptr;
    printf("[ ARV ] topic: %s msg: ", topicName);
    payloadptr = message->payload;

    switch (payloadptr[0])
    {
    case SET_NEW_TIME:
        delayTime = payloadptr[1];
        break;

    default:
        break;
    }

    for (i = 0; i < message->payloadlen; i++)
    {
        printf("0x%02x ", *payloadptr++);
    }
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

// Mensagem para o caso de conexão perdida
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

// Função que publica mensagens num tópico
void publish(MQTTClient client, char *topic, char *payload)
{
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    pubmsg.payload = payload;
    pubmsg.payloadlen = 2;
    // pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
}

// ------------------------------------------------ COMEÇA AQUI ------------------------------------------------ //

// Faz subscribe nos tópicos necessários no início do programa
void start_subscribe_topics(MQTTClient client)
{
    MQTTClient_subscribe(client, SBC_CONFIG_TIME_TOPIC, QOS);
    // MQTTClient_subscribe(client, SENSOR_A0_TOPIC, QOS);
    // MQTTClient_subscribe(client, SENSOR_D0_TOPIC, QOS);
    // MQTTClient_subscribe(client, SENSOR_D1_TOPIC, QOS);
}

// Configuração de conexão do mqtt e criação do client
void mqtt_config()
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer; // initializer for connect options
    conn_opts.keepAliveInterval = 20;                                            // intervalo do KA
    conn_opts.cleansession = 1;
    conn_opts.username = USER;     // User
    conn_opts.password = PASSWORD; // Senha

    MQTTClient_create(&client, IP, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL); // Cria o cliente para se conectar ao broker
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    rc = MQTTClient_connect(client, &conn_opts);

    if (rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    mqtt_config();
    MQTTClient_subscribe(client, SBC_CONFIG_TIME_TOPIC, QOS);
    char readA0[] = {READ_ANALOG, 0x0};
    char readD0[] = {READ_DIGITAL, 0x0};
    char readD1[] = {READ_DIGITAL, 0x1};
    while (true)
    {
        // Request data
        publish(client, COMMAND_TO_ESP_TOPIC, readA0);
        publish(client, COMMAND_TO_ESP_TOPIC, readD0);
        publish(client, COMMAND_TO_ESP_TOPIC, readD1);

        // Delay time is set by message arrive callback
        sleep(delayTime);
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
