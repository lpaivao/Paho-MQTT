#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MQTTClient.h>
#include <unistd.h>
#include <stdbool.h>

#define IP "test.mosquitto.org:1883" // mudar para "tcp://10.0.0.101:1883"

#define CLIENTID "MQTTCClientID"
#define USER "aluno"
#define PASSWORD "@luno*123"

#define QOS 1

#define SUBSCRIBE_TEST_TOPIC "TP01/G01/mqtt/sub"

#define SENSOR_D0_TOPIC "esp0/D0"
#define SENSOR_D1_TOPIC "esp0/D1"
#define SENSOR_A0_TOPIC "esp0/A0"

#define COMMAND_TO_ESP_TOPIC "esp0/cmd"

#define SBC_CONFIG_TIME_TOPIC "sbc/cfg"

// COMANDOS DE REQUISIÇÃO
#define SITUACAO_ATUAL_NODE 0x03
#define SOLICITA_ENTRADA_ANALOGICA 0x04
#define SOLICITA_ENTRADA_DIGITAL 0x05
#define TOGGLE_LED 0x06
// COMANDOS DE RESPOSTA
#define NODE_COM_PROBLEMA 0x1F
#define NODE_FUNCIONANDO 0x00
#define MEDIDA_ENTRADA_ANALOGICA 0x01
#define ESTADO_ENTRADA_DIGITAL 0x02

// Variaveis para configurar mqtt
MQTTClient client; // cliente
int rc;

void publish(MQTTClient client, char *topic, char *payload);

volatile MQTTClient_deliveryToken deliveredtoken;

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
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = message->payload;
    for (i = 0; i < message->payloadlen; i++)
    {
        putchar(*payloadptr++);
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
    pubmsg.payloadlen = strlen(pubmsg.payload);
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
    MQTTClient_subscribe(client, SENSOR_A0_TOPIC, QOS);
    MQTTClient_subscribe(client, SENSOR_D0_TOPIC, QOS);
    MQTTClient_subscribe(client, SENSOR_D1_TOPIC, QOS);
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

void menu_comandos()
{
    int opcao;

    do
    {
        char dado[2];
        printf("1 - Situacao do node\n");
        printf("2 - Sensor A0\n");
        printf("3 - Sensor D0\n");
        printf("4 - Sensor D1\n");
        printf("5 - Led Toggle\n");
        printf("6 - Sair\n");

        scanf("%d", &opcao);
        system("clear");

        switch (opcao)
        {
        case 1:
            dado[0] = SITUACAO_ATUAL_NODE;
            dado[1] = 0; // Qualquer dado para esse byte
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 2:
            dado[0] = SOLICITA_ENTRADA_ANALOGICA;
            dado[1] = 0;
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 3:
            dado[0] = SOLICITA_ENTRADA_DIGITAL;
            dado[1] = 0;
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 4:
            dado[0] = SOLICITA_ENTRADA_DIGITAL;
            dado[1] = 1;
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 5:
            dado[0] = TOGGLE_LED;
            dado[1] = 0; // Qualquer dado para esse byte
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 6:
            break;
        }
    } while (opcao != 6);
}

void menu()
{

    int opcao;
    printf("---------Menu---------\n");
    printf("1 - Menu Comandos\n");
    scanf("%d", &opcao);
    system("clear");

    switch (opcao)
    {
    case 1:
        menu_comandos();
        break;
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    mqtt_config();
    // start_subscribe_topics(client);
    while (true)
    {
        menu();
    }

    // printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", SUBSCRIBE_TEST_TOPIC, CLIENTID, QOS);
    // MQTTClient_subscribe(client, SUBSCRIBE_TEST_TOPIC, QOS);
    // publish(client, SUBSCRIBE_TEST_TOPIC, "Publish test");

    /*int contador = 0;
    do
    {
        char string_msg[50];
        snprintf(string_msg, 50, "%s %d", "MENSAGEM DE TESTE ", contador);
        contador++;
        publish(client, SUBSCRIBE_TEST_TOPIC, string_msg);
        usleep(2000000); // delay 2 sec
    } while (true); */

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
