/*
 * imageProcess.c
 *
 *  Created on: Apr 13, 2020
 *      Author: VIPIN
 */

#include "imageProcess.h"
#include "sleep.h"
static void imageProcISR(void *CallBackRef);
static void dmaReceiveISR(void *CallBackRef);

/*****************************************************************************/
/**
 * This function copies the buffer data from image buffer to video buffer
 *
 * @param	displayHSize is Horizontal size of video in pixels
 * @param   displayVSize is Vertical size of video in pixels
 * @param	imageHSize is Horizontal size of image in pixels
 * @param   imageVSize is Vertical size of image in pixels
 * @param   hOffset is horizontal position in the video frame where image should be displayed
 * @param   vOffset is vertical position in the video frame where image should be displayed
 * @param   imagePointer pointer to the image buffer
 * @return
 * 		-  0 if successfully copied
 * 		- -1 if copying failed
 *****************************************************************************/
int drawImage(u32 displayHSize,u32 displayVSize,u32 imageHSize,u32 imageVSize,u32 hOffset, u32 vOffset,int numColors,char *imagePointer,char *videoFramePointer){
	Xil_DCacheInvalidateRange((u32)imagePointer,(imageHSize*imageVSize));
	for(int i=0;i<displayVSize;i++){
		for(int j=0;j<displayHSize;j++){
			if(i<vOffset || i >= vOffset+imageVSize){
				videoFramePointer[(i*displayHSize*3)+(j*3)]   = 0x00;
				videoFramePointer[(i*displayHSize*3)+(j*3)+1] = 0x00;
				videoFramePointer[(i*displayHSize*3)+(j*3)+2] = 0x00;
			}
			else if(j<hOffset || j >= hOffset+imageHSize){
				videoFramePointer[(i*displayHSize*3)+(j*3)]   = 0x00;
				videoFramePointer[(i*displayHSize*3)+(j*3)+1] = 0x00;
				videoFramePointer[(i*displayHSize*3)+(j*3)+2] = 0x00;
			}
			else {
				if(numColors==1){
					videoFramePointer[(i*displayHSize*3)+j*3]     = *imagePointer;
					videoFramePointer[(i*displayHSize*3)+(j*3)+1] = *imagePointer;
					videoFramePointer[(i*displayHSize*3)+(j*3)+2] = *imagePointer;
					imagePointer++;
				}
				else if(numColors==3){
					videoFramePointer[(i*displayHSize*3)+j*3]     = *imagePointer;
					videoFramePointer[(i*displayHSize*3)+(j*3)+1] = *(imagePointer++);
					videoFramePointer[(i*displayHSize*3)+(j*3)+2] = *(imagePointer++);
					imagePointer++;
				}
			}
		}
	}
	Xil_DCacheFlush();
	return 0;
}

/*****************************************************************************/
/**
 * This function initializes the DMA Controller and interrupts for Image Processing
 *
 * @param	imgProcess is a pointer to ImageProcess instance
 * @param   axiDmaBaseAddress base address for DMA Controller
 * @param	Intc Pointer to interrupt controller
 * 		-  0 if successfully initialized
 * 		- -1 DMA initialization failed
 * 		- -2 Interrupt setup failed
 *****************************************************************************/


int initImgProcessSystem(imgProcess *imgProcessInstance, u32 axiDmaBaseAddress,XScuGic *Intc){
	int status;
	imgProcessInstance->row = 4;
	XAxiDma_Config *myDmaConfig;
	XAxiDma myDma;
	myDmaConfig = XAxiDma_LookupConfigBaseAddr(axiDmaBaseAddress);
	status = XAxiDma_CfgInitialize(&myDma, myDmaConfig);
	if(status != XST_SUCCESS){
		xil_printf("DMA initialization failed with status %d\n",status);
		return -1;
	}
	imgProcessInstance->DmaCtrlPointer = &myDma;
	XAxiDma_IntrEnable(&myDma, XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);

	XScuGic_SetPriorityTriggerType(Intc,XPAR_FABRIC_IMAGEPROCESS_SKETCHIP_1080P_0_O_INTR_INTR,0xA0,3);
	status = XScuGic_Connect(Intc,XPAR_FABRIC_IMAGEPROCESS_SKETCHIP_1080P_0_O_INTR_INTR,(Xil_InterruptHandler)imageProcISR,(void *)imgProcessInstance);

	if(status != XST_SUCCESS){
		xil_printf("Interrupt connection failed");
		return -2;
	}
	XScuGic_Enable(Intc,XPAR_FABRIC_IMAGEPROCESS_SKETCHIP_1080P_0_O_INTR_INTR);

	XScuGic_SetPriorityTriggerType(Intc,XPAR_FABRIC_IMAGEPROCESS_AXI_DMA_0_S2MM_INTROUT_INTR,0xA1,3);
	status = XScuGic_Connect(Intc,XPAR_FABRIC_IMAGEPROCESS_AXI_DMA_0_S2MM_INTROUT_INTR,(Xil_InterruptHandler)dmaReceiveISR,(void *)imgProcessInstance);
	if(status != XST_SUCCESS){
		xil_printf("Interrupt connection failed");
		return -2;
	}
	XScuGic_Enable(Intc,XPAR_FABRIC_IMAGEPROCESS_AXI_DMA_0_S2MM_INTROUT_INTR);

	imgProcessInstance->IntrCtrlPointer=Intc;

	imgProcessInstance->done = 0;
	return 0;
}

/*****************************************************************************/
/**
 * This function initializes the DMA operation for image processing
 *
 * @param	imgProcessInstance is a pointer to the initialized imgProcess instance
 * 		-  0 DMA initiated successfully
 * 		- -1 DMA initiation failed
 *****************************************************************************/

int startImageProcessing(imgProcess *imgProcessInstance){
	int status;
	//status = XAxiDma_SimpleTransfer(imgProcessInstance->DmaCtrlPointer,(u32)imgProcessInstance->filteredImageDataPointer,(imgProcessInstance->imageHSize)*(imgProcessInstance->imageVSize),XAXIDMA_DEVICE_TO_DMA);
	status = XAxiDma_SimpleTransfer(imgProcessInstance->DmaCtrlPointer,(u32)imgProcessInstance->filteredImageDataPointer,(1920*1080*4),XAXIDMA_DEVICE_TO_DMA);
	if(status != XST_SUCCESS){
		xil_printf("DMA Receive Failed with Status %d\n",status);
		return -1;
	}
	status = XAxiDma_SimpleTransfer(imgProcessInstance->DmaCtrlPointer,(u32)imgProcessInstance->imageDataPointer1, 4*1920,XAXIDMA_DMA_TO_DEVICE);
	if(status != XST_SUCCESS){
		xil_printf("DMA Transfer failed with Status %d\n",status);
		return -1;
	}
	return 0;
}

/*****************************************************************************/
/**
 * This function is the interrupt service routine for DMA S2MM interrupt
 *
 * @param	CallBackRef is a pointer to the initialized imgProcess instance
 *
 *****************************************************************************/

static void dmaReceiveISR(void *CallBackRef){
	XAxiDma_IntrDisable((XAxiDma *)(((imgProcess*)CallBackRef)->DmaCtrlPointer), XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrAckIrq((XAxiDma *)(((imgProcess*)CallBackRef)->DmaCtrlPointer), XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);

	if(((imgProcess*)CallBackRef)->done == 0)
		((imgProcess*)CallBackRef)->done =1;
	else if (((imgProcess*)CallBackRef)->done ==1)
		((imgProcess*)CallBackRef)->done =2;
	else if (((imgProcess*)CallBackRef)->done ==2)
		((imgProcess*)CallBackRef)->done =3;

	//((imgProcess*)CallBackRef)->done++;
	xil_printf("done = %d  remain = %d \r\n",((imgProcess*)CallBackRef)->done,XAxiDma_ReadReg(XPAR_IMAGEPROCESS_AXI_DMA_0_BASEADDR,0x10));
	//resetDMA(XPAR_AXI_DMA_0_BASEADDR);
	XAxiDma_Reset((XAxiDma *)((imgProcess*)CallBackRef)->DmaCtrlPointer);
	while(!XAxiDma_ResetIsDone((XAxiDma *)((imgProcess*)CallBackRef)->DmaCtrlPointer)){

	}
	XAxiDma_IntrEnable((XAxiDma *)(((imgProcess*)CallBackRef)->DmaCtrlPointer), XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DEVICE_TO_DMA);

    //sleep(1);
	//xil_printf("done = %d \n",((imgProcess*)CallBackRef)->done);
}

/*****************************************************************************/
/**
 * This function checks whether a DMA channel is IDLE
 *
 * @param	baseAddress is the baseAddress of DMA Controller
 * @param   offset is the offset of Status register
 *
 *****************************************************************************/
u32 checkIdle(u32 baseAddress,u32 offset){
	u32 status;
	status = (XAxiDma_ReadReg(baseAddress,offset))&XAXIDMA_IDLE_MASK;
	return status;
}

/*****************************************************************************/
/**
 * This function reset  DMA engine
 *
 * @param	baseAddress is the baseAddress of DMA Controller
 * @param   offset is the offset of Status register
 *
 *****************************************************************************/
void resetDMA(u32 baseAddress){
	u32 status;
	XAxiDma_WriteReg(baseAddress,0x00,XAXIDMA_CR_RESET_MASK);
	status = (XAxiDma_ReadReg(baseAddress,0x00))&XAXIDMA_CR_RESET_MASK;
	while(status){
		status = (XAxiDma_ReadReg(baseAddress,0x00))&XAXIDMA_CR_RESET_MASK;
	}
}

/*****************************************************************************/
/**
 * This function is the interrupt service routine for the image processing IP
 *
 * @param	CallBackRef is a pointer to the initialized imgProcess instance
 *
 *****************************************************************************/

static void imageProcISR(void *CallBackRef){
	//static int row= 4;
	int status;
	//int status2;
	XScuGic_Disable(((imgProcess*)CallBackRef)->IntrCtrlPointer,XPAR_FABRIC_IMAGEPROCESS_SKETCHIP_1080P_0_O_INTR_INTR);

	status = checkIdle(XPAR_IMAGEPROCESS_AXI_DMA_0_BASEADDR,0x4);

	while(status == 0 && ((imgProcess*)CallBackRef)->row < 4328){

		if(((imgProcess*)CallBackRef)->done == 1)
			xil_printf("row1 = %d \r\n",((imgProcess*)CallBackRef)->row);

		status = checkIdle(XPAR_IMAGEPROCESS_AXI_DMA_0_BASEADDR,0x4);
	}
	if(((imgProcess*)CallBackRef)->row<4328){
		//print("ccc\n");
		if(((imgProcess*)CallBackRef)->row < 1082 && ((imgProcess*)CallBackRef)->done == 0){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer1)+(((imgProcess*)CallBackRef)->row)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status1 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}
		else if(2164 > ((imgProcess*)CallBackRef)->row &&((imgProcess*)CallBackRef)->row >= 1082 && ((imgProcess*)CallBackRef)->done == 0){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer2)+((((imgProcess*)CallBackRef)->row) - 1082)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status2 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}
		else if(3246 > ((imgProcess*)CallBackRef)->row &&((imgProcess*)CallBackRef)->row >= 2164 && ((imgProcess*)CallBackRef)->done == 0){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer3)+((((imgProcess*)CallBackRef)->row) - 2164)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status3 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}
		else if(4328 > ((imgProcess*)CallBackRef)->row &&((imgProcess*)CallBackRef)->row >= 3246 && ((imgProcess*)CallBackRef)->done == 0){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer4)+((((imgProcess*)CallBackRef)->row) - 3246)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status4 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}

//********************************************************************************************

		if(((imgProcess*)CallBackRef)->row < 1082 && ((imgProcess*)CallBackRef)->done == 1){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer5)+(((imgProcess*)CallBackRef)->row)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status5 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}
		else if(2164 > ((imgProcess*)CallBackRef)->row &&((imgProcess*)CallBackRef)->row >= 1082 && ((imgProcess*)CallBackRef)->done == 1){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer6)+((((imgProcess*)CallBackRef)->row) - 1082)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			xil_printf("row1 = %d \r\n",((imgProcess*)CallBackRef)->row);
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status6 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}
		else if(3246 > ((imgProcess*)CallBackRef)->row &&((imgProcess*)CallBackRef)->row >= 2164 && ((imgProcess*)CallBackRef)->done == 1){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer7)+((((imgProcess*)CallBackRef)->row) - 2164)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status7 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}
		else if(4328 > ((imgProcess*)CallBackRef)->row &&((imgProcess*)CallBackRef)->row >= 3246 && ((imgProcess*)CallBackRef)->done == 1){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer8)+((((imgProcess*)CallBackRef)->row) - 3246)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status8 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}
		/*if(((imgProcess*)CallBackRef)->row < 1082 && ((imgProcess*)CallBackRef)->done == 2){
			status = XAxiDma_SimpleTransfer(((imgProcess*)CallBackRef)->DmaCtrlPointer,(u32)(((imgProcess*)CallBackRef)->imageDataPointer4)+(((imgProcess*)CallBackRef)->row)*((imgProcess*)CallBackRef)->imageHSize,((imgProcess*)CallBackRef)->imageHSize,XAXIDMA_DMA_TO_DEVICE);
			((imgProcess*)CallBackRef)->row++;
			//xil_printf("row3 = %d \r\n",((imgProcess*)CallBackRef)->row);
			if(status != XST_SUCCESS){
				xil_printf("DMA Transfer Failed with Status3 %d    done = %d \r\n",status,((imgProcess*)CallBackRef)->done);
			}
		}*/
		//xil_printf("row = %d done = %d \n",((imgProcess*)CallBackRef)->row,((imgProcess*)CallBackRef)->done);
		//printf("row1 = %d \n",((imgProcess*)CallBackRef)->row);
	}
	/*if(row == 1082)
		row=4;*/
	XScuGic_Enable(((imgProcess*)CallBackRef)->IntrCtrlPointer,XPAR_FABRIC_IMAGEPROCESS_SKETCHIP_1080P_0_O_INTR_INTR);
}

