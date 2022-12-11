#include <stdio.h>
#include <stdlib.h>

typedef struct No
{
    int valor;
    struct No *proximo;
} No;

typedef struct
{
    No *inicio, *fim;
    int tam;
} Lista;

void inserirInicio(Lista *lista, int valor);
void inserirFim(Lista *lista, int valor);
void remover(Lista *lista, int valor);
No *removerPrimeiroNO(Lista *lista);
void dividirLista(Lista *lista, Lista *listaI, Lista *listaP);