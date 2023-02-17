#include "fsm.h"
#include <stdlib.h>	//para el NULL
#include <unistd.h>
//Includes MicroBlaze
#include "xparameters.h"
#include "xgpio.h"

//Variables Microblaze
#define GPIO_req_rdy_ID			XPAR_GPIO_1_DEVICE_ID
#define GPIO_Data_ID   			XPAR_GPIO_0_DEVICE_ID
#define GPIO_Valid_Addr_ID 		XPAR_GPIO_2_DEVICE_ID
#define CHANNEL_1	 			1
#define CHANNEL_2 				2
#define INPUT					0xFF
#define OUTPUT					0x00

/************************** Function Prototypes ******************************/
int inicialize_gpio();

void writte_ddr(u32 *Addr, u32 Data);

u32 read_ddr(u32 *Addr);

void start_isr (void);

/************************** Variable Definitions *****************************/

XGpio GPIO_req_rdy; 	/* The Instance of the GPIO Driver */
XGpio GPIO_Data; 		/* The Instance of the GPIO Driver */
XGpio GPIO_Valid_Addr;	/* The Instance of the GPIO Driver */

/************************** Functions ******************************/
int inicialize_gpio(){
    //Variable definitions
	int Status=0;

	/* Initialize the GPIO driver */
	Status = XGpio_Initialize(&GPIO_req_rdy, GPIO_req_rdy_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}
	Status = XGpio_Initialize(&GPIO_Data, GPIO_Data_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}
	Status = XGpio_Initialize(&GPIO_Valid_Addr, GPIO_Valid_Addr_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}

	/* Set the direction for all signals */
	XGpio_SetDataDirection(&GPIO_req_rdy, CHANNEL_1, OUTPUT);
	XGpio_SetDataDirection(&GPIO_req_rdy, CHANNEL_2, INPUT);
	XGpio_SetDataDirection(&GPIO_Data, CHANNEL_1, OUTPUT);
	XGpio_SetDataDirection(&GPIO_Data, CHANNEL_2, INPUT);
	XGpio_SetDataDirection(&GPIO_Valid_Addr, CHANNEL_1, OUTPUT);
	XGpio_SetDataDirection(&GPIO_Valid_Addr, CHANNEL_2, INPUT);

	return Status;
}

void writte_ddr(u32 *Addr, u32 Data) {*Addr = Data;}
u32 read_ddr(u32 *Addr) {return *Addr;}

/************************** FSM ******************************/
enum fsm_state {
    idle,
    OP_req_st,
    wait_rdy1,
    read_data1,
    wait_rdy2,
    read_data2,
	wait_rdy3,
    write_data,
	wait_write
};


//Entrada e interrupciones del sitema
static int ready = 0;
static int start = 0;
static u32 Addr = 0;
static u32 data_in = 0;
static u32 data_ddr2 = 0;

//Variable auxiliar
int end = 0;

void start_isr (void) {start=1;}

static int get_start(fsm_t* this){
    if(start == 1){
        start = 0;
        return 1;
    }
    return 0;
	//return start;
}
static int get_ready(fsm_t* this){
    ready = XGpio_DiscreteRead(&GPIO_req_rdy, CHANNEL_2);
    return ready;
}
static u32 get_addr(){
    Addr = XGpio_DiscreteRead(&GPIO_Valid_Addr, CHANNEL_2);
    return Addr;
}
static u32 get_data_in(){
    data_in = XGpio_DiscreteRead(&GPIO_Data, CHANNEL_2);
    return data_in;
}
static int ddr_valid(fsm_t* this){
    Addr = get_addr();
    data_ddr2 = read_ddr((u32*)Addr);
    if (data_ddr2 == *((u32*)Addr)){
        return 1;
    }
    return 0;
}
static int transition_true(fsm_t* this){return 1;}

//Salidas del sistema
static u32 data_out;
static int OP_req;
static int valid;

//Acciones de nuestro sistema
static void accion_idle	(fsm_t*	this)	{
    data_out = read_ddr((u32*)0xFFFFFFFF);
    OP_req = 0;
    valid = 0;
    //start = 0;
    XGpio_DiscreteWrite(&GPIO_Data, CHANNEL_1, data_out);
    XGpio_DiscreteWrite(&GPIO_req_rdy, CHANNEL_1, OP_req);
    XGpio_DiscreteWrite(&GPIO_Valid_Addr, CHANNEL_1, valid);

    //xil_printf("Idle");
    end = 0;
}

static void accion_OP_req	(fsm_t*	this)	{
    //data_out = read_ddr((u32*)0xFFFFFFFF);
    OP_req = 1;
    valid = 0;
    //XGpio_DiscreteWrite(&GPIO_Data, CHANNEL_1, data_out);
    XGpio_DiscreteWrite(&GPIO_req_rdy, CHANNEL_1, OP_req);
    XGpio_DiscreteWrite(&GPIO_Valid_Addr, CHANNEL_1, valid);

    //xil_printf("req");
    end = 0;
}

static void accion_Wait_rdy	(fsm_t*	this)	{
	//data_out = read_ddr((u32*)0xFFFFFFFF);
    OP_req = 0;
    valid = 0;
    //XGpio_DiscreteWrite(&GPIO_Data, CHANNEL_1, data_out);
    XGpio_DiscreteWrite(&GPIO_req_rdy, CHANNEL_1, OP_req);
    XGpio_DiscreteWrite(&GPIO_Valid_Addr, CHANNEL_1, valid);

    //xil_printf("wait");
    end = 0;
}

static void accion_Wait_rdy_write	(fsm_t*	this)	{
	//data_out = read_ddr((u32*)0xFFFFFFFF);
    OP_req = 0;
    valid = 1;
    //XGpio_DiscreteWrite(&GPIO_Data, CHANNEL_1, data_out);
    XGpio_DiscreteWrite(&GPIO_req_rdy, CHANNEL_1, OP_req);
    XGpio_DiscreteWrite(&GPIO_Valid_Addr, CHANNEL_1, valid);

    //xil_printf("wait");
    end = 1;
}

static void accion_Read_data	(fsm_t*	this)	{
    Addr = get_addr();
    data_out = read_ddr((u32*)Addr);
    //xil_printf("Addr : %08X\n\r", Addr);
    //xil_printf("Data : %08X\n\r", data_out);
    OP_req = 0;
    valid = 1;
    XGpio_DiscreteWrite(&GPIO_Data, CHANNEL_1, data_out);
    XGpio_DiscreteWrite(&GPIO_req_rdy, CHANNEL_1, OP_req);
    XGpio_DiscreteWrite(&GPIO_Valid_Addr, CHANNEL_1, valid);

    //xil_printf("read");
    end = 0;
}

static void accion_Write_data	(fsm_t*	this)	{
    Addr = get_addr();
    data_in = get_data_in();
    writte_ddr((u32*)Addr, data_in);
    //data_out = read_ddr((u32*)Addr);
    //xil_printf("Addr : %08X\n\r", Addr);
    //xil_printf("Data : %08X\n\r", data_out);
    OP_req = 0;
    valid = 0;
    //XGpio_DiscreteWrite(&GPIO_Data, CHANNEL_1, data_out);
    XGpio_DiscreteWrite(&GPIO_req_rdy, CHANNEL_1, OP_req);
    XGpio_DiscreteWrite(&GPIO_Valid_Addr, CHANNEL_1, valid);

    //xil_printf("write");
    end = 0;
}

//Lista de transiciones
//{Estado Origen, Condicion de disparo, Estado final, Acciones de transicion}
static  fsm_trans_t ddr2[] = {
		{idle, get_start, OP_req_st, accion_idle},
        {OP_req_st, transition_true, wait_rdy1, accion_OP_req},
        {wait_rdy1, get_ready, read_data1, accion_Wait_rdy},
        {read_data1, ddr_valid, wait_rdy2, accion_Read_data},
        {wait_rdy2, get_ready, read_data2, accion_Wait_rdy},
		//{wait_rdy2, transition_true, read_data2, accion_Wait_rdy},
		{read_data2, ddr_valid, wait_rdy3, accion_Read_data},
		{wait_rdy3, get_ready, write_data, accion_Wait_rdy},
		//{wait_rdy3, transition_true, write_data, accion_Wait_rdy},
        {write_data, get_ready, wait_write, accion_Write_data},
		{wait_write, ddr_valid, idle, accion_Wait_rdy_write},
        {-1, NULL, -1, NULL},           
};


fsm_t* fsm_ddr2() {
	fsm_t* fsm_ddr2 = fsm_new(ddr2);
    return fsm_ddr2;
}
