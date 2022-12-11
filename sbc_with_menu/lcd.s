.global init_lcd
.global clear_lcd
.global write_char

.include "gpio.s"

.macro nanoSleep time
        LDR R0,=\time      
        LDR R1,=\time        
        MOV R7, #162  
        SWI 0                
.endm

@ Da um pulso no pino enable o ligando e desligando.
.macro enable
        GPIOTurn e, #0
        nanoSleep time1ms

        GPIOTurn e,#1
        nanoSleep time1ms

        GPIOTurn e, #0
        nanoSleep time1ms
        .ltorg
.endm

@ Seta todos os pinos do lcd como saída.
.macro setOut
        GPIODirectionOut e
        GPIODirectionOut rs
        GPIODirectionOut d7
        GPIODirectionOut d6
        GPIODirectionOut d5
        GPIODirectionOut d4
.endm

@ Macro para setar o modo de operação do display para 4 bits.
.macro fset

        GPIOTurn rs,#0

        GPIOTurn d7,#0

        GPIOTurn d6,#0

        GPIOTurn d5, #1

        GPIOTurn d4, #0

        enable
        .ltorg
.endm

@ Macro que define se o cursor será exibido e piscará e se o display será ligado 
.macro display d,c,b

        GPIOTurn rs,#0

        GPIOTurn d7,#0

        GPIOTurn d6, #0

        GPIOTurn d5,#0

        GPIOTurn d4, #0

        enable

        GPIOTurn rs, #0

        GPIOTurn d7,#1

        GPIOTurn d6,#\d     @ O display está ligado quando D é 1 e desligado quando D é 0

        GPIOTurn d5,#\c     @ O cursor é exibido quando C é 1 e não é exibido quando C é 0
 
        GPIOTurn d4,#\b     @ Faz o cursor piscar se B for igual a 1, se não, o cursor fica parado

        enable

        .ltorg
.endm

@ Define o modo de entrada do display. Faz com que o cursor ou display desloque
@ para esquerda ou direita quando um caractere é escrito
.macro entrymode i_d, s

        GPIOTurn rs, #0

        GPIOTurn d7,#0

        GPIOTurn d6,#0

        GPIOTurn d5,#0

        GPIOTurn d4,#0

        enable

        GPIOTurn rs,#0

        GPIOTurn d7,#0

        GPIOTurn d6,#1

        GPIOTurn d5,#\i_d  @ O cursor se move para a direita e para a esquerda quando quando um caractere é escrito ou lido

        GPIOTurn d4,#\s    @ O display se move para a direita ou para a esquerda quando S é 1. O display não muda se S for 0

        enable

        .ltorg
.endm


@ Macro para limpar o display LCD.

.macro clear

        GPIOTurn rs,#0

        GPIOTurn d7,#0

        GPIOTurn d6,#0

        GPIOTurn d5,#0

        GPIOTurn d4,#0

        enable

        GPIOTurn rs,#0

        GPIOTurn d7,#0

        GPIOTurn d6,#0

        GPIOTurn d5,#0

        GPIOTurn d4,#1

        enable
        .ltorg
.endm


@ Macro para escrever um caractere no display
@
@ Parâmetros
@ hex - Hexadecimal do caractere a ser escrito
.macro write hex
        MOV R6, \hex

        GPIOTurn rs, #1

        MOV R0,#7
        get_bit
        GPIOTurn d7, R1

        MOV R0,#6
        get_bit
        GPIOTurn d6, R1

        MOV R0,#5
        get_bit
        GPIOTurn d5, R1

        MOV R0,#4
        get_bit
        GPIOTurn d4, R1

        enable

        GPIOTurn rs, #1

        MOV R0,#3
        get_bit
        GPIOTurn d7, R1


        MOV R0,#2
        get_bit
        GPIOTurn d6, R1


        MOV R0,#1
        get_bit
        GPIOTurn d5, R1


        MOV R0,#0
        get_bit
        GPIOTurn d4, R1

        enable
        .ltorg
.endm

.macro  config_lcd
        setOut 
        
        fset
        fset
        fset
        fset
		
        display 1, 1, 1 
        entrymode 0, 0 
        
.endm

@ branch que pega um bit do valor hexadecimal do caractere
.macro get_bit
        MOV R2,#1 @ Move 1 para o registrador R0
        LSL R2,R0 @ Desloca para esquerda o valor em R2 para a posição do bit passado em R0
        AND R1, R6,R2 @ Realiza a operação lógica and para que seja pego apenas o bit desejado
        LSR R1,R0 @ Desloca para o bit menos significativo o bit da posição desejada

.endm

init_lcd:
	config_lcd
	BX LR

write_char:
	MOV R9,R0
	write R9
	BX LR
	
clear_lcd:
	clear
	BX LR
	

.data

time1s:
	.word 1 @ Tempo em segundos
	.word 000000000 @ Tempo em nanossegundos

time500ms:
	.word 0 @ Tempo em segundos
	.word 500000000 @ Tempo em nanossegundos
time1ms:
	.word 0 @ Tempo em segundos
	.word 1000000 @ Tempo em nanossegundos

@ Lcd

rs:
	.word 8 @ offset para selecionar o registrador de função
	.word 15 @ offset do pino no registrador de função
	.word 25 @ offset do pino no registrador de set e clear
e:
	.word 0 @ offset para selecionar o registrador de função
	.word 3 @ offset do pino no registrador de função
	.word 1 @ offset do pino no registrador de set e clear
d4: 
	.word 4 @ offset para selecionar o registrador de função
	.word 6 @ offset do pino no registrador de função
	.word 12 @ offset do pino no registrador de set e clear
d5:
	.word 4 @ offset para selecionar o registrador de função
	.word 18 @ offset do pino no registrador de função
	.word 16 @ offset do pino no registrador de set e clear 
d6:
	.word 8 @ offset para selecionar o registrador de função
	.word 0 @ offset do pino no registrador de função
	.word 20 @ offset do pino no registrador de set e clear
d7:
 	.word 8 @ offset para selecionar o registrador de função
	.word 3 @ offset do pino no registrador de função
	.word 21 @ offset do pino no registrador de set e clear
