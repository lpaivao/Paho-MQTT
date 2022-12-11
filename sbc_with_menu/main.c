#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MQTTClient.h>
#include <unistd.h>
#include <stdbool.h>
#include <wiringPi.h>
#include <pthread.h>
/*Biblioteca para a IHM local*/
#include "ihm_local.h"
/*Arquivo com as definições de tópicos*/
#include "../topicos.h"
#include "../nodeMCU/commands.h"
/*Biblioteca da lista encadeada para armazenamento do historico*/
#include "lista_encadeada.h"

/*Constantes para confiugração do MQTT*/
#define IP "tcp://test.mosquitto.org:1883" /*mudar para "tcp://10.0.0.101:1883"*/
#define CLIENTID "MQTTCClientID"
#define USER "aluno"
#define PASSWORD "@luno*123"
#define QOS 1

/* Funções da Biblioteca do LCD*/
/*
extern void memory_map(void);
extern void init_lcd(void);
extern void clear_lcd(void);
extern void write_char(unsigned char c);*/

/*Inicialização do LCD
void lcd()
{
    memory_map();
    init_lcd();
    clear_lcd();
}*/

/*Mostrar uma string no LCD
void print_lcd(unsigned char c[])
{
    int len = strlen(c);

    for (int i = 0; i < len; i++)
    {
        write_char(c[i]);
    }
}*/

/*Variaveis Globais*/
/*Variaveis para configurar mqtt*/
MQTTClient client;
int rc;
/*Variaveis de listas encadeadas para armazenar historico*/
Lista *list_historic_A0;
Lista *list_historic_D0;
Lista *list_historic_D1;

/*Delay*/
char delayTime = 5;

/*Declaracao de funcoes*/
void publish(MQTTClient client, char *topic, char *payload);
void *IHM_Local(void *arg);

volatile MQTTClient_deliveryToken deliveredtoken;

/*Confirmação da mensagem*/
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("[ ACK ] tkn: %d\n", dt);
    deliveredtoken = dt;
}

/*Verifica se a lista possui mais de 10 medicoes no historico*/
bool list_full(Lista *list)
{
    if (list->tam >= 10)
    {
        return true;
    }
    return false;
}

/*Verifica se o historico possui mais de 10 medicoes e adiciona e remove adequadamente*/
void add_medicao_historico(Lista *list, char payloadptr)
{
    char caractere = payloadptr + '0';
    if (!list_full(list))
    {
        /*adiciona medicao mais nova no inicio do historico (primeira da lista)*/
        inserirInicio(list, caractere);
    }
    else
    {
        /*remove medicao mais antiga do historico (ultima da lista)*/
        remover(list, list->fim->valor);
        /*adiciona medicao mais nova no inicio do historico (primeira da lista)*/
        inserirInicio(list, caractere);
    }
}

/*Tratamento da notificação para o sbc pegar os dados dos tópicos que está inscrito pelo primeiro byte do payload, utilizada pela função MQTTClient_setCallbacks();*/
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    char *payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;

    puts(payloadptr);

    switch (payloadptr[0])
    {
    case SET_NEW_TIME:
        delayTime = payloadptr[1];
        printf("[ %s ] new time: %d\n", SBC_CONFIG_TIME_TOPIC, delayTime);
        break;

    case ANALOG_READ:
        add_medicao_historico(list_historic_A0, payloadptr[1]);
        break;

    case DIGITAL_READ:
        if (strcmp(topicName, SENSOR_D0_TOPIC) == 0)
            add_medicao_historico(list_historic_D0, payloadptr[1]);
        else if (strcmp(topicName, SENSOR_D1_TOPIC) == 0)
            add_medicao_historico(list_historic_D1, payloadptr[1]);
        break;

    case NODE_NORMAL:
        printf("Node normal\n");
        // print_lcd("Node Normal");
        break;

    case NODE_SKIP:
        printf("Node skip\n");
        // print_lcd("Node Skip");
        break;

    case NODE_TROUBLE:
        printf("Node trouble\n");
        // print_lcd("Node Trouble");
        break;

    default:
        break;
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

/*Mensagem para o caso de conexão perdida*/
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

/*Função que publica mensagens num tópico*/
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

/*------------------------------------------------ COMEÇA AQUI ------------------------------------------------ */

/*Faz subscribe nos tópicos necessários no início do programa*/
void start_subscribe_topics(MQTTClient client)
{
    MQTTClient_subscribe(client, SBC_CONFIG_TIME_TOPIC, QOS);
    MQTTClient_subscribe(client, SENSOR_A0_TOPIC, QOS);
    MQTTClient_subscribe(client, SENSOR_D0_TOPIC, QOS);
    MQTTClient_subscribe(client, SENSOR_D1_TOPIC, QOS);
}

/*Configuração de conexão do mqtt e criação do client*/
void mqtt_config()
{
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USER;
    conn_opts.password = PASSWORD;

    MQTTClient_create(&client, IP, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    rc = MQTTClient_connect(client, &conn_opts);

    if (rc != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

    printf("MQTT Configurado\n");
}

/*Submenu de comandos*/
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
            dado[0] = NODE_STATUS;
            dado[1] = 0; /*Qualquer dado para esse byte*/
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 2:
            dado[0] = READ_ANALOG;
            dado[1] = 0;
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 3:
            dado[0] = READ_DIGITAL;
            dado[1] = 0;
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 4:
            dado[0] = READ_DIGITAL;
            dado[1] = 1;
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 5:
            dado[0] = LED_TOGGLE;
            dado[1] = 0; /*Qualquer dado para esse byte*/
            publish(client, COMMAND_TO_ESP_TOPIC, dado);
            break;
        case 6:
            break;
        }
    } while (opcao != 6);
}

/*Thread Menu Principal*/
void *menu(void *arg)
{
    int opcao = 0;
    do
    {
        printf("---------Menu---------\n");
        printf("0 - Sair\n");
        printf("1 - Menu Comandos\n");
        scanf("%d", &opcao);
        system("clear");

        switch (opcao)
        {
        case 0:
            break;
        case 1:
            menu_comandos();
            break;
        default:
            break;
        }
    } while (opcao != 0);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // lcd();           /*Primeira configuração do LCD*/

    mqtt_config();   /*Configuração MQTT*/
    wiringPiSetup(); /*Configuracao do wiringPi*/

    /*start_subscribe_topics(client);*/

    pthread_t threadMenu, threadIHM;
    pthread_create(&threadMenu, NULL, menu, NULL);     // Criacao da thread para o menu
    pthread_create(&threadIHM, NULL, IHM_Local, NULL); // Criacao da thread para o IHM Local (automatico)

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

/*------------------------------------------------- TUDO RELACIONADO AO IHM LOCAL ABAIXO -------------------------------------------------*/

/*Compara a combinação das chaves 3 e 4 atual com a antiga e verifica se o estado mudou*/
bool state_chaged(int estadoAntigo, int estadoAtual)
{
    if (estadoAntigo != estadoAtual)
    {
        return false;
    }
    return true;
}

/*Verifica o estado do DIP e retorna o inteiro em decimal referente ao binário da combinação das chaves 3 e 4
OBS: os botões possuem logica inversa!!*/
int get_state_dp(int dp3, int dp4)
{
    dp3 = digitalRead(DP3);
    dp4 = digitalRead(DP4);
    if (dp3 == 1 && dp4 == 1)
        return 0;
    else if (dp3 == 1 && dp4 == 0)
        return 1;
    else if (dp3 == 0 && dp4 == 1)
        return 2;

    return -1; // Erro
}

/*Verifica se o botão do argumento está pressionado*/
bool isPressed(int Button)
{
    if (digitalRead(Button) == 0)
    {
        return true;
    }
    return false;
}

/*Thread IHM Local*/
void *IHM_Local(void *arg)
{
    /*Definicao dos botoes e chaves como entradas*/
    pinMode(DP3, INPUT);
    pinMode(DP4, INPUT);
    pinMode(B0, INPUT);
    pinMode(B1, INPUT);
    pinMode(B2, INPUT);

    int dp3 = 0, dp4 = 0; /*Variavel das chaves*/
    int estado = 0;       /*Variavel de estado*/

    while (1)
    {
        estado = get_state_dp(dp3, dp4);
        switch (estado)
        {
        case ESTADO_MENU_DADOS:
            // print_lcd("Menu Dados:");
            while (1)
            {
                int estadoNovo = get_state_dp(dp3, dp4);
                if (state_chaged(estado, estadoNovo))
                    break;
                else
                {
                    if (isPressed(B0))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    else if (isPressed(B1))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    else if (isPressed(B2))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    /*Nenhum botao pressionado, programa nao faz nada*/
                }
                break;
            }
        case ESTADO_MENU_SOLICITAR:
            while (1)
            {
                int estadoNovo = get_state_dp(dp3, dp4);
                if (state_chaged(estado, estadoNovo))
                    break;
                else
                {
                    if (isPressed(B0))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    else if (isPressed(B1))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    else if (isPressed(B2))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    /*Nenhum botao pressionado, programa nao faz nada*/
                }
                break;
            }
        case ESTADO_MENU_CONFIGURAR:
            while (1)
            {
                int estadoNovo = get_state_dp(dp3, dp4);
                if (state_chaged(estado, estadoNovo))
                    break;
                else
                {
                    if (isPressed(B0))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    else if (isPressed(B1))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    else if (isPressed(B2))
                    {
                        // print_lcd("Escreve no lcd");
                    }
                    /*Nenhum botao pressionado, programa nao faz nada*/
                }
                break;
            }
        default:
            printf("Estado invalido\n");
            break;
        }
        /*Descomentar*/
        /*clear_lcd();*/
    }
    pthread_exit(NULL);
}