/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   https://github.com/kmorimatsu/
*/

#include <xc.h>
#include <sys/attribs.h>

extern int g_temp;

extern unsigned char VRAM[1024];
extern unsigned short drawcount;
extern unsigned short g_keybuff[32];
//extern unsigned char g_keymatrix[16];
extern unsigned char g_keymatrix2[10];
extern unsigned char g_video_disabled;

void load_hexfile_and_run(char* filename);

void init_usb(void);

unsigned int NVMUnlock (unsigned int nvmop);
unsigned int NVMWriteWord (void* address, unsigned int data);
unsigned int NVMErasePage(void* address);

void ntsc_init(void);
unsigned char ascii2char(unsigned char ascii);
void printstr(int cursor,char* str);
void printhex4(int cursor, unsigned char val);
void printhex8(int cursor, unsigned char val);
void printhex16(int cursor, unsigned short val);
void printhex32(int cursor, unsigned int val);

#define FLASH_ADDRESS 0xbd000000
#define FLASH_BOOTLOADER_LENGTH 0xE000
#define FLASH_APP_ADDRESS (FLASH_ADDRESS + FLASH_BOOTLOADER_LENGTH)
#define FLASH_APP_PAGENUM ((0x20000-FLASH_BOOTLOADER_LENGTH)>>10)
#define FLASH_DATA_ADDRESS 0xbd000c00
