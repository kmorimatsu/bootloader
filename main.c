#define _SUPPRESS_PLIB_WARNING 1

#include "usb.h"
#include "usb_host_msd.h"
#include "usb_host_msd_scsi.h"
#include "FSIO.h"
#include "main.h"

// Configuration Bits
// Note: 4 MHz crystal is used. CPU clock is 48 MHz.
#pragma config FUSBIDIO  = OFF          // Not using USBID (pin #14, RB5)
#pragma config FVBUSONIO = OFF          // Not using VBUSON (pin #25, RB14)
#pragma config DEBUG = OFF

#pragma config UPLLEN   = ON            // USB PLL Enabled
#pragma config FPLLMUL  = MUL_24        // PLL Multiplier
#pragma config UPLLIDIV = DIV_1         // USB PLL Input Divider
#pragma config FPLLIDIV = DIV_1         // PLL Input Divider
#pragma config FPLLODIV = DIV_2         // PLL Output Divider
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
//#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx1      // ICE/ICD Comm Channel Select
#pragma config JTAGEN   = OFF           // JTAG disabled

FSFILE * myFile;
extern volatile BOOL deviceAttached;

int main(void)
{
	SearchRec sr;
	int i,filenum,cursor;

    // Enable the cache for the best performance
    CheKseg0CacheOn();
	// Enable interrupt
    INTEnableSystemMultiVectoredInt();
	// Initilize video
	ntsc_init();
	printstr(0,"MachiKania Boot: Move \x22\x1C\x22 and Push CR");
	// Initialize USB
    init_usb();

	// Main loop
	while(1){
		// Clear screen
		for(i=40;i<1000;i++) VRAM[i]=0;
		// Wait until the USB device is attached
		printstr(40,"USB memory:");
		drawcount=0;
		while(1){
			USBTasks();
			if (USBHostMSDSCSIMediaDetect()) {
				if (FSInit()) break;
				printstr(52,"format error");
			} else if (120<drawcount) {
				drawcount=0;
				printstr(52,"not ready   ");
			}
	
		}
		printstr(52,"connected   ");
		deviceAttached = TRUE;

		// Directory listing
	   	filenum=0;
		cursor=81;
		if(FindFirst("*.hex",ATTR_MASK,&sr)){
			printstr(0,"No HEX File Found");
			while(deviceAttached) {
				USBTasks();
			}
			continue;
		} else {
			do{
				printstr(cursor,sr.filename);
				filenum++;
				cursor+=13;
				if (0==(filenum % 3)) cursor++;
			} while(!FindNext(&sr) && filenum<69);
		}

		// Select a file
		cursor=80;
		drawcount=0;
		while(deviceAttached){
			// Blink the cursor
			if (drawcount&16) VRAM[cursor]=0;
			else VRAM[cursor]=0x1c;
			// Detect CR key
			if (g_keymatrix2[8]&(1<<4)) {
				VRAM[cursor]=0x1c;
				printstr(0,"HEX file:                              ");
				printstr(10,&VRAM[cursor+1]);
				while(1);
			}
			// Detect right/left key
			if (g_keymatrix2[8]&(1<<3)) {
				// Detect shift key
				if (g_keymatrix2[8]&((1<<0)|(1<<5))) {
					i=cursor-13;
					if ((i%40)==27) i--;
				} else {
					i=cursor+13;
					if ((i%40)==39) i++;
				}
				// Check if valid movement
				if (i<80) i=cursor;
				else if (1000<i) i=cursor;
				else if (0x00==VRAM[i+1]) i=cursor;
				// Refresh view
				VRAM[cursor]=0;
				cursor=i;
				VRAM[cursor]=0x1c;
				drawcount=0;
				// Wait while key down
				while(g_keymatrix2[8]&(1<<3)){
					USBTasks();
				}
			}
			// Detect up/down key
			if (g_keymatrix2[9]&(1<<2)) {
				// Detect shift key
				if (g_keymatrix2[8]&((1<<0)|(1<<5))) {
					i=cursor-40;
				} else {
					i=cursor+40;
				}
				// Check if valid movement
				if (i<80) i=cursor;
				else if (1000<i) i=cursor;
				else if (0x00==VRAM[i+1]) i=cursor;
				// Refresh view
				VRAM[cursor]=0;
				cursor=i;
				VRAM[cursor]=0x1c;
				drawcount=0;
				// Wait while key down
				while(g_keymatrix2[9]&(1<<2)){
					USBTasks();
				}
			}
			USBTasks();
		}
		continue;
	}
    return 0;
}
