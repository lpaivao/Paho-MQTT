#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <MQTTClient.h>
#include <unistd.h>
#include <stdbool.h>
#include <wiringPi.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/ioctl.h>
#include <stdint.h>

/*Biblioteca para a IHM local*/
#include "ihm_local.h"
/*Arquivo com as definições de tópicos*/
#include "../topicos.h"
#include "../nodeMCU/commands.h"
/*Biblioteca da lista encadeada para armazenamento do historico*/
#include "lista_encadeada.h"

/*Constantes para confiugração do MQTT*/
// #define IP "tcp://test.mosquitto.org:1883" /*mudar para "tcp://10.0.0.101:1883"*/
#define IP "tcp://10.0.0.101:1883"
#define CLIENTID "ihmlocalG1"
#define USER "aluno"
#define PASSWORD "@luno*123"
#define QOS 1

/* Funções da Biblioteca do LCD*/

extern void memory_map(void);
extern void init_lcd(void);
extern void clear_lcd(void);
extern void write_char(unsigned char c);

/*Inicialização do LCD*/
void lcd()
{
    memory_map();
    init_lcd();
    init_lcd();
    init_lcd();
    clear_lcd();
}

/*Mostrar uma string no LCD*/

void print_lcd(unsigned char c[])
{
    int len = strlen(c);

    for (int i = 0; i < len; i++)
    {
        write_char(c[i]);
    }
}
/*Variaveis Globais*/
/*Variaveis para configurar mqtt*/
MQTTClient client;
int rc;
/*Variaveis de listas encadeadas para armazenar historico*/
Lista list_historic_A0;
Lista list_historic_D0;
Lista list_historic_D1;
/**/
char config_tempo[3] = {1, 5, 10};
char sensores[3] = {0, 1, 2};

/*Delay*/
char delayTime = 5;

/*Declaracao de funcoes*/
void publish(MQTTClient client, char *topic, char *payload);
void *IHM_Local(void *arg);
void *IHM_Remoto(void *arg);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
void msgarrvd_remoto(char *topicName, MQTTClient_message *message);
void msgarrvd_local_node(char *topicName, MQTTClient_message *message);

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

void retorna_historico_para_remoto(Lista *list, char *topic, char comando)
{
    char *historico;
    historico[0] = comando;

    int i;

    No *aux = (No *)malloc(sizeof(No)); /*No auxiliar*/
    aux = list->inicio;                 /*Ponteiro para inicio da lista*/
    int list_len = list->tam;           /*Tamanho da lista para loop*/

    if (list_len > 0)
    {
        for (i = 0; i < list_len; i++)
        {
            historico[i] = aux->valor;
        }
        publish(client, topic, historico);
    }
    else
    {
        publish(client, topic, historico);
    }
}

/*Call back de requisições do IHM local para a Node*/
void msgarrvd_local_node(char *topicName, MQTTClient_message *message)
{
    char *payloadptr;
    payloadptr = message->payload;

    // char *dado;

    switch (payloadptr[0])
    {
    case ANALOG_READ:
        add_medicao_historico(&list_historic_A0, payloadptr[1]);
        break;

    case DIGITAL_READ:
        if (strcmp(topicName, SENSOR_D0_TOPIC) == 0)
        {
            add_medicao_historico(&list_historic_D0, payloadptr[1]);
            break;
        }
        else if (strcmp(topicName, SENSOR_D0_TOPIC) == 0)
        {
            add_medicao_historico(&list_historic_D1, payloadptr[1]);
            break;
        }

    case NODE_NORMAL:
        printf("Node normal\n");
        print_lcd("Node Normal");
        break;

    case NODE_SKIP:
        printf("Node skip\n");
        print_lcd("Node Skip");
        break;

    case NODE_TROUBLE:
        printf("Node trouble\n");
        print_lcd("Node Trouble");
        break;
    }
}

void msgarrvd_remoto(char *topicName, MQTTClient_message *message)
{
    char *payloadptr;
    payloadptr = message->payload;

    // char *dado;

    switch (payloadptr[0])
    {
    case REQ_HIST_ANALOG:
        retorna_historico_para_remoto(&list_historic_A0, topicName, RESP_HIST_ANALOG);
        break;
    case REQ_HIST_DIGITAL:
        if (strcmp(topicName, SBC_SENSOR_D0_HIST) == 0)
        {
            retorna_historico_para_remoto(&list_historic_D0, topicName, RESP_HIST_DIGITAL);
        }
        else if (strcmp(topicName, SBC_SENSOR_D1_HIST) == 0)
        {
            retorna_historico_para_remoto(&list_historic_D1, topicName, RESP_HIST_DIGITAL);
        }
        break;
    }
}

void msgarrvd_local_time_confg(char *topicName, MQTTClient_message *message)
{
    char *payloadptr;
    payloadptr = message->payload;

    // char *dado;

    if (payloadptr[0] == SET_NEW_TIME)
    {
        delayTime = payloadptr[1];
        printf("[ %s ] new time: %d\n", SBC_CONFIG_TIME_TOPIC, delayTime);
    }
}

/*Tratamento da notificação para o sbc pegar os dados dos tópicos que está inscrito pelo primeiro byte do payload, utilizada pela função MQTTClient_setCallbacks();*/
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    // char *payloadptr;

    // payloadptr = message->payload;

    msgarrvd_local_time_confg(topicName, message);
    msgarrvd_local_node(topicName, message);
    msgarrvd_remoto(topicName, message);

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
    pubmsg.payloadlen = strlen(pubmsg.payload);
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

/*------------------------------------------------- TUDO RELACIONADO AO IHM LOCAL ABAIXO -------------------------------------------------*/

/*Compara a combinação das chaves 3 e 4 atual com a antiga e verifica se o estado mudou*/
bool state_chaged(int *estadoAntigo, int estadoAtual)
{
    // if (*estadoAntigo != estadoAtual)
    // {
    //     *estadoAntigo = estadoAtual;
    //     return true;
    // }
    // return false;
    return (*estadoAntigo != estadoAtual);
}

/*Verifica o estado do DIP e retorna o inteiro em decimal referente ao binário da combinação das chaves 3 e 4
OBS: os botões possuem logica inversa!!*/
int get_state_dp()
{
    int dp3, dp4;
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

/*Para confirmar se o botão foi pressionado*/
bool debounce(int button)
{
    // delayMicroseconds(500);
    if (digitalRead(button) == 0)
    {
        /*Enquanto o botão estiver pressionado*/
        // while (1)
        // {
        delayMicroseconds(5000);      /*Faz uma nova leitura do botão depois de 0,005 Segundos [s]*/
        if (digitalRead(button) == 0) /*Se o botão não estiver mais pressionado, sai do loop*/
            // }
            return true;
    }
    return false;
}

/*Verifica se o botão do argumento foi pressionado e solto*/
bool wasPressed(int button)
{
    return debounce(button);
    // if (digitalRead(button) == 0)
    // {
    //     if (debounce(button))
    //         return true;
    //     return false; /*Botao nao foi pressionado, alarme falso*/
    // }
    // return false;
}

/*Faz o tratamento para mostrar no display um valor analogico*/
void mostra_valor_analogico(char *valorAnalogico)
{
    char buffer[20];
    sprintf(buffer, "%d", *valorAnalogico);
    print_lcd(buffer);
}
/*Faz o tratamento para mostrar no display um valor digital*/
void mostra_valor_digital(char *estado)
{
    write_char(*estado + '0');
}

/*Funcao que percorre a lista de valores digitais do historico e mostra no display*/
void visualizar_historico_digital(int estado, Lista *list)
{
    int count = 0;
    No *aux = (No *)malloc(sizeof(No)); /*No auxiliar*/
    aux = list->inicio;                 /*Ponteiro para inicio da lista*/
    int list_len = list->tam;           /*Tamanho da lista para loop*/

    int dp3, dp4;

    if (list_len > 0)
    {
        while (!wasPressed(BTN1) || !state_chaged(estado, get_state_dp()))
        {
            if (wasPressed(BTN2))
            {
                /*Primeiro limpa lcd*/
                clear_lcd();
                /*Depois mostra*/
                print_lcd("Medida ");
                print_lcd(count);
                print_lcd(": ");
                mostra_valor_digital((char)aux->valor);
                count++;
                /*Se nao for o ultimo elemento da lista, aponta para o proximo*/
                if (count < list_len)
                    aux = aux->proximo;
                else
                {
                    aux = list->inicio;
                    count = 0;
                }
            }
        }
    }
    else
    {
        clear_lcd();
        print_lcd("Historico vazio");
    }
}

/*Funcao que percorre a lista de valores analogicos do historico e mostra no display*/
void visualizar_historico_analogico(int estado, Lista *list)
{
    int count = 0;
    No *aux = (No *)malloc(sizeof(No)); /*No auxiliar*/
    aux = list->inicio;                 /*Ponteiro para inicio da lista*/
    int list_len = list->tam;           /*Tamanho da lista para loop*/

    int dp3, dp4;

    if (list_len > 0)
    {
        while (!wasPressed(BTN1) || !state_chaged(estado, get_state_dp()))
        {
            if (wasPressed(BTN2))
            {
                /*Primeiro limpa lcd*/
                clear_lcd();
                /*Depois mostra*/
                print_lcd("Medida ");
                print_lcd(count);
                print_lcd(": ");
                mostra_valor_analogico((char)aux->valor);
                count++;
                /*Se nao for o ultimo elemento da lista, aponta para o proximo*/
                if (count < list_len)
                    aux = aux->proximo;
                else
                {
                    aux = list->inicio;
                    count = 0;
                }
            }
        }
    }
    else
    {
        clear_lcd();
        print_lcd("Historico vazio");
    }
}

/*Estado do menu para visualização do historico*/
void estado_menu_dados(int *estado, int dp3, int dp4)
{
    print_lcd("Ver ultimas medicoes: ");
    print_lcd("A0, D0, D1");
    while (1)
    {
        /*Verifica se o estado mudou*/
        if (state_chaged(estado, get_state_dp()))
            break;
        else
        {
            /*confirma entrada nas opcao de ver medicoes*/
            if (wasPressed(BTN0))
            {
                int count = 0;
                int aux = 0;
                clear_lcd();
                print_lcd("A0");
                /*enquanto o botao de voltar nao for pressionado ou o estado do dip nao mudar, continua mostrando as opcoes*/
                while (!wasPressed(BTN1) || !state_chaged(estado, get_state_dp()))
                {
                    /*confirma entrada na configuração do tempo*/
                    if (wasPressed(BTN0))
                    {
                        if (count == 0)
                        {
                            visualizar_historico_analogico(estado, &list_historic_A0);
                        }
                        else if (count == 1)
                        {
                            visualizar_historico_digital(estado, &list_historic_D0);
                        }
                        else if (count == 2)
                        {
                            visualizar_historico_digital(estado, &list_historic_D1);
                        }
                        else if (count > 2)
                        {
                            count = 0;
                        }
                        break;
                    }
                    /*mostra proximo sensor*/
                    if (wasPressed(BTN2))
                    {
                        aux++;
                        clear_lcd();
                        if (aux == 0)
                        {
                            print_lcd("Historico A0");
                        }
                        else if (aux == 1)
                        {
                            print_lcd("Historico D0");
                        }
                        else if (aux == 2)
                        {
                            print_lcd("Historico D1");
                        }
                        if (aux > 2)
                            aux = 0;
                    }
                }
            }
        }
        break;
    }
}

/*Funcao para enviar comandos para a ESP*/
void solicitacao_para_esp(char hexaCode, char numSensor)
{
    char *topic = COMMAND_TO_ESP_TOPIC;
    char *payload;
    payload[0] = hexaCode;
    payload[1] = numSensor;
    publish(client, topic, payload);
}

/*Menu de solicitações para a ESP*/
void estado_menu_solicitar(int estado, int dp3, int dp4)
{
    print_lcd("Solicitacoes para esp: ");
    print_lcd("A0, D0, D1, Led Tog, Node status");
    while (1)
    {
        /*Verifica se o estado mudou*/
        if (state_chaged(estado, get_state_dp()))
            break;
        else
        {
            /*confirma entrada nas opcao de solicitar medicoes*/
            if (wasPressed(BTN0))
            {
                int count = 0;
                int aux = 0;
                clear_lcd();
                print_lcd("A0");
                /*enquanto o botao de voltar nao for pressionado ou o estado do dip nao mudar, continua mostrando as opcoes*/
                while (!wasPressed(BTN1) || !state_chaged(estado, get_state_dp()))
                {
                    /*confirma entrada na configuração do tempo*/
                    if (wasPressed(BTN0))
                    {
                        if (count == 0)
                        {
                            solicitacao_para_esp(READ_ANALOG, 0);
                            print_lcd("Solicitacao: A0");
                        }
                        else if (count == 1)
                        {
                            solicitacao_para_esp(READ_DIGITAL, 0);
                            print_lcd("Solicitacao: D0");
                        }
                        else if (count == 2)
                        {
                            solicitacao_para_esp(READ_DIGITAL, 1);
                            print_lcd("Solicitacao: D0");
                        }
                        else if (count == 3)
                        {
                            solicitacao_para_esp(LED_TOGGLE, 0);
                            print_lcd("Solicitacao: LED Toggle");
                        }
                        else if (count == 4)
                        {
                            solicitacao_para_esp(NODE_STATUS, 0);
                            print_lcd("Solicitacao: Node Status");
                        }
                        else if (count > 4)
                        {
                            count = 0;
                        }
                        delay(1); /*mostra o pedido de atualizacao por 1 segundo*/
                        break;
                    }
                    /*mostra proximo sensor*/
                    if (wasPressed(BTN2))
                    {
                        aux++;
                        clear_lcd();
                        if (aux == 0)
                        {
                            print_lcd("Medida atual A0");
                        }
                        else if (aux == 1)
                        {
                            print_lcd("Medida atual D0");
                        }
                        else if (aux == 2)
                        {
                            print_lcd("Medida atual D1");
                        }
                        else if (aux == 3)
                        {
                            print_lcd("Led Toggle");
                        }
                        else if (aux == 4)
                        {
                            print_lcd("Node Status");
                        }
                        if (aux > 4)
                            aux = -1;
                    }
                }
            }
        }
        break;
    }
}

/*Menu de solicitações do tempo*/
void estado_menu_configurar(int estado, int dp3, int dp4)
{
    print_lcd("Configuracao de Tempo:");
    while (1)
    {
        /*Verifica se o estado mudou*/
        if (state_chaged(estado, get_state_dp()))
            break;
        else
        {
            /*confirma entrada nas opcoes de configuração do tempo*/
            if (wasPressed(BTN0))
            {
                int count = 0;
                clear_lcd();
                print_lcd("Nova configuracao: ");
                print_lcd(config_tempo[count]);
                /*enquanto o botao de voltar nao for pressionado ou o estado do dip nao mudar, continua mostrando as opcoes*/
                while (!wasPressed(BTN1) || !state_chaged(estado, get_state_dp()))
                {
                    /*confirma valor de configuração do tempo*/
                    if (wasPressed(BTN0))
                    {
                        delayTime = config_tempo[count];
                        print_lcd("Novo valor de tempo configurado: ");
                        print_lcd((char)config_tempo[count]);
                        delay(2); /*mostra o novo valor por 2 segundos*/
                        break;
                    }
                    /*verifica proximo valor de configuração do tempo*/
                    if (wasPressed(BTN2))
                    {
                        clear_lcd();
                        print_lcd("Nova configuracao: ");
                        print_lcd((char)config_tempo[count]);
                        if (count == 2)
                            count = 0;
                        count++;
                    }
                }
            }
        }
        break;
    }
}
/*Thread IHM Local*/
void *thread_ihm_Local(void *arg)
{
    // piHiPri(0);
    /*Definicao dos botoes e chaves como entradas*/
    pinMode(DP3, INPUT);
    pinMode(DP4, INPUT);
    pinMode(BTN0, INPUT);
    pinMode(BTN1, INPUT);
    pinMode(BTN2, INPUT);

    int dp3 = 0, dp4 = 0; /*Variavel das chaves*/
    int estado = 0;       /*Variavel de estado*/

    while (1)
    {
        estado = get_state_dp();
        switch (estado)
        {
        case ESTADO_MENU_DADOS:
            estado_menu_dados(&estado, dp3, dp4);
        case ESTADO_MENU_SOLICITAR:
            estado_menu_solicitar(&estado, dp3, dp4);
        case ESTADO_MENU_CONFIGURAR:
            estado_menu_configurar(&estado, dp3, dp4);
        default:
            print_lcd("Estado invalido: mude as chaves para 00, 01 ou 10");
            break;
        }
        clear_lcd();
    }
    pthread_exit(NULL);
}

/*------------------------------------------------- TUDO RELACIONADO A THREAD DE LEITURA DOS SENSORES-------------------------------------------------*/
/*Publish automatico A0*/
void publish_medicao_A0()
{
    char *dado;
    dado[0] = READ_ANALOG;
    dado[1] = 0;
    publish(client, COMMAND_TO_ESP_TOPIC, dado);
}

/*Publish automatico D0*/
void publish_medicao_D0()
{
    char *dado;
    dado[0] = READ_DIGITAL;
    dado[1] = 0;
    publish(client, COMMAND_TO_ESP_TOPIC, dado);
}

/*Publish automatico D1*/
void publish_medicao_D1()
{
    char *dado;
    dado[0] = READ_DIGITAL;
    dado[1] = 1;
    publish(client, COMMAND_TO_ESP_TOPIC, dado);
}

/*thread para leitura automatica do valor dos sensores*/
void *thread_leitura_sensores(void *arg)
{
    piHiPri(1);
    while (1)
    {
        publish_medicao_A0();
        publish_medicao_D0();
        publish_medicao_D1();
        delay(delayTime);
    }
    pthread_exit(NULL);
}

/*---------------------------------------------- MAIN ----------------------------------------------*/

int main(int argc, char *argv[])
{
    lcd();           /*Primeira configuração do LCD*/
    mqtt_config();   /*Configuração MQTT*/
    wiringPiSetup(); /*Configuracao do wiringPi*/

    start_subscribe_topics(client); /*Faz todos os subscribes no inicio do programa*/

    // piThreadCreate(thread_ihm_Local);        // Criacao da thread para o IHM Local com wiringp
    //  piThreadCreate(thread_leitura_sensores); // Criacao da thread para a leitura dos sensores a cada periodo de tempo com wiringpi

    pthread_t threadIHM;
    pthread_create(&threadIHM, NULL, thread_ihm_Local, NULL); // Criacao da thread para o IHM Local (automatico)

    while (1)
    {
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
