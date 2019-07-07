#define _SUPPRESS_PLIB_WARNING 1
#include "main.h"
#include "FSIO.h"

typedef struct{
	unsigned char  size;
	unsigned short address;
	unsigned char  type;
	unsigned char data[16];
} HEXLINE;

#define ERR_HEX_ERROR "Hex file error"
#define ERR_UNKNOWN "Unknown error"

// Result of reading a HEX file line
HEXLINE g_hexline;
FSFILE* g_fhandle;
char* g_fbuff;
int g_size;
int g_filepoint;
int g_srcpos;
unsigned int g_hexaddress;
// Vars used only in this file
static unsigned char g_checksum;

void hex_read_file(int blocklen){
	int i;
	if (blocklen==512) {
		// This is first read. Initialize parameter(s).
		g_srcpos=0;
		g_filepoint=0;
	} else if (g_size<512) {
		// Already reached the end of file.
		return;
	} else {
		// Shift buffer and source position 256 bytes.
		for(i=0;i<256;i++) g_fbuff[i]=g_fbuff[i+256];
		g_srcpos-=256;
		g_filepoint+=256;
	}
	// Read 512 or 256 bytes from SD card.
	g_size=512-blocklen+FSfread((void*)&g_fbuff[512-blocklen],1,blocklen,g_fhandle);
	// All lower cases
	for(i=512-blocklen;i<512;i++){
		if ('A'<=g_fbuff[i] && g_fbuff[i]<='F') g_fbuff[i]+=0x20;
	}
}

char* hex_init_file(char* buff,char* filename){
	// Open file
	g_fhandle=FSfopen(filename,"r");
	if (!g_fhandle) {
		return ERR_UNKNOWN;
	}
	// Initialize parameters
	g_fbuff=buff;
	g_srcpos=0;
	g_filepoint=0;
	// Read first 512 bytes
	hex_read_file(512);
	return 0;
}

int hex_read_byte(){
	unsigned char b1,b2;
	b1=g_fbuff[g_srcpos++];
	b2=g_fbuff[g_srcpos++];
	if ('0'<=b1 && b1<='9') {
		b1-='0';
	} else if ('a'<=b1 && b1<='f') {
		b1-='a';
		b1+=0x0a;
	} else {
		return -1;
	}
	if ('0'<=b2 && b2<='9') {
		b2-='0';
	} else if ('a'<=b2 && b2<='f') {
		b2-='a';
		b2+=0x0a;
	} else {
		return -1;
	}
	b1=(b1<<4)|b2;
	g_checksum+=b1;
	return b1;
}

char* hex_read_line(){
	int i,j;
	// Initialize checksum
	g_checksum=0;
	// Maintain at least 256 characters in cache.
	if (256<=g_srcpos) hex_read_file(256);
	// Read a hex file line
	if (g_fbuff[g_srcpos++]!=':') return ERR_HEX_ERROR;
	// Read size
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.size=(unsigned char)i;
	// Read address
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.address=(unsigned short)(i<<8);
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.address|=(unsigned short)(i);
	// Ready type
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	g_hexline.type=(unsigned char)i;
	// Read data
	for(j=0;j<g_hexline.size;j++){
		i=hex_read_byte();
		if (i<0) return ERR_HEX_ERROR;
		g_hexline.data[j]=(unsigned char)i;
	}
	// Read checksum
	i=hex_read_byte();
	if (i<0) return ERR_HEX_ERROR;
	if (g_checksum) return ERR_HEX_ERROR;
	// All done. Remove enter.
	if (g_fbuff[g_srcpos]=='\r') g_srcpos++;
	if (g_fbuff[g_srcpos]=='\n') g_srcpos++;
	return 0;
}

void write_hexline_to_flash(){
	unsigned int* hexdatawords=(unsigned int*)&g_hexline.data[0];
	unsigned int addr=g_hexaddress+g_hexline.address;
	// Change 0x9d0xxxxx -> 0xbd0xxxxx
	addr|=0x20000000;
	// Valid only in flash application address
	if (addr<FLASH_APP_ADDRESS) return;
	if (FLASH_APP_ADDRESS+1024*FLASH_APP_PAGENUM <= addr) return;
	// Write up to 4 word(s)
	NVMWriteWord ((void*)addr, hexdatawords[0]);
	if (g_hexline.size<=4) return;
	addr+=4;
	NVMWriteWord ((void*)addr, hexdatawords[1]);
	if (g_hexline.size<=8) return;
	addr+=4;
	NVMWriteWord ((void*)addr, hexdatawords[2]);
	if (g_hexline.size<=12) return;
	addr+=4;
	NVMWriteWord ((void*)addr, hexdatawords[3]);
}

void stop_with_error(char* err){
	int i;
	// CLS
	for(i=0;i<1000;i++) VRAM[i]=0;
	// Start video again
	T2CONbits.ON=1;
	// Show error
	printstr(0,err);
	printstr(80,"Reset system to continue");
	// Infinite loop
	while(1){
		asm("wait");
	}
}

void load_hexfile_and_run(char* filename){
	char* err;
	unsigned int i;
	char copiedfilename[12];
	// Stop video
	T2CONbits.ON=0;
	// Copy file name
	for(i=0;copiedfilename[i]=filename[i];i++);
	// Open file
	err=hex_init_file(&VRAM[0],filename);
	if (err) stop_with_error(err);
	// Check HEX file
	while(1){
		err=hex_read_line();
		if (err) return stop_with_error(err);
		if (g_hexline.type==1) {
			// EOF
			break;
		}
		if (g_size<=0) stop_with_error(ERR_HEX_ERROR);
	}
	FSrewind(g_fhandle);
	// Erase application first
	erase_app();
	// Read HEX and save to flash area
	while(1){
		err=hex_read_line();
		if (err) stop_with_error(err);
		if (g_hexline.type==1) {
			// EOF
			break;
		} else if (g_hexline.type==4) {
			// extended linear address
			i=g_hexline.data[0];
			i=i<<8;
			i|=g_hexline.data[0];
			i=i<<16;
			g_hexaddress=i;
		} else if (g_hexline.type==0) {
			// data
			write_hexline_to_flash();
		}
	}
	FSfclose(g_fhandle);
	// Update flash data
	erase_flashdata();
	update_apphexfile((char*)&copiedfilename[0]);
	write_flashdata();
	// All done
	SoftReset();
}
