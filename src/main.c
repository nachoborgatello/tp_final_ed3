#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "teclado.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_uart.h"

#define PORT_2 (uint8_t)2
#define OUTPUT (uint8_t)1
#define INPUT (uint8_t)0

/*
	Modos
*/
#define MODO_MANUAL (uint8_t)0
#define MODO_AUTOMATICO (uint8_t)1
#define MODO_GUARDAR (uint8_t)2
#define EJECUTAR (uint8_t)3

/*
		Motores, puertos de motores y pines de motores
*/
uint8_t MODO = 0;
#define MOTOR0 (uint8_t)0
#define MOTOR1 (uint8_t)1

#define PUERTO_M0 (uint8_t)0
#define PUERTO_M1 (uint8_t)0

#define M0_P0 (uint32_t)(1<<6)		//Motor 0 pin 0
#define M0_P1 (uint32_t)(1<<7)		//Motor 0 pin 1
#define M0_P2 (uint32_t)(1<<8)		//Motor 0 pin 2
#define M0_P3 (uint32_t)(1<<9)		//Motor 0 pin 3

#define M1_P0 (uint32_t)(1<<0)		//Motor 1 pin 0
#define M1_P1 (uint32_t)(1<<1)		//Motor 1 pin 1
#define M1_P2 (uint32_t)(1<<18)		//Motor 1 pin 2
#define M1_P3 (uint32_t)(1<<17)		//Motor 1 pin 3

#define GRADOS_PASO_MOTOR	(uint16_t) 7


uint8_t MOTOR = 0;		//Motor seleccionado
uint8_t giroM0 = 0;		//Variable de giro de motor 0
uint8_t giroM1 = 0;		//Varaible de giro de motor 1

__IO uint16_t velocidad = 0;

uint32_t prueba;

uint8_t posicionValor = 0;
uint32_t valoresGuardados[10];
uint8_t motorGuardado[10];
uint8_t sentidosGuardados[10];
uint8_t posicionesMotor[28];
uint32_t posiciones[2] = {0, 0};


//	GIRO
void giroGrados(uint8_t motor, uint16_t grados, uint8_t sentido);
void giroHorario(uint8_t motor);
void giroAntiHorario(uint8_t motor);

//	TECLADO
void Key();							//Consigue el boton presionado y usa el modo seleccionado
void keyManual(uint8_t key);		//Funcion del boton presionado en modo manual
void keyAutomatico(uint8_t key);	//Funcion del boton presionado en modo automatico
void keyGuardar(uint8_t key);		//Funcion del boton presionado en modo guardar
void Ejecutar();
void modificarModo(uint8_t key);
void guardarValor(uint8_t motor, uint32_t valor, uint8_t sentido);

//  UART

void mandarPosicion();
void mandarCambioModo();
void mandarCambioMotor();
void mandarInfoGrupo();
void clearScreen();


//	CONFIGURACION

void configTimer0();
void configADC();
void configGPDMA(uint32_t* mensaje, uint32_t size);
void configMotor();
void confUART();

// Delay
void delay2();
void delayGiro();

int main(){

	configMotor();
	configTeclado();
	configTimer0();
	configTimer1();
	configADC();
	confUART();

	clearScreen();

	mandarInfoGrupo();

	while (1);

	return (0);
}


void confUART(){
	PINSEL_CFG_Type PinCfg;
	//configuraci n pin de Tx
	PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	UART_CFG_Type UARTConfigStruct;
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;
	UART_ConfigStructInit(&UARTConfigStruct);
	UART_Init(LPC_UART2, &UARTConfigStruct);
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART2, &UARTFIFOConfigStruct);
	UART_TxCmd(LPC_UART2, ENABLE);

	return;
}

void configADC() {

	/*Configuracion del pin 0.23 para funcion AD0.0*/

	PINSEL_CFG_Type adcPin;
	adcPin.Portnum = PINSEL_PORT_0;
	adcPin.Pinnum = PINSEL_PIN_23;
	adcPin.Funcnum = PINSEL_FUNC_1;
	adcPin.OpenDrain = 0;
	adcPin.Pinmode = 0;
	PINSEL_ConfigPin(&adcPin);
//	ADC_Init(LPC_ADC, 200000);
//	ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
//	ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);		//Hay que configurar el Pin pero para tener algo
//	//ADC_StartCmd(LPC_ADC,ADC_START_NOW);
//	NVIC_SetPriority(ADC_IRQn,(3));

	LPC_SC->PCONP |= (1 << 12);
	LPC_ADC->ADCR |= (1 << 21);   	//habilitamos el ADC
	LPC_SC->PCLKSEL0 |= (3<<24);  	//CCLK/8
	LPC_ADC->ADCR &=~(255 << 8);  	//[15:8] CLKDIV
	LPC_ADC->ADCR |= (1 << 0);   	//Canal
	LPC_ADC->ADCR &= ~(1 << 16);   	//Modo burst desactivado
	LPC_ADC->ADCR |= (1 << 26);    	//Start por el Match 0.1
	LPC_ADC->ADCR &= ~(3 << 24);   	//0b100
	LPC_ADC->ADCR |= (1 << 27);    	//Por flanco de bajada

	LPC_ADC->ADINTEN |= (1<<0); //habilita interrupcion por canal 0

	NVIC_EnableIRQ(ADC_IRQn);
	NVIC_SetPriority(ADC_IRQn, 3);

	return;
}

void ADC_IRQHandler(){

	velocidad = ((LPC_ADC->ADDR0)>>4) & 0xFFF; //Variable auxiliar para observar el valor del registro de captura
	return;
}


void configTimer0(){
	TIM_TIMERCFG_Type struct_config;
	TIM_MATCHCFG_Type struct_match;

	struct_config.PrescaleOption = TIM_PRESCALE_USVAL;
	struct_config.PrescaleValue = 1000;

	struct_match.MatchChannel = 1;
	struct_match.IntOnMatch = DISABLE;
	struct_match.ResetOnMatch = ENABLE;
	struct_match.StopOnMatch = DISABLE;
	struct_match.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	struct_match.MatchValue = 250; //cada medio segundo actualiza el valor
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &struct_config);
	TIM_ConfigMatch(LPC_TIM0, &struct_match);

	TIM_Cmd(LPC_TIM0, ENABLE);

	return;
}


void configGPDMA(uint32_t* mensaje, uint32_t size){
	GPDMA_Channel_CFG_Type GPDMACfg;
	GPDMA_Init();
	GPDMACfg.ChannelNum = 0;
	GPDMACfg.SrcMemAddr = mensaje;
	GPDMACfg.DstMemAddr = 0;
	GPDMACfg.TransferSize = size;
	GPDMACfg.TransferWidth = 0;
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
	GPDMACfg.SrcConn = 0;
	GPDMACfg.DstConn = GPDMA_CONN_UART2_Tx;
	GPDMACfg.DMALLI = 0;

	GPDMA_Setup(&GPDMACfg);

	GPDMA_ChannelCmd(0,ENABLE);
	return;
}


void configMotor(){
	GPIO_SetDir(0,0xF << 6, OUTPUT); //para motor 0
	GPIO_SetDir(0,3, OUTPUT);
	GPIO_SetDir(0,3 << 17, OUTPUT);
	return;
}



void EINT3_IRQHandler(){

	NVIC_DisableIRQ(EINT3_IRQn);
	TIM_Cmd(LPC_TIM1,DISABLE);
	NVIC_DisableIRQ(TIMER1_IRQn);
	Key();
	delay2();
	NVIC_EnableIRQ(EINT3_IRQn);
	TIM_Cmd(LPC_TIM1,ENABLE);
	NVIC_EnableIRQ(TIMER1_IRQn);
	GPIO_ClearInt(PORT_2,0x00F0);

	return;
}

void giroGrados(uint8_t motor, uint16_t grados, uint8_t sentido) {

	uint32_t pasos;
	pasos = (grados/GRADOS_PASO_MOTOR)*4;

	if(sentido) {
		for(int i = 0; i < pasos; i++) {
			giroAntiHorario(motor);
		}
	} else {
		for(int i = 0; i < pasos; i++) {
			giroHorario(motor);
		}
	}

	mandarPosicion();
	return;
}

void giroHorario(uint8_t motor){
	switch(motor){
		case MOTOR0:
			switch(giroM0){
				case 0:
					GPIO_SetValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P2);
					GPIO_ClearValue(PUERTO_M0, M0_P3);
					break;
				case 1:
					GPIO_SetValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P2);
					GPIO_ClearValue(PUERTO_M0, M0_P3);
					break;
				case 2:
					GPIO_SetValue(PUERTO_M0, M0_P2);
					GPIO_ClearValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P3);
					break;
				case 3:
					GPIO_SetValue(PUERTO_M0, M0_P3);
					GPIO_ClearValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P2);
					break;
			}
			delayGiro();
			giroM0--;
			posiciones[MOTOR0] += 175;
			if(posiciones[MOTOR0] > 360000) {
			    posiciones[MOTOR0] -= 360000;
			}
			if(giroM0 == 255) {
				giroM0 = 3;
			}
			break;
		case MOTOR1:
			switch(giroM1){
				case 0:
					GPIO_SetValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P2);
					GPIO_ClearValue(PUERTO_M1, M1_P3);
					break;
				case 1:
					GPIO_SetValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P2);
					GPIO_ClearValue(PUERTO_M1, M1_P3);
					break;
				case 2:
					GPIO_SetValue(PUERTO_M1, M1_P2);
					GPIO_ClearValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P3);
					break;
				case 3:
					GPIO_SetValue(PUERTO_M1, M1_P3);
					GPIO_ClearValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P2);
					break;
			}
			delayGiro();
			giroM1--;
			posiciones[MOTOR1] += 175;
			if(posiciones[MOTOR1] > 360000) {
			    posiciones[MOTOR1] -= 360000;
			}
			if(giroM1 == 255) {
				giroM1 = 3;
			}
			break;

	}
	return;
}

void giroAntiHorario(uint8_t motor){
	switch(motor){
		case MOTOR0:
			switch(giroM0){
				case 0:
					GPIO_SetValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P2);
					GPIO_ClearValue(PUERTO_M0, M0_P3);
					break;
				case 1:
					GPIO_SetValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P2);
					GPIO_ClearValue(PUERTO_M0, M0_P3);
					break;
				case 2:
					GPIO_SetValue(PUERTO_M0, M0_P2);
					GPIO_ClearValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P3);
					break;
				case 3:
					GPIO_SetValue(PUERTO_M0, M0_P3);
					GPIO_ClearValue(PUERTO_M0, M0_P0);
					GPIO_ClearValue(PUERTO_M0, M0_P1);
					GPIO_ClearValue(PUERTO_M0, M0_P2);
					break;
			}
			delayGiro();
			giroM0++;
			if(posiciones[MOTOR0] < 175) {
			    posiciones[MOTOR0] += 360000;
			}
			posiciones[MOTOR0] -= 175;
			if(giroM0 == 4) {
				giroM0 = 0;
			}
			break;
		case MOTOR1:
			switch(giroM1){
				case 0:
					GPIO_SetValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P2);
					GPIO_ClearValue(PUERTO_M1, M1_P3);
					break;
				case 1:
					GPIO_SetValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P2);
					GPIO_ClearValue(PUERTO_M1, M1_P3);
					break;
				case 2:
					GPIO_SetValue(PUERTO_M1, M1_P2);
					GPIO_ClearValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P3);
					break;
				case 3:
					GPIO_SetValue(PUERTO_M1, M1_P3);
					GPIO_ClearValue(PUERTO_M1, M1_P0);
					GPIO_ClearValue(PUERTO_M1, M1_P1);
					GPIO_ClearValue(PUERTO_M1, M1_P2);
					break;
			}
			delayGiro();
			giroM1++;
			if(posiciones[MOTOR1] < 175) {
				posiciones[MOTOR1] += 360000;
			}
			posiciones[MOTOR1] -= 175;
			if(giroM1 == 4) {
				giroM1 = 0;
			}
			break;

	}
	return;
}


void Key() {

	uint8_t key = 16;
	key = getKey();

	if(key == 15) {
		Ejecutar();
	} else if (key >= 12){
		modificarModo(key);
	} else if(key < 12){
		switch(MODO){
			case MODO_MANUAL:
				keyManual(key);
				break;
			case MODO_AUTOMATICO:
				keyAutomatico(key);
				break;
			case MODO_GUARDAR:
				keyGuardar(key);
				break;
		}
	}
	return;
}

void keyManual(uint8_t key) {

	uint16_t gradosMax = 0;

	switch(key){
		case 2:
			while(GPIO_ReadValue(PORT_2) & (1<<5)){
				gradosMax++;
				if(gradosMax > 2048){
					break;
				}
				giroHorario(MOTOR1);

			}
			break;
		case 4:		//Antihorario base
			while(GPIO_ReadValue(PORT_2) & (1<<4)){
				gradosMax++;
				if(gradosMax > 2048){
					break;
				}
				giroAntiHorario(MOTOR0);
			}
			break;
		case 6:		//Horario base
			while(GPIO_ReadValue(PORT_2) & (1<<6)){
				gradosMax++;
				if(gradosMax > 2048){
					break;
				}
				giroHorario(MOTOR0);
			}
			break;
		case 8:		//Horario codo
			while(GPIO_ReadValue(PORT_2) & (1<<5)){
				gradosMax++;
				if(gradosMax > 2048){
					break;
				}
				giroAntiHorario(MOTOR1);
			}
			break;
	}

	while(GPIO_ReadValue(PORT_2) & (7<<4)); //esperamos que suelte la tecla

	mandarPosicion();
	return;
}

void keyAutomatico(uint8_t key) {
	static uint32_t valor = 0;
	static uint8_t sentido = 0;
	valor = valor*10;		//Movemos una posicion a la izquierda valor (1 -> 10) para poder poner el siguiente digito
	switch(key){
		case 0:
			if(valor == 0) {
				sentido = 1;
			}
			valor += 0;
			break;
		case 1:
			valor += 1;
			break;
		case 2:
			valor += 2;
			break;
		case 3:
			valor += 3;
			break;
		case 4:
			valor += 4;
			break;
		case 5:
			valor += 5;
			break;
		case 6:
			valor += 6;
			break;
		case 7:
			valor += 7;
			break;
		case 8:
			valor += 8;
			break;
		case 9:
			valor += 9;
			break;
		case 10:
			valor = valor/10;		//Movemos una posicion a la derecha (10 -> 1) porque no añadimos un digito en este caso
			if(valor == 0) {		//Motor 0
				MOTOR = MOTOR0;
			} else if(valor == 1) {	//Motor 1
				MOTOR = MOTOR1;
			}
			mandarCambioMotor();
			valor = 0;		//Reseteamos el valor
			break;
		case 11:
			//valor = valor/10;		//Movemos una posicion a la derecha (10 -> 1) porque no añadimos un digito en este caso
			if(valor > 3600){
				valor = 3600;
			}
			giroGrados(MOTOR, valor, sentido);		//Hacemos el giro
			valor = 0;		//Reseteamos el valor
			sentido = 0;
			break;
	}
	return;
}

void keyGuardar(uint8_t key) {
	static uint32_t valor = 0;
	static uint8_t sentido = 0;
		valor = valor*10;		//Movemos una posicion a la izquierda valor (1 -> 10) para poder poner el siguiente digito
		switch(key){
			case 0:
				if(valor == 0) { // si es el primer digito que se ingresa
					sentido = 1;
				}
				valor += 0;
				break;
			case 1:
				valor += 1;
				break;
			case 2:
				valor += 2;
				break;
			case 3:
				valor += 3;
				break;
			case 4:
				valor += 4;
				break;
			case 5:
				valor += 5;
				break;
			case 6:
				valor += 6;
				break;
			case 7:
				valor += 7;
				break;
			case 8:
				valor += 8;
				break;
			case 9:
				valor += 9;
				break;
			case 10:
				valor = valor/10;		//Movemos una posicion a la derecha (10 -> 1) porque no añadimos un digito en este caso
				if(valor == 0) {		//Motor 0
					MOTOR = MOTOR0;
				} else if(valor == 1) {	//Motor 1
					MOTOR = MOTOR1;
				}
				mandarCambioMotor();
				valor = 0;		//Reseteamos el valor
				break;
			case 11:
				//valor = valor/10;		//Movemos una posicion a la derecha (10 -> 1) porque no añadimos un digito en este caso
				if(valor > 3600){
					valor = 3600;
				}
				guardarValor(MOTOR, valor, sentido);		//Hacemos el giro
				valor = 0;		//Reseteamos el valor
				sentido = 0;
				break;
		}
	return;
}

void Ejecutar() {
	for(uint8_t i = 0; i < posicionValor; i++) {
		giroGrados(motorGuardado[i], valoresGuardados[i], sentidosGuardados[i]);
	}
	return;
}

void guardarValor(uint8_t motor, uint32_t valor, uint8_t sentido) {
	valoresGuardados[posicionValor] = valor;
	motorGuardado[posicionValor] = motor;
	sentidosGuardados[posicionValor] = sentido;
	posicionValor++;
	return;
}

void modificarModo(uint8_t key){
	switch(key){
		case 12:
			MODO = MODO_MANUAL;
			break;
		case 13:
			MODO = MODO_AUTOMATICO;
			break;
		case 14:
			MODO = MODO_GUARDAR;
			posicionValor = 0;
			break;
	}
	mandarCambioModo();
	return;
}



void delay2(){

	for (int i = 0; i < 4000000; i++); //aproximadamente 440ms

	return;
}

void delayGiro(){

	uint32_t delayHz = 0.06*velocidad + 50;
	uint32_t delayTicks = SystemCoreClock/delayHz;

	for (uint32_t i = 0; i < (delayTicks/11); i++)
	{
		;
	}
	prueba = delayHz;
	return;
}

void mandarPosicion(){
	clearScreen();
	static uint8_t infoGrupo[] = { "GRUPO 1 - Brazo Robotico \r\n Integrantes: \r\n\t Gina Commisso \r\n\t Ignacio Borgatello \r\n\t Ignacio Javier Goity \r\n ----------------------------------------------------- \r\n" };
	configGPDMA(infoGrupo, sizeof(infoGrupo));


	uint8_t infoModo[] = { "Modo seleccionado: MODO MANUAL \r\n"};

	switch(MODO) {
		case MODO_MANUAL:
			{
				uint8_t infoModo[] = { "Modo seleccionado: MODO MANUAL \r\n"};
				break;
			}
		case MODO_AUTOMATICO:
			{
				uint8_t infoModo[] = { "Modo seleccionado: MODO AUTOMATICO \r\n"};
				break;
			}
		case MODO_GUARDAR:
			{
				uint8_t infoModo[] = { "Modo seleccionado: MODO GUARDAR \r\n"};
				break;
			}
	}
	configGPDMA(infoModo, sizeof(infoModo));

	static uint8_t motor0[] = {"MOTOR 0: "};
    static uint8_t motor1[] = {"MOTOR 1: "};
    static uint8_t posMotor0[3];
    static uint8_t posMotor1[3];
    posiciones[0] = posiciones[0]/1000;
    posiciones[1] = posiciones[1]/1000;
    // 0 = 48
    posMotor0[0] = (posiciones[0]/100) + 48;    // posiciones/100 saca los primeros 2 digitos -> 345/100 = 3
    posMotor1[0] = (posiciones[1]/100) + 48;
    posMotor0[1] = ((posiciones[0]/10)%10) + 48;// (posiciones/10)%10 saca el ultimo y el primer digito -> 345/10 = 34%10 = 4
    posMotor1[1] = ((posiciones[1]/10)%10) + 48;
    posMotor0[2] = (posiciones[0]%10) + 48;        // posiciones%10 saca los ultimos 2 digitos -> 345%10 = 5
    posMotor1[2] = (posiciones[1]%10) + 48;
    for(uint8_t i = 0; i < 26; i++) {
       	if(i < 9) {
       		posicionesMotor[i] = motor0[i];
       	} else if(i < 12) {
       		posicionesMotor[i] = posMotor0[i-9];
       	} else if (i == 12) {
       		posicionesMotor[i] = '\r';
       		posicionesMotor[i+1] = '\n';
       	} else if (i < 22){
       		posicionesMotor[i+1] = motor1[i-13];
       	} else if (i < 25){
       		posicionesMotor[i+1] = posMotor1[i-22];
       	} else {
       		posicionesMotor[i+1] = '\r';
       		posicionesMotor[i+2] = '\n';
       	}
    }
    posiciones[0] = posiciones[0]*1000;
    posiciones[1] = posiciones[1]*1000;
    configGPDMA(posicionesMotor, sizeof(posicionesMotor));
	return;
}

void mandarCambioModo() {
	static uint8_t mensajeManual[13] = { 'M' , 'O' , 'D' , 'O' , ' ' , 'M' , 'A' , 'N' , 'U' , 'A' , 'L' ,'\r', '\n' };
	static uint8_t mensajeAutomatico[17] = { 'M' , 'O' , 'D' , 'O' , ' ' , 'A' , 'U' , 'T' , 'O' , 'M' , 'A' , 'T' , 'I' , 'C' , 'O' ,'\r', '\n'};
	static uint8_t mensajeGuardar[14] = { 'M' , 'O' , 'D' , 'O' , ' ' , 'G' , 'U' , 'A' , 'R' , 'D' , 'A' , 'R' ,'\r', '\n' };
	switch(MODO) {
	case MODO_MANUAL:
		configGPDMA(mensajeManual, sizeof(mensajeManual));
		break;
	case MODO_AUTOMATICO:
		configGPDMA(mensajeAutomatico, sizeof(mensajeAutomatico));
		break;
	case MODO_GUARDAR:
		configGPDMA(mensajeGuardar, sizeof(mensajeGuardar));
		break;
	}

	GPDMA_ChannelCmd(0, ENABLE);
	return;
}

void mandarCambioMotor() {
	static uint8_t mensajeMotor0[] = { 'M', '0','\r', '\n'};
	static uint8_t mensajeMotor1[] = { 'M', '1','\r', '\n'};
	switch(MOTOR) {
		case MOTOR0:
			configGPDMA(mensajeMotor0, sizeof(mensajeMotor0));
			break;
		case MOTOR1:
			configGPDMA(mensajeMotor1, sizeof(mensajeMotor1));
			break;
	}
	GPDMA_ChannelCmd(0, ENABLE);
	return;
}

void mandarInfoGrupo() {
    static uint8_t infoGrupo[] = { "GRUPO 1 - Brazo Robotico \r\n Integrantes: \r\n\t Gina Commisso \r\n\t Ignacio Borgatello \r\n\t Ignacio Javier Goity \r\n ----------------------------------------------------- \r\n" };
    configGPDMA(infoGrupo, sizeof(infoGrupo));
    return;
}

void clearScreen() {
	uint8_t mensaje[] = {'\f'};  //limpia pantalla en emuladores de terminales
    configGPDMA(mensaje, sizeof(mensaje));
    return;
}
