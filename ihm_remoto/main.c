#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MQTTClient.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

// Arquivo com as definições de tópicos
#include "../topicos.h"

#include "../nodeMCU/commands.h"

#define IP "test.mosquitto.org:1883" // mudar para "tcp://10.0.0.101:1883"

#define CLIENTID "ihmremoto001"
#define USER "aluno"
#define PASSWORD "@luno*123"

#define QOS 1

// Variaveis para configurar mqtt
MQTTClient client; // cliente
int rc;

void publish(MQTTClient client, char *topic, char *payload);

volatile MQTTClient_deliveryToken deliveredtoken;

char delayTime = 5;

char menuOp = '0';
volatile int threadExit = 0;

// Confirmação da mensagem
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("[ ACK ] tkn: %d\n", dt);
    deliveredtoken = dt;
}

// Manipulação dos históricos
char histAnalog[10], histDigital[2][10];

/**
 * Trata a os dados de uma mensagem de histórico de sensor analógico
 */
void handleHistAnalog(char *payload, char *topic)
{
    for (int i = 1; i < 11; i++)
    {
        histAnalog[i - 1] = payload[i];
    }
}
void handleHistDigital(char *payload, char *topic)
{
    for (int i = 1; i < 11; i++)
    {
        histDigital[topic[5] - '0'][i - 1] = payload[i];
    }
}

void handleHist(char *payload, char *historico)
{
    for (int i = 1; i < 11; i++)
    {
        historico[i - 1] = payload[i];
    }
}

// Manipulação de leituras individuais
int a0 = 0, d[2] = {0, 0};

void updateAnalog(char *payload, char *topic)
{
    a0 = payload[1];
}
void updateDigital(char *payload, char *topic)
{
    d[topic[6] - '0'] = payload[1];
}

// Retorno da mensagem de publicação, utilizada pela função MQTTClient_setCallbacks();
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char *payloadptr;
    payloadptr = message->payload;

    switch (payloadptr[0])
    {
    case ANALOG_READ:
        updateAnalog(payloadptr, topicName);
        break;
    case DIGITAL_READ:
        updateDigital(payloadptr, topicName);
        break;
    case RESP_HIST_ANALOG:
        // handleHistAnalog(payloadptr, topicName);
        handleHist(payloadptr, histAnalog);
        break;
    case RESP_HIST_DIGITAL:
        handleHistDigital(payloadptr, topicName);
        // handleHist(payloadptr, histDigital);
        break;

    default:
        break;
    }

    printf("[ %s ] size: %d msg: ", topicName, topicLen);
    for (i = 0; i < message->payloadlen; i++)
    {
        printf("%d ", *payloadptr++);
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

// Configuração de conexão do mqtt e criação do client
void mqtt_config()
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer; // initializer for connect options
    conn_opts.keepAliveInterval = 20;                                            // intervalo do KA
    conn_opts.cleansession = 1;
    // conn_opts.username = USER;     // User
    // conn_opts.password = PASSWORD; // Senha

    MQTTClient_create(&client, IP, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL); // Cria o cliente para se conectar ao broker
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    rc = MQTTClient_connect(client, &conn_opts);

    if (rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

/**
 * Funções de menu
 */

/**
 * Imprime os itens de menu principal
 */
void printMainMenu()
{
    printf("Menu Principal\n");
    printf("\t1: Dados atuais\n");
    printf("\t2: Histórico de leituras\n");
}

/**
 * Imprime as oções do menu de dados atualizados automaticamente (1)
 */
void printDataAuto()
{
    printf("Leituras Automaticas: ( pressione x para voltar ao menu anterior )\n");
}

/**
 * Execução de opções de menu
 */

void *inputAssync()
{
    menuOp = getchar();
    getchar();
}

int ri, rl;
pthread_t thrinput, trefreshdata;

void *refreshdata()
{
    while (menuOp != 'x')
    {
        printf("D0: %d\tD1: %d\tA0: %d\n", d[0], d[1], a0);
        sleep(1);
    }
}

void showData()
{
    ri = pthread_create(&trefreshdata, NULL, refreshdata, NULL);
    ri = pthread_create(&thrinput, NULL, inputAssync, NULL);
    pthread_join(trefreshdata, NULL);
    pthread_join(thrinput, NULL);
}

void plotHist()
{
    FILE *gnupl = popen("gnuplot -persistent", "w");
    FILE *hisd0 = fopen("d0.dat", "w");
    FILE *hisd1 = fopen("d1.dat", "w");
    fprintf(gnupl, "set style data steps\n");
    fprintf(gnupl, "set multiplot layout 2, 1 title 'Histórico de medições digitais' font ',14'\n");

    for (int i = 0; i < 10; i++)
    {
        fprintf(hisd0, "%d \n", histDigital[0][i]);
        fprintf(hisd1, "%d \n", histDigital[1][i]);
    }
    fflush(hisd0);
    fflush(hisd1);

    fprintf(gnupl, "set title 'D0'\n");
    fprintf(gnupl, "%s \n", "plot [-1:11] [-1:2] 'd0.dat'");
    fprintf(gnupl, "set title 'D1'\n");
    fprintf(gnupl, "%s \n", "plot [-1:11] [-1:2] 'd1.dat'");
    fprintf(gnupl, "%s \n", "unset multiplot");
    fflush(gnupl);
}

void reqHist()
{
    char cmd[] = {REQ_HIST_ANALOG, 0x0};
    publish(client, SBC_CONFIG_TIME_TOPIC, cmd);
    cmd[0] = REQ_HIST_DIGITAL;
    cmd[1] = 0x0;
    publish(client, SBC_CONFIG_TIME_TOPIC, cmd);
    cmd[1] = 0x1;
    publish(client, SBC_CONFIG_TIME_TOPIC, cmd);
}

void requestHistorico()
{
    printf("Menu de historicos:\n1:Atualizar Históricos\n2:Exibe imprime valores\n3:Grafico dos valores\nx:Retorna ao menu principal\n");
    switch (getchar())
    {
    case '1':
        reqHist();
        break;
    case '2':

        break;
    case '3':
        plotHist();
    default:
        break;
    }
    getchar();
}

int main(int argc, char *argv[])
{
    printf("IHM Remoto\n");
    printf("[    ] MQTT Startup...");

    mqtt_config();

    printf("\r[ OK\n");

    printf("[    ] MQTT Subscriptions...");
    MQTTClient_subscribe(client, SENSOR_A0_TOPIC, QOS);
    MQTTClient_subscribe(client, SENSOR_D0_TOPIC, QOS);
    MQTTClient_subscribe(client, SENSOR_D1_TOPIC, QOS);
    MQTTClient_subscribe(client, SBC_SENSOR_A0_HIST, QOS);
    MQTTClient_subscribe(client, SBC_SENSOR_D0_HIST, QOS);
    MQTTClient_subscribe(client, SBC_SENSOR_D1_HIST, QOS);
    printf("\r[ OK\n");

    while (menuOp != 'q')
    {
        printMainMenu();
        menuOp = getchar();
        getchar();
        switch (menuOp)
        {
        case '1':
            showData();
            break;
        case '2':
            requestHistorico();
            break;
        default:
            break;
        }
    };

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
