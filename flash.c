#include "main.h"

const volatile int __attribute__((address(0x9d000c00))) flashdata[256]={
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

unsigned int NVMUnlock (unsigned int nvmop){
	unsigned int status;
	// Suspend or Disable all Interrupts
	asm volatile ("di %0" : "=r" (status));
	// Enable Flash Write/Erase Operations and Select
	// Flash operation to perform
	NVMCON = nvmop;
	// Write Keys
	NVMKEY = 0xAA996655;
	NVMKEY = 0x556699AA;
	// Start the operation using the Set Register
	NVMCONSET = 0x8000;
	// Wait for operation to complete
	while (NVMCON & 0x8000);
	// Restore Interrupts
	if (status & 0x00000001) asm volatile ("ei");
	else asm volatile ("di");
	// Disable NVM write enable
	NVMCONCLR = 0x0004000;
	// Return WRERR and LVDERR Error Status Bits
	return (NVMCON & 0x3000);
}

unsigned int NVMWriteWord (void* address, unsigned int data){
	unsigned int res;
	// Load data into NVMDATA register
	NVMDATA = data;
	// Load address to program into NVMADDR register
	NVMADDR = (unsigned int) address;
	// Unlock and Write Word
	res = NVMUnlock (0x4001);
	// Return Result
	return res;
}

unsigned int NVMErasePage(void* address){
	unsigned int res;
	// Set NVMADDR to the Start Address of page to erase
	NVMADDR = (unsigned int) address;
	// Unlock and Erase Page
	res = NVMUnlock(0x4004);
	// Return Result
	return res;
}

char* apphexfile(){
	if (-1==flashdata[0]) return 0;
	return (char*)&flashdata[0];
}

void erase_flashdata(){
	int i;
	int* vramwords=(int*)&VRAM[0];
	// Disable interrupt, first
	asm volatile("di");
	// Save current flash data
	for(i=0;i<256;i++) vramwords[i]=flashdata[i];
	// Erase flash
	NVMErasePage((void*)FLASH_DATA_ADDRESS);
}

void update_apphexfile(char* filename){
	int i;
	for(i=0;VRAM[i]=filename[i];i++);
}

void write_flashdata(){
	int i;
	unsigned int* vramwords=(unsigned int*)&VRAM[0];
	// Write flash words
	for(i=0;i<256;i++){
		if (vramwords[i]!=0xffffffff) {
			NVMWriteWord ((void*)(FLASH_DATA_ADDRESS+(i<<2)),vramwords[i]); 
		}
	}
	// All done. Clear video
	for(i=0;i<256;i++) vramwords[i]=0;
	// Enable interrupt
	asm volatile("ei");
}

void erase_app(){
	int i;
	asm volatile("di");
	for(i=0;i<FLASH_APP_PAGENUM;i++) {
		NVMErasePage((void*)(FLASH_APP_ADDRESS+i*1024));
	}
	asm volatile("ei");
}
