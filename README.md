# 21a - emb - AV3

- **A cada 30 minutos você deverá fazer um commit no seu código!**
    - Códigos que não tiverem commit a cada 30min ou que mudarem drasticamente entre os commits serão descartados (conceito I) !!
    - Você deve inserir mensagens condizentes nos commits!
- Duração: 3h
- Faça o seu trabalho de maneira ética!

Usem como base o código disponível neste repositório.

## Descrição

Esta avaliacão será mais "técnica", onde eu irei pedir para vocês realizarem tarefas que demandam conhecimento dos periféricos do uC e de RTOS. Vocês terão que usar os seguintes periféricos:

- PIO
- TC
- RTT
- ADC

E criar as seguintes tarefas:

- task_tc
- task_rtt
- task_adc
- task_main

E os seguintes recursos do RTOS:

- 

### Comecando

#### LEDs

Configure os LEDs da placa OLED, todos devem comecar apagados.

#### Botões

Configure os três botões da placa OLED para gerarem interrupção em borda de descida (quando aperta o botão), para cada botão configure uma funcão de callback e crie uma fila de inteiros (`xQueueBtn`) que a cada vez que ocorrer uma interrupcão nos botões coloqua na fila (`xQueueBtn`) um inteiro referente a qual botão foi pressionado: `1`, `2`, `3`.

A fila `xQueueBtn` deve ser lida na `task_process`.

#### TC

Configure um TC para operar a 2Hz, a cada interrupção do TC você deve liberar um semáforo (`xSemaphoreTC`). 

O semáforo (`xSemaphareTC`) deve ser lido pela `task_process` que incrementa um contador **local** a cada tick do TC (semáforo liberado). 

#### RTC

Inicialize o RTC e configure interrupção de segundo, crie um semáforo `xSemaphoreRTC` que será liberado sempre que ocorrer a interrupcão de segundos.

O semáforo (`xSemaphareRTC`) deve ser lido pela `task_process`. 

#### 

### C

- [ ] **Segue a estrutura de firmware especificada (semáforos e filas)**

### extras 

- (+0.5) Adicione novo um label para exibir o valor máximo dos dados que estão na tela (somente ADC).
