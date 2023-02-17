 // This header file was generated on
// z5214348.web.cse.unsw.edu.au/header_generator/

// header guard: https://en.wikipedia.org/wiki/Include_guard
// This avoids errors if this file is included multiple times
// in a complex source file setup

#ifndef FSM_DDR_H
#define FSM_DDR_H

// #includes

#include "fsm.h"
#include <stdlib.h>	//para el NULL
#include <unistd.h>
#include "xparameters.h"
#include "xgpio.h"

// #defines

#define GPIO_Addr_rdy_ID		XPAR_GPIO_0_DEVICE_ID
#define GPIO_Data_ID   			XPAR_GPIO_1_DEVICE_ID
#define GPIO_Valid_Req_ID  		XPAR_GPIO_2_DEVICE_ID
#define CHANNEL_1	 			1
#define CHANNEL_2 				2
#define INPUT					0xFF
#define OUTPUT					0x00

// Variables
//extern int start;
int end;

// Functions

int inicialize_gpio();
void writte_ddr(u32 *Addr, u32 Data);
u32 read_ddr(u32 *Addr);
void start_isr (void);
/*static void accion_idle	(fsm_t*	this);
static void accion_OP_req	(fsm_t*	this);
static void accion_Wait_rdy	(fsm_t*	this);
static void accion_Read_data	(fsm_t*	this);
static void accion_Write_data	(fsm_t*	this);*/

// End of header file
#endif
