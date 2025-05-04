#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "teclado.h"
#include "lpc17xx_timer.h"

#define OUTPUT (uint8_t)1
#define INPUT (uint8_t)0

/*
	Filas
*/
#define PIN_0 ((uint32_t)(1 << 0))
#define PIN_1 ((uint32_t)(1 << 1))
#define PIN_2 ((uint32_t)(1 << 2))
#define PIN_3 ((uint32_t)(1 << 3))

#define TODAS (uint8_t)4
/*
	Columnas
*/
#define PIN_4 ((uint32_t)(1 << 4))
#define PIN_5 ((uint32_t)(1 << 5))
#define PIN_6 ((uint32_t)(1 << 6))
#define PIN_7 ((uint32_t)(1 << 7))

#define PORT_2 (uint8_t)2

/*
		Modos
*/


uint8_t filaActual;




void configTeclado(){
	PINSEL_CFG_Type PINSELCfg;

	/*
	 * Columnas
	 */
	PINSELCfg.Portnum = PINSEL_PORT_2; // COL3
	PINSELCfg.Pinnum = PINSEL_PIN_7;
	PINSELCfg.Funcnum = PINSEL_FUNC_0;
	PINSELCfg.Pinmode = PINSEL_PINMODE_PULLDOWN;
	PINSELCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
	PINSEL_ConfigPin(&PINSELCfg);

	PINSELCfg.Portnum = PINSEL_PORT_2; // COL2
	PINSELCfg.Pinnum = PINSEL_PIN_6;
	PINSEL_ConfigPin(&PINSELCfg);

	PINSELCfg.Portnum = PINSEL_PORT_2; // COL1
	PINSELCfg.Pinnum = PINSEL_PIN_5;
	PINSEL_ConfigPin(&PINSELCfg);

	PINSELCfg.Portnum = PINSEL_PORT_2; // COL0
	PINSELCfg.Pinnum = PINSEL_PIN_4;
	PINSEL_ConfigPin(&PINSELCfg);

	/*
	 * Filas
	 */
	PINSELCfg.Portnum = PINSEL_PORT_2; // ROW4
	PINSELCfg.Pinnum = PINSEL_PIN_3;
	PINSELCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
	PINSEL_ConfigPin(&PINSELCfg);

	PINSELCfg.Portnum = PINSEL_PORT_2; // ROW3
	PINSELCfg.Pinnum = PINSEL_PIN_2;
	PINSEL_ConfigPin(&PINSELCfg);

	PINSELCfg.Portnum = PINSEL_PORT_2; // ROW2
	PINSELCfg.Pinnum = PINSEL_PIN_1;
	PINSEL_ConfigPin(&PINSELCfg);

	PINSELCfg.Portnum = PINSEL_PORT_2; // ROW1
	PINSELCfg.Pinnum = PINSEL_PIN_0;
	PINSEL_ConfigPin(&PINSELCfg);

	GPIO_SetDir(PORT_2, 0x000F, OUTPUT); // Las filas son salidas.
	GPIO_SetDir(PORT_2, 0x00F0, INPUT);	 // Las columnas son entradas.

	/*
	 * Interrupciones
	 * InterrupciÃ³n de los pines que son columnas por flanco de subida.
	 */
	GPIO_IntCmd(PORT_2, 0x00F0,0);
	GPIO_ClearInt(PORT_2, 0x00F0);

	NVIC_EnableIRQ(EINT3_IRQn);
	NVIC_SetPriority(EINT3_IRQn,1);
	return;
}

void configTimer1(){
	/*
		 * Configuramos un delay de 10ms
		 */
	TIM_TIMERCFG_Type struct_config;
	TIM_MATCHCFG_Type struct_match;

	struct_config.PrescaleOption = TIM_PRESCALE_USVAL;
	struct_config.PrescaleValue = 1000;

	struct_match.MatchChannel = 0;
	struct_match.IntOnMatch = ENABLE;
	struct_match.ResetOnMatch = ENABLE;
	struct_match.StopOnMatch = DISABLE;
	struct_match.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	struct_match.MatchValue = 10;
	TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &struct_config);
	TIM_ConfigMatch(LPC_TIM1, &struct_match);

	NVIC_EnableIRQ(TIMER1_IRQn);
	NVIC_SetPriority(TIMER1_IRQn,2);

	TIM_Cmd(LPC_TIM1, ENABLE);

	return;
}

void rotarTeclado(){

	static int i = 0;
	setFila(i);
	i++;
	i = i%4;
	return;
}


uint8_t getKey() {

	uint8_t columna = getColumna();

	/*en esta funcion se busca que columna interrumpio segun la filaActual*/
	switch(filaActual){
		case 0:		//Settea fila 0
			if(columna == 0){
				return 1;
			}else if(columna == 1){
				return 2;
			}else if(columna == 2){
				return 3;
			}else if(columna == 3){
				return 12; //A
			}
			break;
		case 1:		//Settea fila 1
			if(columna == 0){
				return 4;
			}else if(columna == 1){
				return 5;
			}else if(columna == 2){
				return 6;
			}else if(columna == 3){
				return 13; //B
			}
			break;
		case 2:		//Settea fila 2
			if(columna == 0){
				return 7;
			}else if(columna == 1){
				return 8;
			}else if(columna == 2){
				return 9;
			}else if(columna == 3){
				return 14; //C
			}
			break;
		case 3:		//Settea fila 3
			if(columna == 0){
				return 10; //asterisco *
			}else if(columna == 1){
				return 0;
			}else if(columna == 2){
				return 11; //numeral #
			}else if(columna == 3){
				return 15; //D
			}
			break;
	}

		return 16;


}

void setFila(uint8_t fila){
	filaActual = fila;
	switch(fila){
		case 0:		//Settea fila 0
			GPIO_ClearValue(PORT_2, PIN_3);
			GPIO_SetValue(PORT_2, PIN_0);
			break;
		case 1:		//Settea fila 1
			GPIO_ClearValue(PORT_2, PIN_0);
			GPIO_SetValue(PORT_2, PIN_1);
			break;
		case 2:		//Settea fila 2
			GPIO_ClearValue(PORT_2, PIN_1);
			GPIO_SetValue(PORT_2, PIN_2);
			break;
		case 3:		//Settea fila 3
			GPIO_ClearValue(PORT_2, PIN_2);
			GPIO_SetValue(PORT_2, PIN_3);
			break;
	}
		return;
}


uint8_t getColumna(){

	/*metodo para obtener la columna que interrumpio*/

	static uint8_t columna = 4;

	if(GPIO_GetIntStatus(PORT_2, 4, 0)){
		columna = 0;

	}else if(GPIO_GetIntStatus(PORT_2, 5, 0)){
		columna = 1;

	}else if(GPIO_GetIntStatus(PORT_2, 6, 0)){
		columna = 2;

	}else if(GPIO_GetIntStatus(PORT_2, 7, 0)){
		columna = 3;

	}

	return columna;

}


void TIMER1_IRQHandler(){

	rotarTeclado();
	TIM_ClearIntPending(LPC_TIM1,TIM_MR0_INT);
	return;
}

uint32_t soltoTecla(){

	return (GPIO_ReadValue(PORT_2) & (1<<(getColumna()+4)));

//	switch(getColumna()) {
//		case 0:
//			return GPIO_ReadValue(PORT_2) & (1<<4);
//			break;
//		case 1:
//			return GPIO_ReadValue(PORT_2) & (1<<5);
//			break;
//		case 2:
//			return GPIO_ReadValue(PORT_2) & (1<<6);
//			break;
//		case 3:
//			return GPIO_ReadValue(PORT_2) & (1<<7);
//			break;
//	}

}
