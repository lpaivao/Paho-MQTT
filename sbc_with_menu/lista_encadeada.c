#include <stdio.h>
#include <stdlib.h>
#include "lista_encadeada.h"

void inserirInicio(Lista *lista, int valor)
{
    No *novo = (No *)malloc(sizeof(No));
    novo->valor = valor;

    if (lista->inicio == NULL)
    {
        novo->proximo = NULL;
        lista->inicio = novo;
        lista->fim = novo;
    }
    else
    {
        novo->proximo = lista->inicio;
        lista->inicio = novo;
    }
    lista->tam++;
}

void inserirFim(Lista *lista, int valor)
{
    No *novo = (No *)malloc(sizeof(No));
    novo->valor = valor;
    novo->proximo = NULL;

    if (lista->inicio == NULL)
    {
        lista->inicio = novo;
        lista->fim = novo;
    }
    else
    {
        lista->fim->proximo = novo;
        lista->fim = novo;
    }
    lista->tam++;
}

void remover(Lista *lista, int valor)
{
    No *inicio = lista->inicio;
    No *noARemover = NULL;

    if (inicio != NULL && lista->inicio->valor == valor)
    {
        noARemover = lista->inicio;
        lista->inicio = noARemover->proximo;
        if (lista->inicio == NULL)
            lista->fim = NULL;
    }
    else
    {
        while (inicio != NULL && inicio->proximo != NULL && inicio->proximo->valor != valor)
        {
            inicio = inicio->proximo;
        }
        if (inicio != NULL && inicio->proximo != NULL)
        {
            noARemover = inicio->proximo;
            inicio->proximo = noARemover->proximo;
            if (inicio->proximo == NULL)
                lista->fim = inicio;
        }
    }
    if (noARemover)
    {
        free(noARemover);
        lista->tam--;
    }
}

No *removerPrimeiroNO(Lista *lista)
{
    if (lista->inicio != NULL)
    {
        No *no = lista->inicio;
        lista->inicio = no->proximo;
        lista->tam--;
        if (lista->inicio == NULL)
            lista->fim = NULL;
        return no;
    }
    else
        return NULL;
}

void dividirLista(Lista *lista, Lista *listaI, Lista *listaP)
{
    No *removido;
    while (lista->inicio != NULL)
    {
        removido = removerPrimeiroNO(lista);
        inserirFim(listaI, removido->valor);
        free(removido);

        removido = removerPrimeiroNO(lista);
        if (removido != NULL)
        {
            inserirFim(listaP, removido->valor);
            free(removido);
        }
    }
}

void imprimir(Lista *lista)
{
    No *inicio = lista->inicio;
    printf("Tamanho da lista: %d\n", lista->tam);
    while (inicio != NULL)
    {
        printf("%d ", inicio->valor);
        inicio = inicio->proximo;
    }
    printf("\n\n");
}