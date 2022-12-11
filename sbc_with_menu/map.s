.global memory_map
@ Mapeia o endereço de memória virtual
memory_map:
    LDR R0, =fileName     @ R0 = nome do arquivo
    MOV R1, #0x1b0        @ Move um hexadecimal para R1
    ORR R1, #0x006        @ Faz a operação lógica or para juntar o valor hexadecimal passado com o valor de R1
    MOV R2, R1            @ Modo de acesso de permissão de escrita ou de leitura (O que eu posso fazer)
    MOV R7, #5            @ sys_open
    SWI 0
    MOV R4, R0            @ Salva o descritor do arquivo.

    LDR R5, =gpioaddr                  @ Carrega o endereço do Offset
    LDR R5, [R5]                       @ Carrega o Offset
    MOV R1, #4096                      @ Informa o tamanho da pagina de memoria
    MOV R2, #(1 + 2)                   @ Proteção de escrita(1) + Proteção de entrada(2). A página pode ser lida e escrita. 
    MOV R3, #1                         @ Indica que é uma memória compartilhada map-shared
    MOV R0, #0                         @ Define como NULL o endereço virtual. Dessa forma, o linux escolhe automaticamente.
    MOV R7, #192                       @ sys map
    SWI 0                               
    MOV R8, R0                         @ Move o ponteiro do endereço de memoria mapeado para o R8                     
    
    BX LR

.data
fileName: .asciz "/dev/mem" @ Caminho do arquivo para mapeamento da memória virtual
gpioaddr: .word 0x20200 @ Offset na memória física da área a ser mapeada
