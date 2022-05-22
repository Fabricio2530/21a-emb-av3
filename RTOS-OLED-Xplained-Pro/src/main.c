#include <asf.h>
#include "conf_board.h"

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"
#include "sensor.h"

#define PI 3.142857
#define RAIO 0.2

/* Pino PA21 */
#define PINO21_PIO PIOA
#define PINO21_PIO_ID ID_PIOA
#define PINO21_IDX 21
#define PINO21_IDX_MASK (1 << PINO21_IDX)

/* Botao da placa */
#define LED_1_PIO PIOA
#define LED_1_PIO_ID ID_PIOA
#define LED_1_IDX 0
#define LED_1_IDX_MASK (1 << LED_1_IDX)

#define LED_2_PIO PIOC
#define LED_2_PIO_ID ID_PIOC
#define LED_2_IDX 30
#define LED_2_IDX_MASK (1 << LED_2_IDX)

#define LED_3_PIO PIOB
#define LED_3_PIO_ID ID_PIOB
#define LED_3_IDX 2
#define LED_3_IDX_MASK (1 << LED_3_IDX)

#define BUT_1_PIO PIOD
#define BUT_1_PIO_ID ID_PIOD
#define BUT_1_IDX 28
#define BUT_1_IDX_MASK (1u << BUT_1_IDX)

#define BUT_2_PIO PIOA
#define BUT_2_PIO_ID ID_PIOA
#define BUT_2_IDX 19
#define BUT_2_IDX_MASK (1u << BUT_2_IDX)

#define BUT_3_PIO PIOC
#define BUT_3_PIO_ID ID_PIOC
#define BUT_3_IDX 31
#define BUT_3_IDX_MASK (1u << BUT_3_IDX)

/** RTOS  */
#define TASK_MAIN_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_MAIN_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

/** prototypes */
void io_init(void);
void pin_toggle(Pio *pio, uint32_t mask);
void TC_init(Tc *TC, int ID_TC, int TC_CHANNEL, int freq);
void leds(int pot);

int g_tc_counter = 0;
int potencia = 0;
int dist = 0;
int party_flag;

QueueHandle_t xQueueBUT;
QueueHandle_t xQueuedT;
QueueHandle_t xQueueFesta;

/************************************************************************/
/* RTOS application funcs                                               */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/

void but1_callback(void) {
	int value = 1;
	
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueBUT, &value, &xHigherPriorityTaskWoken);
}

void but2_callback(void) {
	
}

void but3_callback(void) {
	int value = -1;
	
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueBUT, &value, &xHigherPriorityTaskWoken);
}
	
void pino21_callback(void) {
	int dt = g_tc_counter;
	g_tc_counter = 0;
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueuedT, &dt, &xHigherPriorityTaskWoken);
	
}

void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	g_tc_counter++;
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_main(void *pvParameters) {
	gfx_mono_ssd1306_init();
 
	init_sensor();
	io_init();
	
	int but;
	int t;
	int flg;
	
	for (;;)  {
		if (xQueueReceive(xQueueBUT, &(but), 0)) {
			TC_init(TC0, ID_TC1, 1, 100);
			tc_start(TC0, 1);
			
			potencia += but;
			
			if (potencia < 0) {
				potencia = 0;
			} else if (potencia > 4) {
				potencia = 4;
			}
			
			printf("O valor da potencia é de: %d\n", potencia);
			patinete_power(potencia);
			leds(potencia);
		}
		
		if (xQueueReceive(xQueuedT, &(t), 0)) {
			double T = t*0.01;
			double w = 2*PI/T;
			int v = RAIO*w*3.6;
			dist += (v * T)/3.6;
			
			printf("A velocidade é de: %d\n", v);
			printf("A distancia total é de: %d\n", dist);
			
			char vel[300];
			char distancia[300];
			sprintf(vel, "vel %d [km/h]", v);
			gfx_mono_draw_string(vel, 0,0, &sysfont);
			
			sprintf(distancia, "dist %d [m]", dist);
			gfx_mono_draw_string(distancia, 0,20, &sysfont);
		} 
		
		if (xQueueReceive(xQueueFesta, &(flg), 0)) {
			if (flg) {
				party_flag = 1;
			} else {
				party_flag = 0;
			}
		}
		
		if (party_flag) {
			pin_toggle(LED_1_PIO, LED_1_IDX_MASK);
			pin_toggle(LED_2_PIO, LED_2_IDX_MASK);
			pin_toggle(LED_3_PIO, LED_3_IDX_MASK);
		}
	}
}
/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void leds(int pot){
	int mode_flag;
	
	if (pot == 0) {
		pio_set(LED_1_PIO, LED_1_IDX_MASK);
		pio_set(LED_2_PIO, LED_2_IDX_MASK);
		pio_set(LED_3_PIO, LED_3_IDX_MASK);
		mode_flag = 0;
	} else if (pot == 1) {
		pio_clear(LED_1_PIO, LED_1_IDX_MASK);
		pio_set(LED_2_PIO, LED_2_IDX_MASK);
		pio_set(LED_3_PIO, LED_3_IDX_MASK);
		mode_flag = 0;
	} else if (pot == 2) {
		pio_clear(LED_1_PIO, LED_1_IDX_MASK);
		pio_clear(LED_2_PIO, LED_2_IDX_MASK);
		pio_set(LED_3_PIO, LED_3_IDX_MASK);
		mode_flag = 0;
	} else if (pot == 3) {
		pio_clear(LED_1_PIO, LED_1_IDX_MASK);
		pio_clear(LED_2_PIO, LED_2_IDX_MASK);
		pio_clear(LED_3_PIO, LED_3_IDX_MASK);
		mode_flag = 0;
	} else {
		mode_flag = 1;
	}
	
	BaseType_t xHigherPriorityTaskWoken = pdTRUE;
	xQueueSendFromISR(xQueueFesta, &mode_flag, &xHigherPriorityTaskWoken);
}

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
}

void io_init(void) {
  pmc_enable_periph_clk(LED_1_PIO_ID);
  pmc_enable_periph_clk(LED_2_PIO_ID);
  pmc_enable_periph_clk(LED_3_PIO_ID);
  pmc_enable_periph_clk(BUT_1_PIO_ID);
  pmc_enable_periph_clk(BUT_2_PIO_ID);
  pmc_enable_periph_clk(BUT_3_PIO_ID);
  pmc_enable_periph_clk(PINO21_PIO_ID);
	
  pio_set_output(LED_1_PIO, LED_1_IDX_MASK, 1, 0, 0);
  pio_set_output(LED_2_PIO, LED_2_IDX_MASK, 1, 0, 0);
  pio_set_output(LED_3_PIO, LED_3_IDX_MASK, 1, 0, 0);

  
  pio_configure(BUT_1_PIO, PIO_INPUT, BUT_1_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
  pio_configure(BUT_2_PIO, PIO_INPUT, BUT_2_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
  pio_configure(BUT_3_PIO, PIO_INPUT, BUT_3_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
  
  pio_set_input(PINO21_PIO,PINO21_IDX_MASK,PIO_DEFAULT);

  pio_handler_set(BUT_1_PIO, BUT_1_PIO_ID, BUT_1_IDX_MASK, PIO_IT_FALL_EDGE,
  but1_callback);
  pio_handler_set(BUT_2_PIO, BUT_2_PIO_ID, BUT_2_IDX_MASK, PIO_IT_FALL_EDGE,
  but3_callback);
  pio_handler_set(BUT_3_PIO, BUT_3_PIO_ID, BUT_3_IDX_MASK, PIO_IT_FALL_EDGE,
  but2_callback);
  
  pio_handler_set(PINO21_PIO, PINO21_PIO_ID, PINO21_IDX_MASK, PIO_IT_RISE_EDGE,
  pino21_callback);

  pio_enable_interrupt(BUT_1_PIO, BUT_1_IDX_MASK);
  pio_enable_interrupt(BUT_2_PIO, BUT_2_IDX_MASK);
  pio_enable_interrupt(BUT_3_PIO, BUT_3_IDX_MASK);
  pio_enable_interrupt(PINO21_PIO, PINO21_IDX_MASK);

  pio_get_interrupt_status(BUT_1_PIO);
  pio_get_interrupt_status(BUT_2_PIO);
  pio_get_interrupt_status(BUT_3_PIO);
  pio_get_interrupt_status(PINO21_PIO);

  NVIC_EnableIRQ(BUT_1_PIO_ID);
  NVIC_SetPriority(BUT_1_PIO_ID, 4);

  NVIC_EnableIRQ(BUT_2_PIO_ID);
  NVIC_SetPriority(BUT_2_PIO_ID, 4);

  NVIC_EnableIRQ(BUT_3_PIO_ID);
  NVIC_SetPriority(BUT_3_PIO_ID, 4);
  
  NVIC_EnableIRQ(PINO21_PIO_ID);
  NVIC_SetPriority(PINO21_PIO_ID, 4);
}



static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
		.charlength = CONF_UART_CHAR_LENGTH,
		.paritytype = CONF_UART_PARITY,
		.stopbits = CONF_UART_STOP_BITS,
	};

	/* Configure console UART. */
	stdio_serial_init(CONF_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/


int main(void) {
	/* Initialize the SAM system */
	sysclk_init();
	board_init();

	/* Initialize the console uart */
	configure_console();

	/* Create task to control oled */
	if (xTaskCreate(task_main, "main", TASK_MAIN_STACK_SIZE, NULL, TASK_MAIN_STACK_PRIORITY, NULL) != pdPASS) {
	  printf("Failed to create main task\r\n");
	}

	xQueueBUT = xQueueCreate(100, sizeof(int));
	if (xQueueBUT == NULL)
	printf("falha em criar a queue xQueueBUT \n");
	
	xQueuedT = xQueueCreate(100, sizeof(int));
	if (xQueuedT == NULL)
	printf("falha em criar a queue xQueuedT \n");
	
	xQueueFesta = xQueueCreate(100, sizeof(int));
	if (xQueueFesta == NULL)
	printf("falha em criar a queue xQueueFesta \n");
	
	/* Start the scheduler. */
	vTaskStartScheduler();

  /* RTOS não deve chegar aqui !! */
	while(1){}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}
