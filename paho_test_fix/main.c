#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MQTTClient.h>
#include <unistd.h>
#include <stdbool.h>

#define IP   "tcp://localhost:1883" //mudar para "tcp://10.0.0.101:1883"

#define CLIENTID       "MQTTCClientID"  
#define USER           "aluno"
#define PASSWORD       "@luno*123"

#define QOS 1

#define SUBSCRIBE_TEST_TOPIC   "TP01/G01/mqtt/sub"

MQTTClient client;

void publish(MQTTClient client, char* topic, char* payload);

volatile MQTTClient_deliveryToken deliveredtoken;
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void publish(MQTTClient client, char* topic, char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;

    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, 1000L);
}

int main(int argc, char *argv[])
{
   int rc;
   //int ch;
   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer; //initializer for connect options
   conn_opts.keepAliveInterval = 20; //intervalo do KA
   conn_opts.cleansession = 1;
   conn_opts.username = USER;       // User
   conn_opts.password = PASSWORD;   // Senha

   MQTTClient_create(&client, IP, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);  // Cria o cliente para se conectar ao broker
   MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

   rc = MQTTClient_connect(client, &conn_opts);

   if (rc != MQTTCLIENT_SUCCESS)
   {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
   }else{
   	printf("Teste de conexao passou\n");
 }

    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", SUBSCRIBE_TEST_TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, SUBSCRIBE_TEST_TOPIC, QOS);

    int contador = 0;
    do
    {
        char string_msg[50];
        snprintf(string_msg, 50, "%s %d", "MENSAGEM DE TESTE ", contador);
        contador++;
        publish(client, SUBSCRIBE_TEST_TOPIC, string_msg);
        usleep(2000000); //delay 2 sec
    } while(true);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
