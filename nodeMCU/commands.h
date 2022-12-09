/**
 * @file commands.h
 * @author Sergio Matos (sergiopugli@gmail.com), Lucas Paiva ()
 * @brief Comandos de comunicação entre nó de sensores NodeMCU e SBC
 * @version 0.1
 * @date 2022-11-02
 *
 */

// #define __TESTING__

/**
 * @brief Request commands: Comandos de solicitação enviados pelo SBC
 *
 */
#ifdef __TESTING__
#define READ_DIGITAL 'D'
#define READ_ANALOG 'A'
#define NODE_STATUS 'S'
#define LED_TOGGLE 'L'
#else
#define READ_DIGITAL 0x5
#define READ_ANALOG 0x4
#define NODE_STATUS 0x3
#define LED_TOGGLE 0x6
#endif

/**
 * @brief Response commands: Comandos de resposta enviados pelo NodeMCU
 *
 */
#ifdef __TESTING__
#define NODE_TROUBLE 'T'
#define NODE_NORMAL 'N'
#define ANALOG_READ 'A'
#define DIGITAL_READ 'D'
#else
#define NODE_TROUBLE 0x1f
#define NODE_SKIP 0xff
#define NODE_NORMAL 0x00
#define ANALOG_READ 0x1
#define DIGITAL_READ 0x2
#endif

/**
 * Comandos de configuração e controle
*/
#define SET_NEW_TIME 0x7
