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
//  13/06/22
//
/***************************** Include Files *********************************/

#include "xparameters.h"
#include "xil_printf.h"
#include <stdio.h>
#include "xil_exception.h"
#include "xuartlite.h"
#include "xintc.h"
#include "xgpio_l.h"
#include "xstatus.h"
//#include "platform.h"
#include "xgpio.h"

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

#define GPIO_buttons_ID			XPAR_GPIO_0_DEVICE_ID
#define CHANNEL_1	 			1
#define CHANNEL_2 				2
#define INPUT					0xFF
#define OUTPUT					0x00

/*
 * The following constant controls the length of the buffers to be sent
 * and received with the UartLite device.
 */
#define TEST_BUFFER_SIZE        512
//#define RECV_BUFFER_SIZE 		1000
#define FILE_SIZE     		    4096 //16384 B


/************************** Function Prototypes ******************************/

int SetupUartLite(u16 DeviceId);

int SetupInterruptSystem(XUartLite *UartLitePtr);

void SendHandler(void *CallBackRef, unsigned int EventData);

void RecvHandler(void *CallBackRef, unsigned int EventData);

int UartLitePolled(u16 DeviceId);

void writte_ddr(u32 *Addr, u32 Data);

u32 read_ddr(u32 *Addr);

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

 u8 RecvFIFO[TEST_BUFFER_SIZE];	/* Buffer for Receiving Data */

 /* Here are the pointers to the buffer */
 u8* ReceiveBufferPtr = &ReceiveBuffer[0];
 u8* CommandPtr       = &ReceiveBuffer[0];

 /*
  * The following counters are used to determine when the entire buffer has
  * been sent and received.
  */
 static volatile int TotalReceivedCount;
 static volatile int TotalSentCount;

 /* DDR2 memory address, initialised */
 static u32 addr = DDR_BASEADDR;

 /*
  * The following are declared globally so they are zeroed and so they are
  * easily accessible from a debugger
  */

 XGpio GPIO_buttons; /* The Instance of the GPIO Driver */

int main()
{
	//Variable definitions
	int Status=0;
	u8 button = 0;

	/* Initialize the GPIO driver */
	Status = XGpio_Initialize(&GPIO_buttons, GPIO_buttons_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Gpio Initialization Failed\r\n");
		return XST_FAILURE;
	}

	/* Set the direction for all signals */
	XGpio_SetDataDirection(&GPIO_buttons, 1, OUTPUT);


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

	while(1){

	  xil_printf("\n\r");
      xil_printf("Options ---------------------\n\r");
      //xil_printf("BTNL - Start DDR test\n\r"); 					//0x04
      xil_printf("BTNC - See DDR2 data values\n\r");				//0x01
      xil_printf("BTNR - Start loading data into the DDR\n\r"); 	//0x08
      xil_printf("\n\r");

      //Check to make sure there is a new command to run
      while (button == 0){ //button = 0 when no button is pressed
    	  button = XGpio_DiscreteRead(&GPIO_buttons, 1);
    	  for(int delay = 0; delay < 500000; delay++){}//delay 5ms (fclk= 100MHz) (Debouncer)
      };

      if (button == 0x01){
		  //Variables
		  u32 addr = DDR_BASEADDR;
		  u32 data;
		  int i = 0;

		  while(1){
			  data = read_ddr((u32*)addr);
			  xil_printf("Addr : %08X\n\r", addr);
			  xil_printf("Data : %08X\n\r", data);
			  xil_printf("\n\r");
			  addr = addr + 4;
			  i++;

			  if(i == 10){
				  break;
			  }
		  }

      } else if (button == 0x08){
    	  //Send file to the DDR2
    	  xil_printf("  -> Select a file from to send to the FPGA\n\r");
    	  Status = UartLitePolled(UARTLITE_DEVICE_ID);

      } else {
		  xil_printf("Not a valid command!\n\r");
	  }


      //Increment to the next command
      button = 0;
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

/****************************************************************************/
/**
* This function receives data through the uart and writes it to the ddr2 memory afterwards.
*
* This function polls the UartLite and does not require the use of interrupts.
*
* @param	DeviceId is the Device ID of the UartLite and is the
*		XPAR_<uartlite_instance>_DEVICE_ID value from xparameters.h.
*
* @return	XST_SUCCESS if successful, XST_FAILURE if unsuccessful.
*
*
* @note
*
* This function is in testing
*
****************************************************************************/
int UartLitePolled(u16 DeviceId)
{
	int Status;
	unsigned int ReceivedCount = 0;
	int Index;
	u32 data = 0;

	/*
	 * Initialize the UartLite driver so that it is ready to use.
	 */
	Status = XUartLite_Initialize(&UartLite, DeviceId);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Initialize the the receive buffer bytes to zero.
	 */
	for (Index = 0; Index < TEST_BUFFER_SIZE; Index++) {
		RecvFIFO[Index] = 0;
	}


	/*
	 * Receive the number of bytes which is transfered.
	 * Data may be received in fifo with some delay hence we continuously
	 * check the receive fifo for valid data and update the receive buffer
	 * accordingly.
	 */
	int i = 0;
	int end = 0;

	while (1) {
		ReceivedCount += XUartLite_Recv(&UartLite,
					   RecvFIFO + ReceivedCount,
					   TEST_BUFFER_SIZE - ReceivedCount);

		//if (((ReceivedCount % 4) == 0) && (ReceivedCount != 0)) {
			//for (Index = 0; Index < 8; Index++) {

		if (ReceivedCount == TEST_BUFFER_SIZE){
			for (Index = 0; Index < TEST_BUFFER_SIZE; Index++) {
				if ((((Index+1) % 4) == 0) && (Index != 0)){
					data = (RecvFIFO[Index]<<24) + (RecvFIFO[Index-1]<<16) + (RecvFIFO[Index-2]<<8) + RecvFIFO[Index-3];
					writte_ddr((u32*)addr, data);
					i++;
					addr = addr + 4;

					if (i == FILE_SIZE){
						xil_printf("\n\r");
						xil_printf("%d Bytes received\n\r", i*4); //i = n ---> 4nB recibidos
						xil_printf("DATA TRANSFER IS COMPLETED!!\n\r");
						xil_printf("\n\r");
						end = 1;
					}
				}
			}

			ReceivedCount = 0;
		}
		if (addr > DDR_HIGHADDR){
			xil_printf("DDR is FULL!!\n\r");
			return XST_FAILURE;
		}
		if(end == 1){
			break;
		}
	}

	return XST_SUCCESS;
}

void writte_ddr(u32 *Addr, u32 Data) {*Addr = Data;}
u32 read_ddr(u32 *Addr) {return *Addr;}
