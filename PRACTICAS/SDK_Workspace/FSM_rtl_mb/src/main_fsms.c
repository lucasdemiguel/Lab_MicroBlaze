//NAME:
//  main.c
//
//PURPOSE:
//  This C file is meant to be built with the Xilinx SDK and run as an
//  ELF file on a Microblaze processor.
//
//AUTHOR:
//  Lucas de Miguel
//  Note: Code uses Xilinx examples included with Vivado.
//
//DATE:
//  10/06/22
//
/***************************** Include Files *********************************/

#include "xparameters.h"
#include "xil_printf.h"
#include <stdio.h>
#include "xil_exception.h"
#include "xuartlite.h"
#include "xintc.h"
#include "xgpio_l.h"

//#include "platform.h"
#include "xgpio.h"
#include "fsm.h"
#include "fsm_ddr.h"

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define UARTLITE_DEVICE_ID      XPAR_UARTLITE_0_DEVICE_ID
#define INTC_DEVICE_ID          XPAR_INTC_0_DEVICE_ID
#define UARTLITE_INT_IRQ_ID     XPAR_INTC_0_UARTLITE_0_VEC_ID

#define DDR_BASEADDR		    XPAR_MIG_7SERIES_0_BASEADDR
#define DDR_HIGHADDR		    XPAR_MIG_7SERIES_0_HIGHADDR

#define GPIO_req_rdy_ID			XPAR_GPIO_1_DEVICE_ID
//#define GPIO_Data_ID   			XPAR_GPIO_0_DEVICE_ID
#define GPIO_Valid_Addr_ID 		XPAR_GPIO_2_DEVICE_ID
#define CHANNEL_1	 			1
#define CHANNEL_2 				2
#define INPUT					0xFF
#define OUTPUT					0x00

/*
 * The following constant controls the length of the buffers to be sent
 * and received with the UartLite device.
 */
#define TEST_BUFFER_SIZE        500


/************************** Function Prototypes ******************************/

int SetupUartLite(u16 DeviceId);

int SetupInterruptSystem(XUartLite *UartLitePtr);

void SendHandler(void *CallBackRef, unsigned int EventData);

void RecvHandler(void *CallBackRef, unsigned int EventData);

/************************** Variable Definitions *****************************/

 XUartLite UartLite;             /* The instance of the UartLite Device */
 XUartLite_Config *UartLite_Cfg; /* The instance of the UartLite Config */
 XIntc InterruptController;      /* The instance of the Interrupt Controller */

 /*
  * The following buffers are used in this example to send and receive data
  * with the UartLite.
  */
 u8 SendBuffer[TEST_BUFFER_SIZE];
 u8 ReceiveBuffer[TEST_BUFFER_SIZE];

 /* Here are the pointers to the buffer */
 u8* ReceiveBufferPtr = &ReceiveBuffer[0];
 u8* CommandPtr       = &ReceiveBuffer[0];

 /*
  * The following counters are used to determine when the entire buffer has
  * been sent and received.
  */
 static volatile int TotalReceivedCount;
 static volatile int TotalSentCount;

 /*
  * The following are declared globally so they are zeroed and so they are
  * easily accessible from a debugger
  */

 XGpio GPIO_req_rdy; /* The Instance of the GPIO Driver */
 XGpio GPIO_Data; /* The Instance of the GPIO Driver */
 XGpio GPIO_Valid_Addr; /* The Instance of the GPIO Driver */

 //FSM
 fsm_t* fsm_ddr2();

int main()
{
	//FSM definition
	fsm_t* fsm = fsm_ddr2();
	//fsm_fire(fsm);

	//Variable definitions
	int Status=0;

    //Set up the UART and configure the interrupt handler for bytes in RX buffer
	Status = SetupUartLite(UARTLITE_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	//Get a reference pointer to the Uart Configuration
	UartLite_Cfg = XUartLite_LookupConfig(UARTLITE_DEVICE_ID);

	//Print out the info about our XUartLite instance
	xil_printf("\n\r");
    xil_printf("Serial Port Properties ------------------\n\r");
	xil_printf("Device ID : %d\n\r", UartLite_Cfg->DeviceId);
	xil_printf("Baud Rate : %d\n\r", UartLite_Cfg->BaudRate);
	xil_printf("Data Bits : %d\n\r", UartLite_Cfg->DataBits);
	xil_printf("Base Addr : %08X\n\r", UartLite_Cfg->RegBaseAddr);
	xil_printf("\n\r");

	//Inicialize GPIO
	inicialize_gpio();

	//Precharge data in ddr2
	u32 data = 0x11001101;
	u32 addr = DDR_BASEADDR;
	writte_ddr((u32*)addr,   data);
	xil_printf("Addr : %08X\n\r", addr);
	data = read_ddr((u32*)addr);
	xil_printf("Data : %08X\n\r", data);
	xil_printf("\n\r");
	data = 0x10101010;
	addr = DDR_BASEADDR + 4;
	writte_ddr((u32*)addr,   data);
	xil_printf("Addr : %08X\n\r", addr);
	data = read_ddr((u32*)addr);
	xil_printf("Data : %08X\n\r", data);

	xil_printf("\n\r");
	addr = DDR_BASEADDR + 8;
	data = read_ddr((u32*)addr);
	xil_printf("Data DDR2: %08X\n\r", data);
	xil_printf("Addr DDR2: %08X\n\r", addr);

	/*xil_printf("\n\r");
	addr = DDR_BASEADDR;
	data = read_ddr((u32*)addr);
	xil_printf("Addr 1 : %08X\n\r", addr);
	xil_printf("Data 1 : %08X\n\r", data);
	xil_printf("\n\r");
	addr = DDR_BASEADDR + 4;
	data = read_ddr((u32*)addr);
	xil_printf("Addr 2 : %08X\n\r", addr);
	xil_printf("Data 2 : %08X\n\r", data);*/

	while(1){

	  fsm_fire(fsm);

	  //Print out the current contents of the buffer
      xil_printf("\n\r");
      xil_printf("Options ---------------------\n\r");
      xil_printf("a - Start DDR test\n\r");
      xil_printf("b - See ddr data values\n\r");
      xil_printf("\n\r");

      //Check to make sure there is a new command to run
      while (ReceiveBufferPtr <= CommandPtr){};

      if (*CommandPtr == 'a'){
    	  start_isr();
    	  //start = 1;
    	  xil_printf("Starting DDR test\n\r");
    	  while(end == 0){
    		  fsm_fire(fsm);
    	  }
      } else if (*CommandPtr == 'b'){
		  //Variables
		  u32 addr;
		  u32 data;
		  u32 data1;
		  u32 data2;

		  addr = DDR_BASEADDR;
		  data1 = read_ddr((u32*)addr);
		  xil_printf("Addr 1 : %08X\n\r", addr);
		  xil_printf("Data 1 : %08X\n\r", data1);
		  xil_printf("\n\r");
		  addr = DDR_BASEADDR + 4;
		  data2 = read_ddr((u32*)addr);
		  xil_printf("Addr 2 : %08X\n\r", addr);
		  xil_printf("Data 2 : %08X\n\r", data2);
		  xil_printf("\n\r");
		  data = data1 & data2;
		  xil_printf("Data : %08X\n\r", data);
		  addr = DDR_BASEADDR + 8;
		  data = read_ddr((u32*)addr);
		  xil_printf("Data DDR2: %08X\n\r", data);
		  xil_printf("Addr DDR2: %08X\n\r", addr);

      } else {
		  xil_printf("Not a valid command!\n\r");
	  }


      //Increment to the next command
      CommandPtr++;
    }

	//End of program
	return XST_SUCCESS;
}


/****************************************************************************/
/**
*
* This function does a minimal test on the UartLite device and driver as a
* design example. The purpose of this function is to illustrate
* how to use the XUartLite component.
*
* This function sends data and expects to receive the same data through the
* UartLite. The user must provide a physical loopback such that data which is
* transmitted will be received.
*
* This function uses interrupt driver mode of the UartLite device. The calls
* to the UartLite driver in the handlers should only use the non-blocking
* calls.
*
* @param	DeviceId is the Device ID of the UartLite Device and is the
*		XPAR_<uartlite_instance>_DEVICE_ID value from xparameters.h.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note
*
* This function contains an infinite loop such that if interrupts are not
* working it may never return.
*
****************************************************************************/
int SetupUartLite(u16 DeviceId)
{
	int Status;
	int Index;

	/*
	 * Initialize the UartLite driver so that it's ready to use.
	 */
	Status = XUartLite_Initialize(&UartLite, DeviceId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Perform a self-test to ensure that the hardware was built correctly.
	 */
	Status = XUartLite_SelfTest(&UartLite);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect the UartLite to the interrupt subsystem such that interrupts can
	 * occur. This function is application specific.
	 */
	Status = SetupInterruptSystem(&UartLite);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Setup the handlers for the UartLite that will be called from the
	 * interrupt context when data has been sent and received, specify a
	 * pointer to the UartLite driver instance as the callback reference so
	 * that the handlers are able to access the instance data.
	 */
	XUartLite_SetSendHandler(&UartLite, SendHandler, &UartLite);
	XUartLite_SetRecvHandler(&UartLite, RecvHandler, &UartLite);

	/*
	 * Enable the interrupt of the UartLite so that interrupts will occur.
	 */
	XUartLite_EnableInterrupt(&UartLite);

	/*
	 * Initialize the send buffer bytes with a pattern to send and the
	 * the receive buffer bytes to zero to allow the receive data to be
	 * verified.
	 */
	for (Index = 0; Index < TEST_BUFFER_SIZE; Index++) {
		SendBuffer[Index] = 1;
		ReceiveBuffer[Index] = 0;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is the handler which performs processing to send data to the
* UartLite. It is called from an interrupt context such that the amount of
* processing performed should be minimized. It is called when the transmit
* FIFO of the UartLite is empty and more data can be sent through the UartLite.
*
* This handler provides an example of how to handle data for the UartLite,
* but is application specific.
*
* @param	CallBackRef contains a callback reference from the driver.
*		In this case it is the instance pointer for the UartLite driver.
* @param	EventData contains the number of bytes sent or received for sent
*		and receive events.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
void SendHandler(void *CallBackRef, unsigned int EventData)
{
	TotalSentCount = EventData;
}

/****************************************************************************/
/**
*
* This function is the handler which performs processing to receive data from
* the UartLite. It is called from an interrupt context such that the amount of
* processing performed should be minimized.  It is called data is present in
* the receive FIFO of the UartLite such that the data can be retrieved from
* the UartLite. The size of the data present in the FIFO is not known when
* this function is called.
*
* This handler provides an example of how to handle data for the UartLite,
* but is application specific.
*
* @param	CallBackRef contains a callback reference from the driver, in
*		this case it is the instance pointer for the UartLite driver.
* @param	EventData contains the number of bytes sent or received for sent
*		and receive events.
*
* @return	None.
*
* @note		None.
*
****************************************************************************/
void RecvHandler(void *CallBackRef, unsigned int EventData)
{
	XUartLite_Recv(&UartLite, ReceiveBufferPtr, 1);
	ReceiveBufferPtr++;
	TotalReceivedCount++;

	//If we've reached the end of the buffer, start over
    if (ReceiveBufferPtr >= (&ReceiveBuffer[0] + TEST_BUFFER_SIZE)){
      xil_printf("Resetting Receive Buffer. Please enter a new command!\n\r");
      ReceiveBufferPtr = &ReceiveBuffer[0];
      CommandPtr = &ReceiveBuffer[0];
      TotalReceivedCount = 0;
    }

}

/****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for the UartLite device. This function is application specific since the
* actual system may or may not have an interrupt controller. The UartLite
* could be directly connected to a processor without an interrupt controller.
* The user should modify this function to fit the application.
*
* @param    UartLitePtr contains a pointer to the instance of the UartLite
*           component which is going to be connected to the interrupt
*           controller.
*
* @return   XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note     None.
*
****************************************************************************/
int SetupInterruptSystem(XUartLite *UartLitePtr)
{

	int Status;

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	Status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect a device driver handler that will be called when an interrupt
	 * for the device occurs, the device driver handler performs the
	 * specific interrupt processing for the device.
	 */
	Status = XIntc_Connect(&InterruptController, UARTLITE_INT_IRQ_ID,
			   (XInterruptHandler)XUartLite_InterruptHandler,
			   (void *)UartLitePtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the interrupt controller such that interrupts are enabled for
	 * all devices that cause interrupts, specific real mode so that
	 * the UartLite can cause interrupts through the interrupt controller.
	 */
	Status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupt for the UartLite device.
	 */
	XIntc_Enable(&InterruptController, UARTLITE_INT_IRQ_ID);

	/*
	 * Initialize the exception table.
	 */
	Xil_ExceptionInit();

	/*
	 * Register the interrupt controller handler with the exception table.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			 (Xil_ExceptionHandler)XIntc_InterruptHandler,
			 &InterruptController);

	/*
	 * Enable exceptions.
	 */
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}
