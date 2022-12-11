.equ setregoffset, 28 @ Offset do registrador de definição
.equ clrregoffset, 40 @ Offset do registrador de limpeza

@ Define a direção do pino como saída
.macro GPIODirectionOut pin
        LDR R2,=\pin         @ Carrega o endereço do mapeamento do pino em R2
        LDR R2, [R2]         @ Carrega o valor do mapeamento do pino com offset igual a 0 em R2
        LDR R1, [R8, R2]     @ Carrega em R1 o valor do endereço base em R8 com um offset que é o va$
        LDR R3, =\pin        @ Carrega o endereço do mapeamento do pino em R3
        ADD R3, #4           @ Adiciona no endereço do mapeamento do pino carregado em R3 um offset $
        LDR R3, [R3]         @ Carrega o valor que está no endereço em R3
        MOV R0, #0b111       @ Mascara para limpar 3 bits
        LSL R0, R3           @ Faz um deslocamento em R0 com o valor que está em R3
        BIC R1, R0           @ Limpa os 3 bits da posição
        MOV R0, #1           @ Move 1 bit para R0
        LSL R0, R3           @ Faz um deslocamento em R0 com o valor que está em R3
        ORR R1, R0           @ Faz uma operação lógica ORR para adicionar na posição o valor 1
        STR R1, [R8, R2]     @ Armazena no endereço base em R8 com um offset que é o valor de R2, o $
.endm

@ A branch GPIOTurn passa o valor 0 ou 1 para um pino o ativando ou limpando.
@ value - Valor (1 = ativar, 0 = limpar)
@ pin - Pino (Pino que será setado)

.macro GPIOTurn pin, value
        MOV R1, \value
        LDR R3, =\pin

        MOV R2, R8              @ Move o endereço base em R8, para o R2
        CMP R1,#0               @ Compara se R1 tem valor 0
        ADDEQ R2, #clrregoffset @ Se for, adiciona no endereço base o offset do registrador de limpe$
        CMP R1,#1               @ Compara se valor = 1
        ADDEQ R2, #setregoffset @ Se for, adiciona no endereço base o offset do registrador de defin$

        MOV R0, #1              @ Move 1 para o registrador R0
        ADD R3, #8              @ Adiciona no endereço do mapeamento do pino, o offset da terceira p$
        LDR R3, [R3]            @ Carrega o valor da terceira palavra
        LSL R0, R3              @ Faz um deslocamento em R0 com o valor da terceira palavra
        STR R0, [R2]            @ Carrega no valor da terceira palavra o valor que está em R0
.endm
