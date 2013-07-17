//
//  HelloWorldLayer.m
//  Z89Emu
//
//  Created by Les Bird on 3/5/11.
//  Copyright Les Bird 2011. All rights reserved.
//

// Import the interfaces
#import "HelloWorldScene.h"
#import "CCTouchDispatcher.h"
#import "RootViewController.h"
#import "KeyInputView.h"
#import "Z80.h"
#import "U8250.h"
#import <mach/mach_time.h>

#define DEBUG_MODE	0

#define ROM_FILE	"2716_444-62_MTR89.bin"
#define	ROM_SIZE	2048
#define ROM_H17FILE	"2716_444-19_H17.ROM"
#define ROM_H17SIZE	2048
#define FONT_FILE	"2716_444-29_H19FONT.BIN"
#define FONT_SIZE	2048
#define FONT_SCALEX	1.6f
#define FONT_SCALEY	(FONT_SCALEX * 1.3333f) // scale Y to 4:3 aspect

#define PORT174Q	0x7C
#define PORT175Q	0x7D
#define PORT176Q	0x7E
#define PORT177Q	0x7F
#define PORT320Q	0xD0
#define PORT330Q	0xD8
#define PORT340Q	0xE0
#define PORT350Q	0xE8
#define PORT360Q	0xF0
#define PORT361Q	0xF1
#define PORT362Q	0xF2

// emulator variables
byte	RAMFont[FONT_SIZE];
int		debugMode;
unsigned int timerOneMS;
BOOL	bReset;
BOOL	bSingleStep;
BOOL	bStep;
BOOL	b2MHz;
mach_timebase_info_data_t timeInfo;
uint64_t timeLastTime;

// Z80 variables
byte	RAM[65536];
byte	PortData[256];
byte	LastDataIn[256];
byte	LastDataOut[256];
Z80		Z80CPU;

// H17 emulation
// disk drive parameters
// 1. disk archives will be stored as a resource in the app bundle
// 2. once modified they are copied to working area and made r/w
// 3. disk archives are listed in a tableview (my images/distribution images)
// 4. select a disk by tapping it in the tableview
// 5. all images are stored internally as 2 sided 80 track

#define MAX_TRACKS	80	// 80 tracks
#define MAX_SIDES	2	// 2 sides
#define MAX_SECTORS	10	// 10 sectors per track
#define MAX_SECSIZE	256	// 256 bytes per sector
#define DISK_CAP	(MAX_TRACKS * MAX_SIDES * MAX_SECTORS * MAX_SECSIZE)
#define TRACK_SIZE	(MAX_SECTORS * MAX_SECSIZE)
//
// note that 1S80T and 2S40T are the same size but for simplicity we are
// only supporting 2S40T
//
#define DSIZE_1S40T	(40 * 1 * MAX_SECTORS * MAX_SECSIZE) // 1 side, 40 tracks
#define DSIZE_2S40T	(40 * 2 * MAX_SECTORS * MAX_SECSIZE) // 2 sides, 40 tracks
#define DSIZE_2S80T	(80 * 2 * MAX_SECTORS * MAX_SECSIZE) // 2 sides, 80 tracks

// port 175Q status bits
#define H17STAT_RDR	0x01 // receive data ready
#define H17STAT_ROV	0x02 // receiver overrun (ignored)
#define H17STAT_RPE	0x04 // receiver parity error (ignored)
#define H17STAT_FIL	0x40 // fill character (ignored)
#define H17STAT_TBE	0x80 // transmitter buffer empty

// port 177Q output bits
#define H17CTRL_WRG	0x01 // write gate enable
#define H17CTRL_DR0 0x02 // drive select 0
#define H17CTRL_DR1	0x04 // drive select 1
#define H17CTRL_DR2	0x08 // drive select 2
#define H17CTRL_MON	0x10 // motor on
#define H17CTRL_DIR	0x20 // head direction
#define H17CTRL_STP	0x40 // head step
#define H17CTRL_WEN	0x80 // ram write enable

// port 177Q input bits (H17ControlBits)
#define H17CTRL_HOL	0x01 // hole detect
#define H17CTRL_TR0	0x02 // track 0
#define H17CTRL_WRP	0x04 // write protect
#define H17CTRL_SYN	0x08 // sync detect

#define MAX_FILES	300
NSString *fileList[MAX_FILES];
NSString *fileDocsList[MAX_FILES];

UIView *helpView;
UIWebView *webView;

UIView *fileLibrary;
UILabel *SYLabel[3];
/*
typedef struct sSector
{
	byte data[MAX_SECSIZE];
} Sector_t;

typedef struct sTrack
{
	Sector_t sector[MAX_SECTORS];
} Track_t;

typedef struct sFloppy
{
	Track_t track[MAX_SIDES][MAX_TRACKS];
} Floppy_t;
*/
byte DiskImage[3][DISK_CAP]; // 3 disk images, 400K per image
int selectedDisk[3] = {-1, -1, -1};
int selectedSection[3] = {1, 1, 1};
int selectedDrive;
int numDiskImages;
int numDocImages;

int H17ControlBits; // control bits for port 177Q (input only)
int H17Drive; // 0 - 3 currently selected drive
int H17CharIndex; // current byte being read from a sector
int H17ReadChecksum; // computed checksum for sector
int H17Sector; // current working sector
int H17WriteSector; // current writing sector (for formatting)
int H17Side; // selected side (0 or 1)
int H17Track; // current track
int H17NullBytes; // count of null bytes (debugging)
BOOL bH17Write; // writing data to the disk image

unsigned int H17DiskTime; // incremented once every ms
int H17HoleTime = 3; // milliseconds to keep sector hole bit on
int H17SectorTime = 20; // milliseconds between track sector holes
int H17Index; // current index hole (0 - 11)

int H17DiskTracks[3] = {40, 40, 40};
int H17DiskSides[3] = {1, 1, 1};
int H17Dirty[3];
byte H17DiskVolume[3];
byte H17SectorBuffer[256];

// H19 variables
byte H19ScreenBuffer[80 * 25];

CCTexture2D *H19FontTexture;
CCSprite *H19Screen[80 * 25];
CCSpriteBatchNode *H19ScreenBatch;
CCSprite *H19Cursor;

int H19CursorBlink;
int H19CursorX, H19CursorY;
int H19CursorSaveX, H19CursorSaveY;
int H19DirectPos;
int H19ResetMode;
int H19SetMode;
int H19HoldScreenCount;
BOOL bH19EscMode;
BOOL bH19Graphics;
BOOL bH19Reverse;
BOOL bH19CursorOff;
BOOL bH19HoldScreen;

// H19 keyboard
CCSprite *keyboard;
CCSprite *keyboardIcon;

CGPoint keyboardPos;

CCSprite *keyHilite1[4];
CCSprite *keyHilite2[2];
CCSprite *keyHilite3[2];
CCSprite *keyHilite4;

CCSprite *quickKey[10];

#define KEYBUF_SIZE	64
int keyboardBeg, keyboardEnd;
byte keyboardBuf[KEYBUF_SIZE];
BOOL bKeyShift;
BOOL bOffline;
BOOL bKeyCtrl;
BOOL bCapsLock;

struct key_s
{
	byte c;
	byte shift_c;
	int x, y, w, h;
};

#define MAX_KEYS	84
#define KEY_WIDTH	50
#define KEY_WIDTH2	100
#define KEY_WIDTH3	450
#define KEY_HEIGHT	35
#define KEY_X(c)	(KEY_WIDTH * c)
#define KEY_Y(r)	(KEY_HEIGHT * r)

#define KEY_OFFLINE	0
#define KEY_F1		1
#define KEY_F2		2
#define KEY_F3		3
#define KEY_F4		4
#define KEY_F5		5
#define KEY_ERASE	6
#define KEY_BLUE	7
#define KEY_RED		8
#define KEY_GRAY	9
#define KEY_RESET	10
#define KEY_BREAK	11
#define KEY_CTRL	42
#define KEY_CAPSLOCK 43
#define KEY_SCROLL	57
#define KEY_SHIFTL	58
#define KEY_SHIFTR	69
#define KEY_REPEAT	70

struct key_s keys[] =
{
	// row 1
	{  0,  0, 80 + KEY_X( 0), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // OFF-LINE 000
	{  0,  0, 80 + KEY_X( 1), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // F1
	{  0,  0, 80 + KEY_X( 2), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // F2
	{  0,  0, 80 + KEY_X( 3), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // F3
	{  0,  0, 80 + KEY_X( 4), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // F4
	{  0,  0, 80 + KEY_X( 5), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // F5
	{  0,  0, 80 + KEY_X( 6), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // ERASE
	{  0,  0, 80 + KEY_X( 7), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // BLUE
	{  0,  0, 80 + KEY_X( 8), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // RED
	{  0,  0, 80 + KEY_X( 9), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // GREY
	{  0,  0, 80 + KEY_X(10), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // RESET
	{  0,  0, 80 + KEY_X(11), 20 + KEY_Y(0), KEY_WIDTH, KEY_HEIGHT}, // BREAK
	// row 2
	{ 27, 27, 30 + KEY_X( 0), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // ESC		012
	{'1','!', 30 + KEY_X( 1), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 1
	{'2','@', 30 + KEY_X( 2), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 2
	{'3','#', 30 + KEY_X( 3), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 3
	{'4','$', 30 + KEY_X( 4), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 4
	{'5','%', 30 + KEY_X( 5), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 5
	{'6','^', 30 + KEY_X( 6), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 6
	{'7','&', 30 + KEY_X( 7), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 7
	{'8','*', 30 + KEY_X( 8), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 8
	{'9','(', 30 + KEY_X( 9), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 9
	{'0',')', 30 + KEY_X(10), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // 0
	{'-','_', 30 + KEY_X(11), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // -
	{'=','+', 30 + KEY_X(12), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // =
	{'`','~', 30 + KEY_X(13), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // ~
	{  8,  8, 30 + KEY_X(14), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // BACKSPACE
	// row 3
	{  9,  0, 45 + KEY_X( 0), 20 + KEY_Y(2),KEY_WIDTH2, KEY_HEIGHT}, // TAB		027
	{'q','Q',105 + KEY_X( 0), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // Q
	{'w','W',105 + KEY_X( 1), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // W
	{'e','E',105 + KEY_X( 2), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // E
	{'r','R',105 + KEY_X( 3), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // R
	{'t','T',105 + KEY_X( 4), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // T
	{'y','Y',105 + KEY_X( 5), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // Y
	{'u','U',105 + KEY_X( 6), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // U
	{'i','I',105 + KEY_X( 7), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // I
	{'o','O',105 + KEY_X( 8), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // O
	{'p','P',105 + KEY_X( 9), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // P
	{'[',']',105 + KEY_X(10), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // [
	{'\\','|',105 + KEY_X(11), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // BACKSLASH
	{ 10, 10,105 + KEY_X(12), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // LINE-FEED
	{  8,  8,105 + KEY_X(13), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // DELETE
	// row 4
	{  0,  0, 30 + KEY_X( 0), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // CTRL	042
	{  0,  0, 30 + KEY_X( 1), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // CAPS LOCK
	{'a','A', 30 + KEY_X( 2), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // A
	{'s','S', 30 + KEY_X( 3), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // S
	{'d','D', 30 + KEY_X( 4), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // D
	{'f','F', 30 + KEY_X( 5), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // F
	{'g','G', 30 + KEY_X( 6), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // G
	{'h','H', 30 + KEY_X( 7), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // H
	{'j','J', 30 + KEY_X( 8), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // I
	{'k','K', 30 + KEY_X( 9), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // J
	{'l','L', 30 + KEY_X(10), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // L
	{';',':', 30 + KEY_X(11), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // SEMI-COLON
	{  0,'"', 30 + KEY_X(12), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // SINGLE QUOTE
	{'{','}', 30 + KEY_X(13), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // BRACE
	{ 13, 13,740 + KEY_X( 0), 20 + KEY_Y(3),KEY_WIDTH2, KEY_HEIGHT}, // RETURN
	// row 5
	{  0,  0, 30 + KEY_X( 0), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // SCROLL	057
	{  0,  0, 90 + KEY_X( 0), 20 + KEY_Y(4),KEY_WIDTH2, KEY_HEIGHT}, // SHIFT	058
	{'z','Z',150 + KEY_X( 0), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // Z
	{'x','X',150 + KEY_X( 1), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // X
	{'c','C',150 + KEY_X( 2), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // C
	{'v','V',150 + KEY_X( 3), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // V
	{'b','B',150 + KEY_X( 4), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // B
	{'n','N',150 + KEY_X( 5), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // N
	{'m','M',150 + KEY_X( 6), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // M
	{',','<',150 + KEY_X( 7), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // COMMA
	{'.','>',150 + KEY_X( 8), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // PERIOD
	{'/','?',150 + KEY_X( 9), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // SLASH
	{  0,  0,665 + KEY_X( 0), 20 + KEY_Y(4),KEY_WIDTH2, KEY_HEIGHT}, // SHIFT	069
	{  0,  0,730 + KEY_X( 0), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // REPEAT
	// row 6
	{' ',' ',360 + KEY_X( 0), 20 + KEY_Y(5),KEY_WIDTH3, KEY_HEIGHT}, // SPACEBAR 071
	// keypad
	{'7',  0,840 + KEY_X( 0), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 7 072
	{'8',  0,840 + KEY_X( 1), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 8
	{'9',  0,840 + KEY_X( 2), 20 + KEY_Y(1), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 9
	{'4',  0,840 + KEY_X( 0), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 4
	{'5',  0,840 + KEY_X( 1), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 5
	{'6',  0,840 + KEY_X( 2), 20 + KEY_Y(2), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 6
	{'1',  0,840 + KEY_X( 0), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 1
	{'2',  0,840 + KEY_X( 1), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 2
	{'3',  0,840 + KEY_X( 2), 20 + KEY_Y(3), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 3
	{'0',  0,840 + KEY_X( 0), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD 0
	{'.',  0,840 + KEY_X( 1), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD .
	{ 13,  0,840 + KEY_X( 2), 20 + KEY_Y(4), KEY_WIDTH, KEY_HEIGHT}, // KEYPAD ENTER 083
};

// global class pointer
HelloWorld *HelloWorldPtr;
UIView *buttonView;
extern RootViewController *ViewController;

#ifdef IE_Rx_Ready
U8250 *ConsolePort;
U8250 *Serial320Q;
U8250 *Serial330Q;
U8250 *Serial340Q;
#endif
//
//
//
extern int DAsm(byte *S,word A);

byte PortRd(int port)
{
	byte data = PortData[port];
	return data;
}

byte PortRd174Q(int port)
{
	byte data = 0;
	if (port == PORT174Q)
	{
		// reads a byte from the disk image, stuffs it into 174Q and sets data available bit
//		PortData[PORT175Q] &= ~H17STAT_RDR; // reset receive data available bit
		H17ControlBits &= ~H17CTRL_SYN;
		data = PortRd(port);

		[HelloWorldPtr H17ReadNextByte];
	}
	else if (port == PORT175Q)
	{
		data = PortRd(port) | H17STAT_TBE;
		PortData[PORT175Q] &= ~H17STAT_RDR; // reset receive data available bit
	}
	else if (port == PORT176Q)
	{
		PortData[PORT176Q] = 0xFF;
		data = PortRd(port);
	}
	else if (port == PORT177Q)
	{
		data = H17ControlBits;
		if ((H17ControlBits & H17CTRL_SYN) == 0 || H17CharIndex == 6)
		{
			if ((PortData[PORT177Q] & H17CTRL_WRG) == 0)
			{
				H17CharIndex = 0;
			}
		}
	}
	
	return data;
}

#ifdef IE_Rx_Ready
#else
// rewrite the 8250 emulation

#define UR_RBR		0 // Receiver
#define UR_THR		0 // Transmitter
#define UR_DLL		0 // Divisor latch lsb
#define UR_DLM		1 // Divisor latch msb
#define UR_IER		1 // Interrupt enable reg
#define UR_IIR		2 // Interrupt ID reg
#define UR_LCR		3 // Line control reg
#define UR_MCR		4 // Modem control reg
#define UR_LSR		5 // Line status reg
#define UR_MSR		6 // Modem status reg
#define UR_SCR		7 // Scratch reg

#define LCR_DLAB	0x80
#define IER_RDA		0x01
#define IER_THRE	0x02

#define IIR_RDA		0x04
#define IIR_THRE	0x02

int U8250PortChannel[4] = {0xE8,0xE0,0xD8,0xD0};
int U8250PortInterrupt[4] = {0x03,0x00,0x00,0x00};
int U8250PortIntAddr[8] = {INT_RST00,INT_RST08,INT_RST10,INT_RST18,INT_RST20,INT_RST28,INT_RST30,INT_RST38};
int U8250PortRegister[4][8];

void U8250Interrupt(int channel, byte iir_bit)
{
	int addr_index = U8250PortInterrupt[channel];
	if (addr_index != 0)
	{
		U8250PortRegister[channel][UR_IIR] &= 0xFE; // sets interrupt pending bit to 0
		U8250PortRegister[channel][UR_IIR] |= iir_bit; // turn on interrupt ID bit
		Z80CPU.IRequest = U8250PortIntAddr[addr_index];
	}
}

void U8250InterruptClear(int channel, byte iir_bit)
{
	int addr_index = U8250PortInterrupt[channel];
	if (addr_index != 0)
	{
		U8250PortRegister[channel][UR_IIR] |= 0x01; // disables interrupt pending bit (sets it to 1)
		U8250PortRegister[channel][UR_IIR] &= ~iir_bit;
	}
}

int U8250GetBaseChannel(int port)
{
	for (int i = 0; i < 4; i++)
	{
		if (port == U8250PortChannel[i])
		{
			return i;
		}
	}
	return -1;
}

// output to the 8250 UART
void U8250Out(int port, byte data)
{
	int reg = port & 0x07;
	int base = port & 0xF8;
	int channel = U8250GetBaseChannel(base);
	if (channel != -1)
	{
		U8250PortRegister[channel][reg] = data;
		if ((U8250PortRegister[channel][UR_LCR] & LCR_DLAB) == 0)
		{
			if (reg == 0)
			{
				if ((U8250PortRegister[channel][UR_IER] & IER_THRE) != 0)
				{
					U8250Interrupt(channel, IIR_THRE);
				}
			}
		}
	}
}

// input from the 8250 UART
byte U8250In(int port)
{
	int reg = port & 0x07;
	int base = port & 0xF8;
	int channel = U8250GetBaseChannel(base);
	byte data = 0;
	if (channel != -1)
	{
		data = U8250PortRegister[channel][reg];
		if (reg == UR_RBR)
		{
			if ((U8250PortRegister[channel][UR_IER] & IER_RDA) != 0)
			{
				U8250InterruptClear(channel, IIR_RDA);
			}
			U8250PortRegister[channel][UR_LSR] &= 0xFE;
		}
		else if (reg == UR_IIR)
		{
			U8250InterruptClear(channel, IIR_THRE);
		}
	}
	return data;
}

void U8250Reset(int channel)
{
	U8250PortRegister[channel][0] = 0;
	U8250PortRegister[channel][1] = 0;
	U8250PortRegister[channel][2] = 0x01; // no interrupts pending
	U8250PortRegister[channel][3] = 0x07; // 8 data bits, 2 stop bits
	U8250PortRegister[channel][4] = 0;
	U8250PortRegister[channel][5] = 0x60; // THRE & TSRE
	U8250PortRegister[channel][6] = 0;
	U8250PortRegister[channel][7] = 0;
}
#endif

//

byte PortRd320Q(int port)
{
#ifdef IE_Rx_Ready
	byte d = [Serial320Q Read:port];
#else
	byte d = U8250In(port);
#endif
	return d;
}

byte PortRd330Q(int port)
{
#ifdef IE_Rx_Ready
	byte d = [Serial330Q Read:port];
#else
	byte d = U8250In(port);
#endif
	return d;
}

byte PortRd340Q(int port)
{
#ifdef IE_Rx_Ready
	byte d = [Serial340Q Read:port];
#else
	byte d = U8250In(port);
#endif
	return d;
}

byte PortRd350Q(int port)
{
#ifdef IE_Rx_Ready
	byte d = [ConsolePort Read:port];
#else
	byte d = U8250In(port);
#endif
	return d;
}

void PortWr(int port, byte data)
{
	PortData[port] = data;
}

// H17 control/data ports
void PortWr174Q(int port, byte data)
{
	if (port == PORT177Q) // port 177Q H17 control
	{
		// 0000.0001 = write gate enable
		// 0001.0000 = motor on
		// 0000.0010 = drive select 0
		// 0000.0100 = drive select 1
		// 0000.1000 = drive select 2
		// 0010.0000 = head step direction (0 = out)
		// 0100.0000 = head step
		// 1000.0000 = RAM write enable
		if ((PortData[PORT177Q] & H17CTRL_STP) == 0 && (data & H17CTRL_STP) != 0) // step the head
		{
			if ((PortData[PORT177Q] & H17CTRL_DIR) != 0)
			{
				if (H17Track < H17DiskTracks[H17Drive])
				{
					H17Track++;
				}
				H17ControlBits &= ~H17CTRL_TR0;
			}
			else
			{
				if (H17Track > 0)
				{
					H17Track--;
				}
				if (H17Track == 0)
				{
					H17ControlBits |= H17CTRL_TR0;
				}
			}
		}
		if ((data & H17CTRL_DR0) != 0)
		{
			H17Drive = 0;
		}
		else if ((data & H17CTRL_DR1) != 0)
		{
			H17Drive = 1;
		}
		else if ((data & H17CTRL_DR2) != 0)
		{
			H17Drive = 2;
		}
		if ((data & H17CTRL_WRG) == 0)
		{
			bH17Write = NO;
		}
	}
	else if (port == PORT175Q) // formatting drive
	{
		return;
	}
	else if (port == PORT176Q)
	{
		// puts 0xFD (sync char) in port buffer and turns on read mode
		H17ControlBits |= H17CTRL_SYN;
		H17ReadChecksum = 0xFD;
		[HelloWorldPtr H17ReadByte:0xFD];
	}
	else if (port == PORT174Q) // writing to a disk image
	{
		if (bH17Write)
		{
			[HelloWorldPtr H17WriteNextByte:data];
		}
		else if (data == 0xFD) // sync char turns on writing
		{
//			NSLog(@"PORT: OUT 174,0 %d NULL BYTES", H17NullBytes);
			H17NullBytes = 0;
//			NSLog(@"PORT: OUT 174,0xFD SYNC");
			bH17Write = YES;
			if (H17CharIndex != 6)
			{
				H17CharIndex++;
			}
		}
		else
		{
			if (data == 0)
			{
				H17NullBytes++;
			}
		}
	}
	
	PortWr(port, data);
}

void PortWr320Q(int port, byte data)
{
#ifdef IE_Rx_Ready
	[Serial320Q Write:port :data];
#else
	U8250Out(port, data);
#endif
}

void PortWr330Q(int port, byte data)
{
#ifdef IE_Rx_Ready
	[Serial330Q Write:port :data];
#else
	U8250Out(port, data);
#endif
}

void PortWr340Q(int port, byte data)
{
#ifdef IE_Rx_Ready
	[Serial340Q Write:port :data];
#else
	U8250Out(port, data);
#endif
}

void PortWr350Q(int port, byte data)
{
#ifdef IE_Rx_Ready
	[ConsolePort Write:port :data];
	if (port == PORT350Q)
	{
		data = [ConsolePort GetData];
		if (data != 0)
		{
			[HelloWorldPtr H19SetChar:data];
		}
	}
	if (ConsolePort.m_bINT)
	{
		Z80CPU.IRequest = INT_RST18;
	}
#else
	U8250Out(port, data);

	int base = port & 0xF8;
	int channel = U8250GetBaseChannel(base);
	if (U8250PortRegister[channel][UR_THR] == data)
	{
		[HelloWorldPtr H19SetChar:data];
	}
#endif
}

/** RdZ80()/WrZ80() ******************************************/
/** These functions are called when access to RAM occurs.   **/
/** They allow to control memory access.                    **/
/************************************ TO BE WRITTEN BY USER **/
void WrZ80(register word Addr,register byte Value)
{
	if (Addr < 0x2000)
	{
		if ((PortData[PORT362Q] & 0x20) == 0 && (PortData[PORT177Q] & H17CTRL_WEN) == 0)
		{
			return;
		}
	}
	RAM[Addr] = Value;
}

byte RdZ80(register word Addr)
{
	byte d = RAM[Addr];
//	NSLog(@"%04X: %03X", Addr, d);
	return d;
}

/** InZ80()/OutZ80() *****************************************/
/** Z80 emulation calls these functions to read/write from  **/
/** I/O ports. There can be 65536 I/O ports, but only first **/
/** 256 are usually used.                                   **/
/************************************ TO BE WRITTEN BY USER **/
void OutZ80(register word Port,register byte Value)
{
	Port &= 0xFF;
	if (Port >= PORT174Q && Port <= PORT177Q)
	{
		PortWr174Q(Port, Value);
	}
	else if (Port >= PORT320Q && Port < PORT320Q + 8)
	{
		PortWr320Q(Port, Value);
	}
	else if (Port >= PORT330Q && Port < PORT330Q + 8)
	{
		PortWr330Q(Port, Value);
	}
	else if (Port >= PORT340Q && Port < PORT340Q + 8)
	{
		PortWr340Q(Port, Value);
	}
	else if (Port >= PORT350Q && Port < PORT350Q + 8)
	{
		PortWr350Q(Port, Value);
	}
	else if (Port == PORT360Q) // H8 PAD input port
	{
		PortWr(Port, 0);
	}
	else if (Port == PORT361Q) // digit select output port
	{
		PortWr(Port, 0);
	}
	else if (Port == PORT362Q) // H8/H88/H89 control port
	{
		H17Side = ((Value & 0x40) == 0) ? 0 : 1;
//		if (H17Side != 0)
//		{
//			H17DiskSides[H17Drive] = 2;
//		}
		PortWr(Port, Value);

//		NSLog(@"[%09d] OUT %02X,%02X", timerOneMS, Port, Value);
	}
	else
	{
		PortWr(Port, Value);
	}

	if (Value != LastDataOut[Port] || Port == PORT174Q || Port == PORT350Q)
	{
		LastDataOut[Port] = Value;
		if (Port != PORT360Q)
		{
			if (debugMode == 2)
			{
				if (Port < PORT174Q || Port > PORT177Q)
				{
					NSLog(@"[%09d] OUT %02X,%02X", timerOneMS, Port, Value);
				}
			}
		}
	}
}

byte InZ80(register word Port)
{
	byte d = 0;
	Port &= 0xFF;
	if (Port >= PORT174Q && Port <= PORT177Q)
	{
		d = PortRd174Q(Port);
	}
	else if (Port >= PORT320Q && Port < PORT320Q + 8)
	{
		d = PortRd320Q(Port);
	}
	else if (Port >= PORT330Q && Port < PORT330Q + 8)
	{
		d = PortRd330Q(Port);
	}
	else if (Port >= PORT340Q && Port < PORT340Q + 8)
	{
		d = PortRd340Q(Port);
	}
	else if (Port >= PORT350Q && Port < PORT350Q + 8)
	{
		d = PortRd350Q(Port);
	}
	else if (Port == PORT360Q) // H8 PAD input port
	{
		d = 0xFF;
	}
	else if (Port == PORT361Q) // digit select output port
	{
		d = 0xFF;
	}
	else if (Port == PORT362Q) // H8/H88/H89 control port
	{
		d = 0x20; // switch settings 0010.0000
	}
	else
	{
		d = PortRd(Port);
	}

//	if (d != LastDataIn[Port])
//	{
//		LastDataIn[Port] = d;
//		if (debugMode != 0)
//		{
//			NSLog(@"[%09d] IN %02X,%02X", timerOneMS, Port, d);
//		}
//	}
	return d;
}

/** PatchZ80() ***********************************************/
/** Z80 emulation calls this function when it encounters a  **/
/** special patch command (ED FE) provided for user needs.  **/
/** For example, it can be called to emulate BIOS calls,    **/
/** such as disk and tape access. Replace it with an empty  **/
/** macro for no patching.                                  **/
/************************************ TO BE WRITTEN BY USER **/
void PatchZ80(register Z80 *R)
{
}

/** DebugZ80() ***********************************************/
/** This function should exist if DEBUG is #defined. When   **/
/** Trace!=0, it is called after each command executed by   **/
/** the CPU, and given the Z80 registers. Emulation exits   **/
/** if DebugZ80() returns 0.                                **/
/*************************************************************/
#ifdef DEBUG
byte DebugZ80(register Z80 *R)
{
	if (debugMode != 0)
	{
		while (bSingleStep && !bStep)
		{
		}
		bStep = NO;
	}
	
	return 1;
}
#endif

/** LoopZ80() ************************************************/
/** Z80 emulation calls this function periodically to check **/
/** if the system hardware requires any interrupts. This    **/
/** function must return an address of the interrupt vector **/
/** (0x0038, 0x0066, etc.) or INT_NONE for no interrupt.    **/
/** Return INT_QUIT to exit the emulation loop.             **/
/************************************ TO BE WRITTEN BY USER **/
word LoopZ80(register Z80 *R)
{
	if (b2MHz)
	{
		do
		{
			uint64_t timeCurrentTime = mach_absolute_time();
			uint64_t timeElapsed = timeCurrentTime - timeLastTime;
			timeElapsed *= timeInfo.numer;
			timeElapsed /= timeInfo.denom;
			if (timeElapsed >= 1000000)
			{
				break;
			}
		} while (1);
	}
	timeLastTime = mach_absolute_time();
	
	if (bReset)
	{
		timerOneMS = 0;
		H17DiskTime = 0;
		PortData[PORT362Q] = 0x20;
		[HelloWorldPtr H17Reset];
		[HelloWorldPtr H19Cls];
		[HelloWorldPtr loadROM];
		ResetZ80(&Z80CPU);
		bReset = NO;
	}
	else
	{
		timerOneMS++;
		H17DiskTime++;
		[HelloWorldPtr H17Service];
		if ((timerOneMS % 2) == 0)
		{
			if ((PortData[PORT362Q] & 0x02) == 0x02)
			{
				// clock interrupt
				R->IFF |= IFF_1;
				return INT_RST08;
			}
		}
	}
	
	return INT_NONE;
}

/** JumpZ80() ************************************************/
/** Z80 emulation calls this function when it executes a    **/
/** JP, JR, CALL, RST, or RET. You can use JumpZ80() to     **/
/** trap these opcodes and switch memory layout.            **/
/************************************ TO BE WRITTEN BY USER **/
#ifndef JUMPZ80
#define JumpZ80(PC)
#else
void JumpZ80(word PC)
{
}
#endif


// HelloWorld implementation
@implementation HelloWorld

@synthesize emuThread;
//@synthesize H19Thread;

+(id) scene
{
	// 'scene' is an autorelease object.
	CCScene *scene = [CCScene node];
	
	// 'layer' is an autorelease object.
	HelloWorld *layer = [HelloWorld node];
	
	// add layer as a child to scene
	[scene addChild: layer];
	
	// return the scene
	return scene;
}

-(void)addSwitch:(int)x :(int)y :(int)t :(NSString *)label
{
	CGRect r = CGRectMake(x, y, 60, 15);
	UISwitch *s = [[UISwitch alloc] initWithFrame:r];
	s.tag = t;
	[s addTarget:self action:@selector(speedToggle:) forControlEvents:UIControlEventValueChanged];
	[[ViewController view] addSubview:s];

	r = CGRectMake(x - 60, y + 7, 60, 15);
	UILabel *l = [[UILabel alloc] initWithFrame:r];
	[l setText:label];
	l.backgroundColor = [UIColor clearColor];
	l.textColor = [UIColor greenColor];
	[[ViewController view] addSubview:l];
}

// on "init" you need to initialize your instance
-(id) init
{
	// always call "super" init
	// Apple recommends to re-assign "self" with the "super" return value
	if( (self=[super init] ))
	{
		// set up the 2MHz switch
		int width = [[CCDirector sharedDirector] winSize].width;
		int height = [[CCDirector sharedDirector] winSize].height;
		int x = width - 100;
		int y = height - 25;
		[self addSwitch:x :y :0 :@"2MHz"];
		
		// init the high resolution timer
		mach_timebase_info(&timeInfo);
		timeLastTime = mach_absolute_time();
#ifdef IE_Rx_Ready
		ConsolePort = [[U8250 alloc] init];
		[ConsolePort InitUART];
		Serial320Q = [[U8250 alloc] init];
		[Serial320Q InitUART];
		Serial330Q = [[U8250 alloc] init];
		[Serial330Q InitUART];
		Serial340Q = [[U8250 alloc] init];
		[Serial340Q InitUART];
#else
		U8250Reset(0);
		U8250Reset(1);
		U8250Reset(2);
		U8250Reset(3);
#endif		
		[self loadROM];
		[self H19Init];
		
		// create a tableview and fill it with the disk library
		[self loadLibrary];
		
		[self loadQuickKeys];
		
		CGRect r = CGRectMake(width - 64, 0, 64, 64);
		UIButton *button = [UIButton buttonWithType:UIButtonTypeInfoLight];
		button.frame = r;
		button.tag = 10;
		[button addTarget:self action:@selector(buttonClicked:) forControlEvents:UIControlEventTouchUpInside];
		[[ViewController view] addSubview:button];
		
		HelloWorldPtr = self;
		[[CCTouchDispatcher sharedDispatcher] addTargetedDelegate:self priority:0 swallowsTouches:YES];
		
		KeyInputView *inputView = [[KeyInputView alloc] initWithFrame:CGRectZero];
		[[ViewController view] addSubview:inputView];
		[inputView becomeFirstResponder];
		[inputView release];
		
		[self scheduleUpdate];
		
		[self startCPU];
	}
	return self;
}

// on "dealloc" you need to release all your retained objects
- (void) dealloc
{
	[super dealloc];
}

- (void) loadROM
{
	char romFile[512];
	
//	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
//	const char *userPath = [[paths objectAtIndex:0] cStringUsingEncoding:NSASCIIStringEncoding];
	const char *path = [[[NSBundle mainBundle] bundlePath] cStringUsingEncoding:NSASCIIStringEncoding];
	sprintf(romFile, "%s/%s", path, ROM_FILE);
	FILE *fp = fopen(romFile, "rb");
	if (fp != nil)
	{
		fread(&RAM[0], sizeof(byte), ROM_SIZE, fp);
		fclose(fp);
	}
	sprintf(romFile, "%s/%s", path, ROM_H17FILE);
	fp = fopen(romFile, "rb");
	if (fp != nil)
	{
		fread(&RAM[0x1800], sizeof(byte), ROM_H17SIZE, fp);
		fclose(fp);
	}
}

- (void) loadQuickKeys
{
	int center_x = 100;
	int center_y = 280;
	for (int i = 0; i < 10; i++)
	{
		switch (i)
		{
			case 0:
				quickKey[0] = [CCSprite spriteWithFile:@"orange_up.png"];
				quickKey[0].position = ccp(center_x, center_y + 50);
				break;
			case 1:
				quickKey[1] = [CCSprite spriteWithFile:@"orange_right.png"];
				quickKey[1].position = ccp(center_x + 50, center_y);
				break;
			case 2:
				quickKey[2] = [CCSprite spriteWithFile:@"orange_down.png"];
				quickKey[2].position = ccp(center_x, center_y - 50);
				break;
			case 3:
				quickKey[3] = [CCSprite spriteWithFile:@"orange_left.png"];
				quickKey[3].position = ccp(center_x	- 50, center_y);
				break;
			case 4:
				quickKey[4] = [CCSprite spriteWithFile:@"orange_center.png"];
				quickKey[4].position = ccp(center_x, center_y);
				break;
			default:
				quickKey[i] = [CCSprite spriteWithFile:@"orange_action.png"];
				quickKey[i].position = ccp((1024 - (5 * 50)) + ((i - 5) * 50), center_y);
				break;
		}
		quickKey[i].opacity = 64;
		quickKey[i].visible = NO;
		
		[self addChild:quickKey[i]];
	}
}

- (void) showHelpScreen
{
	int width = [[CCDirector sharedDirector] winSize].width - 64;
	int height = [[CCDirector sharedDirector] winSize].height - 64;
	CGRect r = CGRectMake(32, 32, width, height);
	helpView = [[UIView alloc] initWithFrame:r];
	helpView.backgroundColor = [UIColor lightGrayColor];
	[[helpView layer] setCornerRadius:10];
	[helpView setClipsToBounds:YES];

	UIButton *doneButton = [self initButton:8 :8 :80 :30 :98 :@"CLOSE"];
	[helpView addSubview:doneButton];
	
	UIButton *helpButton = [self initButton:width - 88 :8 :80 :30 :97 :@"HELP"];
	[helpView addSubview:helpButton];
	
	UIButton *refButton = [self initButton:width - 88 - 150 :8 :120 :30 :96 :@"HDOS REF"];
	[helpView addSubview:refButton];
	
	if (webView == nil)
	{
		CGRect r2 = CGRectMake(16, 48, width - 32, height - (48 + 16));
		webView = [[UIWebView alloc] initWithFrame:r2];
		[[webView layer] setCornerRadius:10];
		[webView setClipsToBounds:YES];
		webView.scalesPageToFit = YES;
		NSString *path = [[NSBundle mainBundle] pathForResource:@"HDOS_3_02_Reference_Manual" ofType:@"pdf"];
		NSURL *url = [NSURL fileURLWithPath:path];
		NSURLRequest *request = [NSURLRequest requestWithURL:url];
		[webView loadRequest:request];
	}
	
	[helpView addSubview:webView];

	[[ViewController view] addSubview:helpView];
	[helpView release];
}

- (void) showHelpWebpage
{
	if (webView != nil)
	{
		NSURL *url = [NSURL URLWithString:@"http://www.lesbird.com/iPadZ89/default.html"];
		NSURLRequest *request = [NSURLRequest requestWithURL:url];
		[webView loadRequest:request];
	}
}

- (void) showRefWebpage
{
	if (webView != nil)
	{
		NSString *path = [[NSBundle mainBundle] pathForResource:@"HDOS_3_02_Reference_Manual" ofType:@"pdf"];
		NSURL *url = [NSURL fileURLWithPath:path];
		NSURLRequest *request = [NSURLRequest requestWithURL:url];
		[webView loadRequest:request];
	}
}

// load a disk image
- (void) loadDisk:(int)drive :(NSString *)disk_file
{
	NSString *path = nil;
	if (selectedSection[drive] == 0)
	{
		// documents directory (My Images)
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES); 
		path = [paths objectAtIndex:0];
	}
	else
	{
		// bundle directory (Distribution)
		path = [[NSBundle mainBundle] bundlePath];
	}
	NSString *diskFile = [NSString stringWithFormat:@"%@/%@", path, disk_file];
	NSData *data = [NSData dataWithContentsOfFile:diskFile];
	if (data != nil)
	{
		NSUInteger disk_size = [data length];
		[data getBytes:&DiskImage[drive][0]];
		if (disk_size == DSIZE_1S40T)
		{
			H17DiskTracks[drive] = 40;
			H17DiskSides[drive] = 1;
		}
		else if (disk_size == DSIZE_2S40T)
		{
			H17DiskTracks[drive] = 40;
			H17DiskSides[drive] = 2;
		}
		else if (disk_size == DSIZE_2S80T)
		{
			H17DiskTracks[drive] = 80;
			H17DiskSides[drive] = 2;
		}
		[self H17SetDiskParameters:drive];
	}
}

// save disk images to the apps documents directory
- (void) saveDisk:(int)drive :(NSString *)disk_file
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES); 
	NSString *documentsDirectoryPath = [paths objectAtIndex:0];
	NSString *diskFile = [NSString stringWithFormat:@"%@/%@", documentsDirectoryPath,disk_file];
	NSUInteger diskSize = H17DiskTracks[drive] * H17DiskSides[drive] * TRACK_SIZE;
	if (diskSize > 0)
	{
		NSData *data = [NSData dataWithBytes:&DiskImage[drive][0] length:diskSize];
		if ([[NSFileManager defaultManager] createFileAtPath:diskFile contents:data attributes:nil])
		{
			NSLog(@"File %@ saved", diskFile);
		}
		else
		{
			NSLog(@"Error saving %@", diskFile);
		}
	}
	for (int i = 0; i < numDocImages; i++)
	{
		if ([fileDocsList[i] isEqualToString:disk_file])
		{
			// don't increase docs count
			return;
		}
	}
	// add the new image to the list
	fileDocsList[numDocImages] = [[NSString alloc] initWithString:disk_file];
	numDocImages++;
}

- (void) deleteDisk:(NSString *)disk_file
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES); 
	NSString *documentsDirectoryPath = [paths objectAtIndex:0];
	NSString *diskFile = [NSString stringWithFormat:@"%@/%@", documentsDirectoryPath,disk_file];
	if ([[NSFileManager defaultManager] removeItemAtPath:diskFile error:nil])
	{
		NSLog(@"File %@ removed successfully", diskFile);
	}
	else
	{
		NSLog(@"Error %@ not removed", diskFile);
	}
}

// create a tableview and fill it with the disk library
// allow tap to select a disk image to insert
// drives supported: SY0:,SY1:,SY2:
// organize library by CP/M and HDOS (2 groups)
- (void)loadLibrary
{
	NSFileManager *localFileManager=[[NSFileManager alloc] init];

	NSString *path = [[NSBundle mainBundle] bundlePath];
	NSDirectoryEnumerator *dirEnum = [localFileManager enumeratorAtPath:path];
	
	int n = 0;
	NSString *file;
	while ((file = [dirEnum nextObject]))
	{
		if ([[file pathExtension] isEqualToString: @"h8d"])
		{
			fileList[n] = [[NSString alloc] initWithString:file];
			n++;
		}
	}
	numDiskImages = n;

	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES); 
	NSString *documentsDirectoryPath = [paths objectAtIndex:0];
	NSDirectoryEnumerator *dirEnum2 = [localFileManager enumeratorAtPath:documentsDirectoryPath];
	
	n = 0;
	while ((file = [dirEnum2 nextObject]))
	{
		if ([[file pathExtension] isEqualToString: @"h8d"])
		{
			fileDocsList[n] = [[NSString alloc] initWithString:file];
			n++;
		}
	}
	numDocImages = n;
	
	[localFileManager release];
}

- (NSString *)getDriveFile:(int)drive
{
	if (selectedDisk[drive] == -1)
	{
		return @"-EMPTY-";
	}
	if (selectedSection[drive] == 0)
	{
		return fileDocsList[selectedDisk[drive]];
	}
	return fileList[selectedDisk[drive]];
}

- (NSString *)getDiskName:(int)drive
{
	NSString *s = nil;
	
	if ([self H17IsHDOSDisk:drive])
	{
		NSString *label = [self H17GetHDOSLabel:drive];
		if (label.length > 0)
		{
			s = [NSString stringWithFormat:@"%@.h8d", label];
		}
	}
//	else
//	{
//		s = [NSString stringWithFormat:@"%dS%dT_%03d.h8d", H17DiskSides[drive], H17DiskTracks[drive], numDocImages];
//	}
	return s;
}

- (void)setSelectedDrive:(id)sender
{
	UIButton *button = (UIButton *)sender;
	selectedDrive = button.tag;
}

- (void)buttonClicked:(id)sender
{
	UIButton *button = (UIButton *)sender;
	switch (button.tag)
	{
		case 0: // select drive 0
		case 1: // select drive 1
		case 2: // select drive 2
			selectedDrive = button.tag;
			if (selectedDisk[selectedDrive] != -1)
			{
				selectedDisk[selectedDrive] = -1; // eject the disk
				SYLabel[selectedDrive].text = [self getDriveFile:selectedDrive];
			}
			break;
		case 10: // help button
			[self showHelpScreen];
			break;
		case 20: // mount button
			[HelloWorld keyboardPut:'m'];
			[HelloWorld keyboardPut:'o'];
			[HelloWorld keyboardPut:'u'];
			[HelloWorld keyboardPut:'n'];
			[HelloWorld keyboardPut:'t'];
			[HelloWorld keyboardPut:' '];
			break;
		case 21: // dir button
			[HelloWorld keyboardPut:'d'];
			[HelloWorld keyboardPut:'i'];
			[HelloWorld keyboardPut:'r'];
			[HelloWorld keyboardPut:' '];
			break;
		case 22: // sy0: button
			[HelloWorld keyboardPut:'s'];
			[HelloWorld keyboardPut:'y'];
			[HelloWorld keyboardPut:'0'];
			[HelloWorld keyboardPut:':'];
			break;
		case 23: // sy1: button
			[HelloWorld keyboardPut:'s'];
			[HelloWorld keyboardPut:'y'];
			[HelloWorld keyboardPut:'1'];
			[HelloWorld keyboardPut:':'];
			break;
		case 24: // sy2: button
			[HelloWorld keyboardPut:'s'];
			[HelloWorld keyboardPut:'y'];
			[HelloWorld keyboardPut:'2'];
			[HelloWorld keyboardPut:':'];
			break;
		case 25: // set button
			[HelloWorld keyboardPut:'s'];
			[HelloWorld keyboardPut:'e'];
			[HelloWorld keyboardPut:'t'];
			[HelloWorld keyboardPut:' '];
			break;
		case 26: // ctl-d button
			[HelloWorld keyboardPut:0x04];
			break;
		case 27: // ctl-c button
			[HelloWorld keyboardPut:0x03];
			break;
		case 28: // boot button
			[HelloWorld keyboardPut:'b'];
			[HelloWorld keyboardPut:0x0D];
			break;
		case 29: // reset button
			bReset = YES;
			break;
		case 96: // reference manual page request
			[self showRefWebpage];
			break;
		case 97: // web page help (lesbird.com)
			[self showHelpWebpage];
			break;
		case 98: // close
			[helpView removeFromSuperview];
			helpView = nil;
			break;
		case 99: // done
			[fileLibrary removeFromSuperview];
			SYLabel[0] = nil;
			SYLabel[1] = nil;
			SYLabel[2] = nil;
			fileLibrary = nil;
			break;
	}
}

- (void)insertDisk:(int)drive :(NSString *)fileName
{
	[self loadDisk:selectedDrive :fileName];
	SYLabel[drive].text = fileName;
}

- (UIButton *)initButton:(int)x :(int)y :(int)width :(int)height :(int)tag :(NSString *)label
{
	CGRect r = CGRectMake(x, y, width, height);
	UIButton *button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	button.frame = r;
	button.tag = tag;
    [button addTarget:self action:@selector(buttonClicked:) forControlEvents:UIControlEventTouchUpInside];	
	[button setTitle:label forState:UIControlStateNormal];
	[button setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
	
	return button;
}

- (UILabel *)initLabel:(int)x :(int)y :(int)width :(int)height :(NSString *)s
{
	CGRect r = CGRectMake(x, y, width, height);
	UILabel *label = [[[UILabel alloc] initWithFrame:r] autorelease];
	label.backgroundColor = [UIColor clearColor];
	label.text = s;
	
	return label;
}

- (void)initTableView
{
	int x = 150;
	int y = 25;
	int w = [[CCDirector sharedDirector] winSize].width - (x * 2);
	int h = [[CCDirector sharedDirector] winSize].height - (y * 2);
	CGRect r = CGRectMake(x, y, w, h);
	UIView *view = [[[UIView alloc] initWithFrame:r] autorelease];
	view.backgroundColor = [UIColor lightGrayColor];
	[[view layer] setCornerRadius:10];
	[view setClipsToBounds:YES];
	
	CGRect labelRect = CGRectMake(0, 0, w, 32);
	UILabel *tableHeader = [[[UILabel alloc] initWithFrame:labelRect] autorelease];
	tableHeader.text = [NSString stringWithFormat:@"FLOPPY DISK IMAGES"];
	tableHeader.textAlignment = UITextAlignmentCenter;
	[[tableHeader layer] setCornerRadius:10];
	[tableHeader setClipsToBounds:YES];
	[view addSubview:tableHeader];
	
	w = r.size.width;
	h = r.size.height;
	x = w - 100;
	y = h - 40;
	UIButton *doneButton = [self initButton:x :y :80 :30 :99 :@"DONE"];
	[view addSubview:doneButton];
	
	x = 10;
	y = 40;
	UIButton *sy0Button = [self initButton:x :y :80 :30 :0 :@"SY0:"];
	[view addSubview:sy0Button];
	
	SYLabel[0] = [self initLabel:x+100 :y :w-150 :30 :[self getDriveFile:0]];
	SYLabel[0].textColor = [UIColor whiteColor];
	[view addSubview:SYLabel[0]];
	y += 40;

	UIButton *sy1Button = [self initButton:x :y :80 :30 :1 :@"SY1:"];
	[view addSubview:sy1Button];
	
	SYLabel[1] = [self initLabel:x+100 :y :w-150 :30 :[self getDriveFile:1]];
	SYLabel[1].textColor = [UIColor whiteColor];
	[view addSubview:SYLabel[1]];
	y += 40;
	
	UIButton *sy2Button = [self initButton:x :y :80 :30 :2 :@"SY2:"];
	[view addSubview:sy2Button];
	
	SYLabel[2] = [self initLabel:x+100 :y :w-150 :30 :[self getDriveFile:2]];
	SYLabel[2].textColor = [UIColor whiteColor];
	[view addSubview:SYLabel[2]];
	y += 40;
	
	CGRect tableViewFrame = CGRectMake(10, y, w-20, h-210);
	UITableView *tableView = [[[UITableView alloc] initWithFrame:tableViewFrame style:UITableViewStyleGrouped] autorelease];
	tableView.delegate = self;
	tableView.dataSource = self;
	tableView.separatorStyle = UITableViewCellSeparatorStyleSingleLineEtched;
	tableView.allowsSelection = YES;
	[[tableView layer] setCornerRadius:10];
	[tableView setClipsToBounds:YES];
	[view addSubview:tableView];
	
	UILabel *credit = [self initLabel:0 :(h - 30) :w :20 :@"Heathkit H89 Emulator V1.0 (C)2011 by Les Bird"];
	credit.textAlignment = UITextAlignmentCenter;
	credit.textColor = [UIColor whiteColor];
	[view addSubview:credit];
	
	[[ViewController view] addSubview:view];
	
	fileLibrary = view;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
	return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
	// count of total number of disk images (section 0 = modified, section 1 = distribution)
	if (section == 0)
	{
		return numDocImages;
	}
	return numDiskImages;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	static NSString *MyIdentifier = @"Library";
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:MyIdentifier];
	if (cell == nil)
	{
		cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:MyIdentifier] autorelease];
		cell.textLabel.font = [UIFont fontWithName:@"Courier-Bold" size:16];
//		cell.editingAccessoryType = UITableViewCellAccessoryDisclosureIndicator;
		NSString *path = [[NSBundle mainBundle] pathForResource:@"Floppy" ofType:@"png"];
		UIImage *floppy = [UIImage imageWithContentsOfFile:path];
		cell.imageView.image = floppy;
	}
	
	if (indexPath.section == 0 && indexPath.row < numDocImages)
	{
		NSString *fileName = fileDocsList[indexPath.row];
		if (fileName != nil)
		{
			cell.textLabel.text = fileName;
			if (tableView.allowsSelection)
			{
				cell.detailTextLabel.textColor = [UIColor blackColor];
			}
			else
			{
				cell.detailTextLabel.textColor = [UIColor grayColor];
			}
		}
	}
	else if (indexPath.section == 1 && indexPath.row < numDiskImages)
	{
		NSString *fileName = fileList[indexPath.row];
		if (fileName != nil)
		{
			cell.textLabel.text = fileName;
			if (tableView.allowsSelection)
			{
				cell.detailTextLabel.textColor = [UIColor blackColor];
			}
			else
			{
				cell.detailTextLabel.textColor = [UIColor grayColor];
			}
		}
	}
	
	return cell;
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (indexPath.section == 0)
	{
		return UITableViewCellEditingStyleDelete;
	}
	return UITableViewCellEditingStyleNone;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (editingStyle == UITableViewCellEditingStyleDelete)
	{
		int n = indexPath.row;
		[self deleteDisk:fileDocsList[n]];
		[fileDocsList[n] release];
		if (n + 1 < numDocImages)
		{
			for (int i = n + 1; i < numDocImages; i++)
			{
				fileDocsList[i - 1] = fileDocsList[i];
			}
		}
		fileDocsList[numDocImages - 1] = nil;
		numDocImages--;
		
		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:YES];
		[tableView reloadData];
	}
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	if (indexPath.row <	numDiskImages)
	{
		if (selectedDisk[selectedDrive] != -1)
		{
			for (int i = 0; i < 3; i++)
			{
				if (selectedDisk[i] == -1)
				{
					selectedDrive = i;
					break;
				}
			}
		}
		if (selectedDisk[selectedDrive] == -1)
		{
			selectedSection[selectedDrive] = indexPath.section;
			selectedDisk[selectedDrive] = indexPath.row;
			if (indexPath.section == 0)
			{
				[self insertDisk:selectedDrive :fileDocsList[indexPath.row]];
			}
			else
			{
				[self insertDisk:selectedDrive :fileList[indexPath.row]];
			}
		}
	}
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
	if (section == 0)
	{
		return @"My Images";
	}
	return @"Distribution";
}

+ (void) keyboardPut:(byte)c
{
	keyboardBuf[keyboardEnd++] = c;
	keyboardEnd %= KEYBUF_SIZE;
}

+ (byte) keyboardGet
{
	byte data = keyboardBuf[keyboardBeg++];
	keyboardBeg %= KEYBUF_SIZE;

	return data;
}

- (void) toggleKeyboard
{
	keyboard.visible = (keyboard.visible ? NO : YES);
	if (keyboard.visible)
	{
	}
}

- (BOOL) ccTouchBegan:(UITouch *)touch withEvent:(UIEvent *)event
{
	CGPoint location = [touch locationInView: [touch view]];
	CGPoint convertedLocation = [[CCDirector sharedDirector] convertToGL: location];

	if (helpView != nil)
	{
		return NO;
	}
	/*
	if (CGRectContainsPoint(keyboardIcon.textureRect, convertedLocation))
	{
		[self toggleKeyboard];
		return NO;
	}
	*/
	if (keyboard.visible)
	{
		CGRect r;
		for (int i = 0; i < MAX_KEYS; i++)
		{
			r = CGRectMake(keys[i].x - (keys[i].w / 2), 208 - keys[i].y - (keys[i].h / 2), keys[i].w, keys[i].h);
			if (CGRectContainsPoint(r, convertedLocation))
			{
				[self H19KeyHilite:i];
				return YES;
			}
		}
	}
	
	return YES;
}

- (void) ccTouchMoved:(UITouch *)touch withEvent:(UIEvent *)event
{
	CGPoint location = [touch locationInView: [touch view]];
	CGPoint convertedLocation = [[CCDirector sharedDirector] convertToGL: location];
	
	if (helpView != nil)
	{
		return;
	}
	
	if (keyboard.visible)
	{
		CGRect r;
		for (int i = 0; i < MAX_KEYS; i++)
		{
			r = CGRectMake(keys[i].x - (keys[i].w / 2), 208 - keys[i].y - (keys[i].h / 2), keys[i].w, keys[i].h);
			if (CGRectContainsPoint(r, convertedLocation))
			{
				[self H19KeyHilite:i];
				return;
			}
		}
	}
}

- (void) ccTouchEnded:(UITouch *)touch withEvent:(UIEvent *)event
{
	CGPoint location = [touch locationInView: [touch view]];
	CGPoint convertedLocation = [[CCDirector sharedDirector] convertToGL: location];
	
	if (helpView != nil)
	{
		return;
	}
	
	CGSize winSize = [[CCDirector sharedDirector] winSize];
	
	if (debugMode != 0)
	{
		if (convertedLocation.x < 64 && convertedLocation.y > winSize.height - 64)
		{
			bSingleStep = (bSingleStep ? NO : YES);
			return;
		}
		
		if (bSingleStep)
		{
			if (convertedLocation.x > winSize.width - 64 && convertedLocation.y > winSize.height - 64)
			{
				bStep = YES;
				return;
			}
		}
	}
	else
	{
		//if (convertedLocation.x < 64 && convertedLocation.y > winSize.height - 64)
		if (convertedLocation.y > 350)
		{
			if (fileLibrary == nil)
			{
				[self initTableView];
			}
			return;
		}
	}
#if DEBUG_MODE == 1
	if (convertedLocation.x < 64 && convertedLocation.y < 32)
	{
		debugMode = (debugMode + 1) % 3;
		return;
	}
#endif
	if (convertedLocation.x > winSize.width - 64 && convertedLocation.y < 64)
	{
		[self H19Cls];
		
		int c = 0;
		for (int i = 0; i < 2000; i++)
		{
			int y = (i / 80);
			int x = (i % 80);
			[self H19SetCharAt:x :y :c];
			c = (c + 1) % 256;
		}
		return;
	}
	
	if (keyboard.visible)
	{
		byte c = [self H19KeyPress:convertedLocation];
		if (c != 0)
		{
			if (bOffline)
			{
				[self H19SetChar:c];
			}
			else
			{
				[HelloWorld keyboardPut:c];
			}
		}
	}
}

- (void) loadFont
{
	char romFile[512];
	
	const char *path = [[[NSBundle mainBundle] bundlePath] cStringUsingEncoding:NSASCIIStringEncoding];
	sprintf(romFile, "%s/%s", path, FONT_FILE);
	FILE *fp = fopen(romFile, "rb");
	if (fp != nil)
	{
		fread(RAMFont, sizeof(char), FONT_SIZE, fp);
		fclose(fp);
	}
}

-(void)keywordDetection:(int)c
{
}

- (CGRect)H19FontChar:(int)c
{
	int x = (c * 8) % 128;
	int y = ((c / 16) * 16) % 256;
	return CGRectMake(x, y, 8, 16);
}

// scroll the text up
- (void) H19ScrollUp
{
	int c;
	int s = 80; // source pos
	int d = 0; // dest pos
	int len = (23 * 80);
	for (int i = 0; i < len; i++)
	{
		c = H19ScreenBuffer[s++];
		H19ScreenBuffer[d] = c;
		d++;
	}
	for (int i = d; i < d + 80; i++)
	{
		H19ScreenBuffer[i] = ' ';
	}
	H19HoldScreenCount--;
	if (bH19HoldScreen)
	{
		if (H19HoldScreenCount == 0)
		{
			[HelloWorld keyboardPut:0x13];
		}
	}
}

// scroll the text down
- (void) H19ScrollDown
{
	int c;
	int d = (24 * 80) - 1; // source pos
	int s = d - 80; // dest pos
	int len = (23 * 80);
	for (int i = 0; i < len ; i++)
	{
		c = H19ScreenBuffer[s--];
		H19ScreenBuffer[d] = c;
		d--;
	}
	for (int i = 0; i < 80; i++)
	{
		H19ScreenBuffer[i] = ' ';
	}
}

- (void) H19CursorHome
{
	H19CursorX = 0;
	H19CursorY = 0;
}

- (void) H19CursorUp
{
	if (H19CursorY > 0)
	{
		H19CursorY--;
	}
}

- (void) H19CursorDwn
{
	H19CursorY++;
	if (H19CursorY > 23)
	{
		H19CursorY = 23;
		[self H19ScrollUp];
	}
}

- (void) H19CursorFwd
{
	H19CursorX++;
	if (H19CursorX > 79)
	{
		H19CursorX = 79;
	}
}

- (void) H19CursorBck
{
	if (H19CursorX > 0)
	{
		H19CursorX--;
	}
}

- (void) H19Cls
{
	for (int n = 0; n < 2000; n++)
	{
		H19ScreenBuffer[n] = ' ';
	}
	[self H19CursorHome];
}

- (void) H19EraseLIN:(int)i
{
	int n = (i * 80);
	for (int c = 0; c < 80; c++)
	{
		H19ScreenBuffer[n + c] = ' ';
	}
}

- (void) H19EraseBOP
{
	if (H19CursorY > 0)
	{
		for (int i = 0; i < H19CursorY; i++)
		{
			[self H19EraseLIN:i];
		}
	}
}

- (void) H19EraseEOP
{
	if (H19CursorY < 24)
	{
		for (int i = H19CursorY; i < 24; i++)
		{
			[self H19EraseLIN:i];
		}
	}
}

- (void) H19EraseBOL
{
	if (H19CursorX > 0)
	{
		int n = H19CursorY * 80;
		for (int i = 0; i < H19CursorX; i++)
		{
			H19ScreenBuffer[n + i] = ' ';
		}
	}
}

- (void) H19EraseEOL
{
	if (H19CursorX < 79)
	{
		int n = H19CursorY * 80;
		for (int i = H19CursorX; i < 80; i++)
		{
			H19ScreenBuffer[n + i] = ' ';
		}
	}
}

- (void) H19DeleteChar
{
	int n = (H19CursorY * 80);
	int i = 0;
	for (i = n + H19CursorX; i < n + 79; i++)
	{
		H19ScreenBuffer[i] = H19ScreenBuffer[i + 1];
	}
	H19ScreenBuffer[i] = ' ';
}

- (void) H19DeleteLine
{
}

- (void) H19InsertLine
{
}

- (void) H19SetCharAt:(int)x :(int)y :(byte)c
{
	int n = (y * 80) + x;
	H19ScreenBuffer[n] = c;
	[self keywordDetection:c];
//	if (y >= 24)
//	{
//		buttonView.alpha = 0.2f;
//	}
}

- (void) H19SetChar:(byte)c
{
	if (bH19EscMode)
	{
		if (H19DirectPos != 0)
		{
			if (H19DirectPos == 1)
			{
				H19CursorY = c - 32;
				H19CursorY = (H19CursorY < 0) ? 0 : (H19CursorY > 24) ? 24 : H19CursorY;
				H19DirectPos++;
			}
			else
			{
				H19CursorX = c - 32;
				H19CursorX = (H19CursorX < 0) ? 0 : (H19CursorX > 79) ? 79 : H19CursorX;
				H19DirectPos = 0;
				bH19EscMode = NO;
			}
			return;
		}
		else if (H19SetMode)
		{
			switch (c)
			{
				case '1': // enable 25th line
					break;
				case '3': // enable hold screen mode
					bH19HoldScreen = YES;
					H19HoldScreenCount = 23;
					break;
				case '4': // enable block cursor
					break;
				case '5': // enable cursor off
					bH19CursorOff = YES;
					break;
				case '6': // enable shifted keypad
					break;
				case '7': // enable alternate keypad
					break;
				case '8': // enable auto LF on CR
					break;
				case '9': // enable auto CR on LF
					break;
			}
			H19SetMode = 0;
			bH19EscMode = NO;
		}
		else if (H19ResetMode)
		{
			switch (c)
			{
				case '1': // disable 25th line
					break;
				case '3': // disable hold screen mode
					bH19HoldScreen = NO;
					break;
				case '4': // disable block cursor
					break;
				case '5': // disable cursor off
					bH19CursorOff = NO;
					break;
				case '6': // disable shifted keypad
					break;
				case '7': // disable alternate keypad
					break;
				case '8': // disable auto LF on CR
					break;
				case '9': // disable auto CR on LF
					break;
			}
			H19ResetMode = 0;
			bH19EscMode = NO;
		}
		switch (c)
		{
			// cursor functions
			case 'H':
				[self H19CursorHome];
				break;
			case 'C':
				[self H19CursorFwd];
				break;
			case 'D':
				[self H19CursorBck];
				break;
			case 'B':
				[self H19CursorDwn];
				break;
			case 'A':
				[self H19CursorUp];
				break;
			case 'I': // reverse index
				if (H19CursorY == 0)
				{
					[self H19ScrollDown];
				}
				else
				{
					[self H19CursorUp];
				}
				break;
			case 'n': // cursor position report
				[HelloWorld keyboardPut:0x1B];
				[HelloWorld keyboardPut:'Y'];
				[HelloWorld keyboardPut:H19CursorY+32];
				[HelloWorld keyboardPut:H19CursorX+32];
				break;
			case 'j': // save cursor position
				H19CursorSaveX = H19CursorX;
				H19CursorSaveY = H19CursorY;
				break;
			case 'k': // restore cursor position
				H19CursorX = H19CursorSaveX;
				H19CursorY = H19CursorSaveY;
				break;
			case 'Y': // direct cursor addressing
				H19DirectPos = 1;
				return;
				break;
			// erasing and editing
			case 'E':
				[self H19Cls];
				break;
			case 'b': // erase beginning of display
				[self H19EraseBOP];
				break;
			case 'J': // erase to end of page
				[self H19EraseEOP];
				break;
			case 'l': // erase entire line
				[self H19EraseLIN:H19CursorY];
				break;
			case 'o': // erase beginning of line
				[self H19EraseBOL];
				break;
			case 'K': // erase to end of line
				[self H19EraseEOL];
				break;
			case 'L': // insert line
				[self H19InsertLine];
				break;
			case 'M': // delete line
				[self H19DeleteLine];
				break;
			case 'N': // delete character
				[self H19DeleteChar];
				break;
			case '@': // enter insert character mode
				break;
			case 'O': // exit insert character mode
				break;
			// graphics
			case 'F': // enter graphics mode
				bH19Graphics = YES;
				break;
			case 'G': // exit graphics mode
				bH19Graphics = NO;
				break;
			case 'p': // reverse video mode
				bH19Reverse = YES;
				break;
			case 'q': // exit reverse video mode
				bH19Reverse = NO;
				break;
			case 'x':
				H19SetMode = 1;
				return;
				break;
			case 'y':
				H19ResetMode = 1;
				return;
				break;
			case 'z':
				[self H19Cls];
				bH19Reverse = NO;
				bH19Graphics = NO;
				bH19CursorOff = NO;
				break;
		}
		bH19EscMode = NO;
		return;
	}
	if (c >= ' ' && c <= '~')
	{
		// graphics mode remap
		if (bH19Graphics && c >= 94 && c <= 126)
		{
			if (c == 94)
			{
				c = 127;
			}
			else if (c == 95)
			{
				c = 31;
			}
			else
			{
				if (c >= 96)
				{
					c -= 96;
				}
			}
		}
		// reverse video mode
		if (bH19Reverse)
		{
			c += 128;
		}
		[self H19SetCharAt:H19CursorX :H19CursorY :c];
		[self H19CursorFwd];
	}
	else if (c == 0x07) // bell
	{
	}
	else if (c == 0x08) // backspace
	{
		[self H19CursorBck];
	}
	else if (c == 0x09) // tab
	{
		H19CursorX = ((H19CursorX + 8) / 8) * 8;
		H19CursorX = (H19CursorX > 79) ? 79 : H19CursorX;
	}
	else if (c == 0x0A) // linefeed
	{
		[self H19CursorDwn];
	}
	else if (c == 0x0D) // return
	{
		H19CursorX = 0;
	}
	else if (c == 0x1B) // ESC
	{
		bH19EscMode = YES;
	}
}

- (void) H19PrintAt:(int)x :(int)y :(NSString *)s
{
	const char *str = [s UTF8String];
	for (int i = 0; i < strlen(str); i++)
	{
		byte c = str[i];
		[self H19SetCharAt:x + i :y :c];
	}
}

- (void) H19KeyHiliteOff
{
	for (int i = 0; i < 4; i++)
	{
		keyHilite1[i].visible = NO;
		if (i < 2)
		{
			keyHilite2[i].visible = NO;
			keyHilite3[i].visible = NO;
		}
	}
	keyHilite4.visible = NO;
}

- (void) H19KeyHilite:(int)i
{
	[self H19KeyHiliteOff];
	
	if (bKeyShift)
	{
		keyHilite2[1].visible = YES;
		keyHilite3[1].visible = YES;
	}
	if (bOffline)
	{
		keyHilite1[1].visible = YES;
	}
	if (bKeyCtrl)
	{
		keyHilite1[2].visible = YES;
	}
	if (bCapsLock)
	{
		keyHilite1[3].visible = YES;
	}
	switch (i)
	{
		case 27:
			keyHilite2[0].visible = YES;
			keyHilite2[0].position = ccp(keys[i].x, 208 - keys[i].y);
			keyHilite2[0].opacity = 255;
			break;
		case KEY_SHIFTL:
		case KEY_SHIFTR:
			if (bKeyShift)
			{
				keyHilite3[1].visible = YES;
				keyHilite3[1].position = ccp(keys[KEY_SHIFTR].x, 208 - keys[KEY_SHIFTR].y);
				keyHilite3[1].opacity = 255;
				keyHilite2[1].visible = YES;
				keyHilite2[1].position = ccp(keys[KEY_SHIFTL].x, 208 - keys[KEY_SHIFTL].y);
				keyHilite2[1].opacity = 255;
			}
			break;
		case 56:
			keyHilite3[0].visible = YES;
			keyHilite3[0].position = ccp(keys[i].x, 208 - keys[i].y);
			keyHilite3[0].opacity = 255;
			break;
		case 71:
			keyHilite4.visible = YES;
			keyHilite4.position = ccp(keys[i].x, 208 - keys[i].y);
			keyHilite4.opacity = 255;
			break;
		case KEY_CAPSLOCK:
			if (bCapsLock)
			{
				keyHilite1[3].visible = YES;
				keyHilite1[3].position = ccp(keys[i].x, 208 - keys[i].y);
				keyHilite1[3].opacity = 255;
			}
			break;
		case KEY_CTRL:
			if (bKeyCtrl)
			{
				keyHilite1[2].visible = YES;
				keyHilite1[2].position = ccp(keys[i].x, 208 - keys[i].y);
				keyHilite1[2].opacity = 255;
			}
			break;
		case KEY_OFFLINE:
			if (bOffline)
			{
				keyHilite1[1].visible = YES;
				keyHilite1[1].position = ccp(keys[i].x, 208 - keys[i].y);
				keyHilite1[1].opacity = 255;
			}
			break;
		default:
			keyHilite1[0].visible = YES;
			keyHilite1[0].position = ccp(keys[i].x, 208 - keys[i].y);
			keyHilite1[0].opacity = 255;
			break;
	}
}

- (BOOL) H19KeyPressSpecial:(int)i
{
	switch (i)
	{
		case KEY_OFFLINE:
			bOffline = (bOffline ? NO : YES);
			return YES;
			break;
		case KEY_F1:
			return YES;
			break;
		case KEY_F2:
			return YES;
			break;
		case KEY_F3:
			return YES;
			break;
		case KEY_F4:
			return YES;
			break;
		case KEY_F5:
			return YES;
			break;
		case KEY_ERASE:
			if (bOffline)
			{
				if (bKeyShift)
				{
					[self H19Cls];
				}
			}
			return YES;
			break;
		case KEY_BLUE:
			return YES;
			break;
		case KEY_RED:
			return YES;
			break;
		case KEY_GRAY:
			return YES;
			break;
		case KEY_RESET:
			if (bKeyShift)
			{
				// reset computer
				bReset = YES;
			}
			return YES;
			break;
		case KEY_BREAK:
			return YES;
			break;
		case KEY_CTRL:
			bKeyCtrl = (bKeyCtrl ? NO : YES);
			return YES;
			break;
		case KEY_CAPSLOCK:
			bCapsLock = (bCapsLock ? NO : YES);
			return YES;
			break;
		case KEY_SCROLL:
			if (bH19HoldScreen)
			{
				if (bKeyCtrl)
				{
					bH19HoldScreen = NO;
					bKeyCtrl = NO;
				}
				else if (bKeyShift)
				{
					H19HoldScreenCount = 23;
					bKeyShift = NO;
				}
				else
				{
					H19HoldScreenCount = 1;
				}
				[HelloWorld keyboardPut:0x11];
			}
			else
			{
				bH19HoldScreen = YES;
				H19HoldScreenCount = 23;
			}
			return YES;
			break;
		case KEY_SHIFTL:
		case KEY_SHIFTR:
			bKeyShift = (bKeyShift ? NO : YES);
			return YES;
			break;
		case KEY_REPEAT:
			return YES;
			break;
	}
	return NO;
}

- (byte) H19KeyPress:(CGPoint)p
{
	CGRect r;
	
	for (int i = 0; i < MAX_KEYS; i++)
	{
		r = CGRectMake(keys[i].x - (keys[i].w / 2), 208 - keys[i].y - (keys[i].h / 2), keys[i].w, keys[i].h);
		if (CGRectContainsPoint(r, p))
		{
			if (keyHilite1[0].visible)
			{
				[keyHilite1[0] runAction:[CCFadeOut actionWithDuration:0.5f]];
			}
			else if (keyHilite2[0].visible)
			{
				[keyHilite2[0] runAction:[CCFadeOut actionWithDuration:0.5f]];
			}
			else if (keyHilite3[0].visible)
			{
				[keyHilite3[0] runAction:[CCFadeOut actionWithDuration:0.5f]];
			}
			else if (keyHilite4.visible)
			{
				[keyHilite4 runAction:[CCFadeOut actionWithDuration:0.5f]];
			}
			
			if ([self H19KeyPressSpecial:i])
			{
				[self H19KeyHilite:i];
				return 0;
			}
			
			byte c;
			if (bKeyShift)
			{
				c = keys[i].shift_c;
			}
			else
			{
				c = keys[i].c;
			}
			if (bKeyCtrl)
			{
				c &= 0x1F;
			}
			if (c >= 'a' && c <= 'z' && bCapsLock)
			{
				c -= 0x20;
			}
			
			bKeyShift = NO;
			bKeyCtrl = NO;
			
			[self H19KeyHilite:i];
			
			return c;
		}
	}
	return 0;
}

- (void) H19KeyboardInit
{
	CGSize winSize = [[CCDirector sharedDirector] winSize];
	/*
	keyboardIcon = [CCSprite spriteWithFile:@"Keyboard_icon_2.png"];
	keyboardIcon.position = ccp(winSize.width - 120, winSize.height - 35);
	keyboardIcon.opacity = 128;
	keyboardIcon.scale = 0.5f;
	[self addChild:keyboardIcon];
	*/
	keyboard = [CCSprite spriteWithFile:@"keyboard2.png"];
	keyboard.position = ccp(winSize.width / 2, 208 / 2);
	[self addChild:keyboard];
	
	for (int i = 0; i < 4; i++)
	{
		keyHilite1[i] = [CCSprite spriteWithFile:@"keyhilite1.png"];
		keyHilite1[i].visible = NO;
		[self addChild:keyHilite1[i]];
	}
	
	for (int i = 0; i < 2; i++)
	{
		keyHilite2[i] = [CCSprite spriteWithFile:@"keyhilite2.png"];
		keyHilite2[i].visible = NO;
		[self addChild:keyHilite2[i]];
	}
	
	for (int i = 0; i < 2; i++)
	{
		keyHilite3[i] = [CCSprite spriteWithFile:@"keyhilite3.png"];
		keyHilite3[i].visible = NO;
		[self addChild:keyHilite3[i]];
	}
	
	keyHilite4 = [CCSprite spriteWithFile:@"keyhilite4.png"];
	keyHilite4.visible = NO;
	[self addChild:keyHilite4];
	
	UIView *view = [ViewController view];

	int w = [[CCDirector sharedDirector] winSize].width;
	int h = 35;
	int y = [[CCDirector sharedDirector] winSize].height - 245;
	int x = 0;
	CGRect r = CGRectMake(x, y, w, h);
	buttonView = [[[UIView alloc] initWithFrame:r] autorelease];
	buttonView.backgroundColor = [UIColor lightGrayColor];
	buttonView.alpha = 0.3f;
	
	y = 3;
	UIButton *mountButton = [self initButton:x :y :80 :30 :20 :@"MOUNT"];
	[buttonView addSubview:mountButton];
	x += 100;

	UIButton *dirButton = [self initButton:x :y :80 :30 :21 :@"DIR"];
	[buttonView addSubview:dirButton];
	x += 100;
	
	UIButton *setButton = [self initButton:x :y :80 :30 :25 :@"SET"];
	[buttonView addSubview:setButton];
	x += 100;
	
	UIButton *sy0Button = [self initButton:x :y :80 :30 :22 :@"SY0:"];
	[buttonView addSubview:sy0Button];
	x += 100;
	
	UIButton *sy1Button = [self initButton:x :y :80 :30 :23 :@"SY1:"];
	[buttonView addSubview:sy1Button];
	x += 100;
	
	UIButton *sy2Button = [self initButton:x :y :80 :30 :24 :@"SY2:"];
	[buttonView addSubview:sy2Button];
	x += 100;
	
	UIButton *ctldButton = [self initButton:x :y :40 :30 :26 :@"^D"];
	[buttonView addSubview:ctldButton];
	x += 50;
	
	UIButton *ctlcButton = [self initButton:x :y :40 :30 :27 :@"^C"];
	[buttonView addSubview:ctlcButton];
	x += 50;
	
//	UIButton *h19Button = [self initButton:x :y :80 :30 :30 :@"H19"];
//	[buttonView addSubview:h19Button];
//	x += 100;
	
	x = [[CCDirector sharedDirector] winSize].width - 80;
	UIButton *bootButton = [self initButton:x :y :80 :30 :28 :@"BOOT"];
	[buttonView addSubview:bootButton];

	x -= 100;
	UIButton *resetButton = [self initButton:x :y :80 :30 :29 :@"RESET"];
	[buttonView addSubview:resetButton];
	
	[view addSubview:buttonView];
}

- (void) H19Init
{
	ccColor4B data[128 * 256]; // 128 x 256 texture sheet
	
	[self loadFont];
	
	// decode the font into a 128 x 256 sprite sheet
	int n = 0;
	for (int i = 0; i < 128; i++)
	{
		// offset into data[] holding buffer
		int y = ((i * 8) / 128) * (16 * 128);
		int x = (i * 8) % 128;
		// pointer to the bytes defining the font
		byte *p = &RAMFont[i * 16];
		for (int j = 0; j < 16; j++) // convert RAMFont[] to 32-bit data[] array
		{
			for (int k = 0; k < 8; k++) // look at each bit
			{
				ccColor4B color = ccc4(0, 0, 0, 255);
				ccColor4B rev_color = ccc4(0, 255, 0, 255);
				int b = (0x80 >> k);
				if ((p[j] & b) != 0)
				{
					color = ccc4(0, 255, 0, 255);
					rev_color = ccc4(0, 0, 0, 255);
				}
				n = y + (j * 128) + x + k;
				data[n] = color;
				n += 16384;
				data[n] = rev_color;
			}
		}
	}
	int screenHeight = [[CCDirector sharedDirector] winSize].height;
	
	H19FontTexture = [[CCTexture2D alloc] initWithData:data pixelFormat:kCCTexture2DPixelFormat_Default pixelsWide:128 pixelsHigh:256 contentSize:CGSizeMake(128, 256)];
	
	H19ScreenBatch = [CCSpriteBatchNode batchNodeWithTexture:H19FontTexture capacity:2000];
	[self addChild:H19ScreenBatch];
	
	// place the character cells into position
	n = 0;
	float font_width = (8 * FONT_SCALEX);
	float font_height = (10 * FONT_SCALEY);
	for (int j = 0; j < 25; j++)
	{
		float x = (font_width / 2);
		float y = (j * font_height);
		for (int i = 0; i < 80; i++)
		{
			int c = 32; //n % 256;
			H19Screen[n] = [CCSprite spriteWithTexture:H19ScreenBatch.texture rect:[self H19FontChar:c]];
			H19Screen[n].position = ccp(x, screenHeight - font_height - y);
			H19Screen[n].scaleX = FONT_SCALEX;
			H19Screen[n].scaleY = FONT_SCALEY;
			[H19ScreenBatch addChild:H19Screen[n]];
			x += font_width;
			n++;
		}
	}
	
	H19Cursor = [CCSprite spriteWithTexture:H19FontTexture rect:[self H19FontChar:27]];
	[self addChild:H19Cursor];
	
	[self H19Cls];

	[self H19KeyboardInit];
	
	/*
	int screenWidth = [[CCDirector sharedDirector] winSize].width;
	H19Screen[0] = [CCSprite spriteWithTexture:H19FontTexture];
	H19Screen[0].position = ccp(screenWidth / 2, screenHeight / 2);
	[self addChild:H19Screen[0]];
	 */
}

- (void) H19DrawScreenBuffer
{
	int c;
	int len = (25 * 80);
	for (int i = 0; i < len; i++)
	{
		c = H19ScreenBuffer[i];
		H19Screen[i].textureRect = [self H19FontChar:c];
	}
}

- (BOOL) H17IsHDOSDisk:(int)drive
{
	byte c0 = DiskImage[drive][0];
	byte c1 = DiskImage[drive][1];
	byte c2 = DiskImage[drive][2];
	byte c3 = DiskImage[drive][3];
	if ((c0 == 0xAF && c1 == 0xD3 && c2 == 0x7D && c3 == 0xCD) ||
		(c0 == 0xC3 && c1 == 0xA0 && c2 == 0x22 && c3 == 0x20) ||
		(c0 == 0xC3 && c1 == 0xA0 && c2 == 0x22 && c3 == 0x30))
	{
		return YES;
	}
	return NO;
}

- (NSString *) H17GetHDOSLabel:(int)drive
{
	char s[61];
	
	int offset = [self H17GetImageOffset:drive :0 :0 :9];
	offset += 17;
	char *label = (char *)&DiskImage[drive][offset];
	int n = 59;
	// strip trailing spaces
	while (label[n] <= ' ' || label[n] >= 127)
	{
		n--;
	}
	int len = n + 1;
	n = 0;
	for (int i = 0; i < len; i++)
	{
		if (label[i] >= '0' && label[i] <= '9')
		{
			s[n++] = label[i];
		}
		else if (label[i] >= 'A' && label[i] <= 'Z')
		{
			s[n++] = label[i];
		}
		else if (label[i] >= 'a' && label[i] <= 'z')
		{
			s[n++] = label[i];
		}
		else if ((label[i] == '_' || label[i] == '-') && n > 0)
		{
			s[n++] = label[i];
		}
		else if (label[i] == ' ' && n > 0)
		{
			s[n++] = '_';
		}
	}
	s[n] = 0;
	// test for an unlabeled HDOS disk
	if (strncmp(s, "GLGLGLGLGL", 10) == 0)
	{
		s[0] = 0;
	}
	return [NSString stringWithFormat:@"%s", s];
}

- (void) H17SetDiskParameters:(int)drive
{
	byte vol_flags = 0;
	H17DiskVolume[drive] = 0x00;
	//  check for HDOS disk image
	if ([self H17IsHDOSDisk:drive])
	{
		H17DiskVolume[drive] = (byte)DiskImage[drive][0x900];
		vol_flags = DiskImage[drive][0x910];
		switch (vol_flags)
		{
			case 0:
				H17DiskTracks[drive] = 40;
				H17DiskSides[drive] = 1;
				break;
			case 1:
				H17DiskTracks[drive] = 40;
				H17DiskSides[drive] = 2;
				break;
			case 2:
				H17DiskTracks[drive] = 80;
				H17DiskSides[drive] = 1;
				break;
			case 3:
				H17DiskTracks[drive] = 80;
				H17DiskSides[drive] = 2;
				break;
			default:
				H17DiskTracks[drive] = 40;
				H17DiskSides[drive] = 1;
				break;
		}
	}
	else
	{
		H17DiskTracks[drive] = 40;
		H17DiskSides[drive] = 1;
		/*
		if (DiskImage[drive].DiskImageBuffer.Length < 200000)
		{
			DiskImage[drive].Tracks = 40;
			DiskImage[drive].Sides = 1;
			SetDriveSpecs(drive, 0);
		}
		else if (DiskImage[drive].DiskImageBuffer.Length < 400000)
		{
			DiskImage[drive].Tracks = 80;
			DiskImage[drive].Sides = 1;
			SetDriveSpecs(drive, 2);
		}
		else
		{
			DiskImage[drive].Tracks = 80;
			DiskImage[drive].Sides = 2;
			SetDriveSpecs(drive, 3);
		}
		*/
	}
}

// compute disk image position
- (int) H17GetImageOffset:(int)drive :(int)side :(int)track :(int)sector
{
	int sides = H17DiskSides[drive];
//	int tracks = H17DiskTracks[drive];
	int step_size = TRACK_SIZE;
//	if (tracks == 40)
//	{
//		step_size *= 2;
//	}
	int offset = (track * sides * step_size) + (side * TRACK_SIZE) + (sector * MAX_SECSIZE);
	return offset;
}

// write a byte to the disk image
- (void) H17WriteByte:(byte)c
{
	PortData[PORT174Q] = c;
	PortData[PORT175Q] &= ~H17STAT_TBE;
	H17CharIndex++; // index for bytes being written to the disk image
}

- (void) H17WriteNextByte:(byte)c
{
	int index, n;
	int image_index = H17Drive;
	switch (H17CharIndex)
	{
		case 1: //  volume number
			H17DiskVolume[image_index] = c;
			if (H17Side != 0)
			{
				H17DiskSides[image_index] = 2;
			}
			break;
		case 2: //  track number
			H17Track = c / H17DiskSides[image_index];
			break;
		case 3:
			H17Sector = H17WriteSector;
			H17Index = H17Sector;
			H17WriteSector = (H17WriteSector + 1) % 10;
			break;
		case 4:
			H17ReadChecksum = c;
			bH17Write = NO; // the sync char will turn this back on
			//NSLog(@"> Drive=%d Track=%02d Side=%02d Sector=%02d begin", image_index, H17Track, H17Side, H17Sector);
			break;
		case 5:
			if (c != 0xFD)
			{
				NSLog(@"H17WriteNextByte at index 5 is not sync (%d) Track=%d Sector=%d", c, H17Track, H17Sector);
			}
			c = 0xFD;
			H17ReadChecksum = c;
			//H17ControlBits |= H17CTRL_SYN;
			break;
		case 262:
			NSLog(@"> Drive=%d Track=%02d Side=%02d Sector=%02d write", image_index, H17Track, H17Side, H17Sector);
			PortData[PORT175Q] |= H17STAT_TBE;
			H17ReadChecksum = c;
			H17CharIndex = 0;
			if (H17Track >= 40 && H17DiskTracks[H17Drive] != 80)
			{
				//[self imageReorderFor80T:H17Drive];
				H17DiskTracks[H17Drive] = 80;
			}
			H17Dirty[H17Drive] = 1; // mark this for saving
			bH17Write = NO; // the sync char will turn this back on
			return;
			break;
		default:
			index = [self H17GetImageOffset:image_index :H17Side :H17Track :H17Sector];
			n = index + (H17CharIndex - 6);
			DiskImage[image_index][n] = c;
			H17SectorBuffer[H17CharIndex - 6] = c;
			break;
	}
	[self H17WriteByte:c];
}

// read a byte from the disk image - puts data in port 174Q and sets receive data bit
- (void) H17ReadByte:(byte)c
{
	PortData[PORT174Q] = c;
	H17ReadChecksum ^= c;
	H17ReadChecksum <<= 1;
	if ((H17ReadChecksum & 0x0100) != 0) // went through the carry-bit, wrap back to bit 1
	{
		H17ReadChecksum = (H17ReadChecksum & 0xFF) | 0x01;
	}
	PortData[PORT175Q] |= H17STAT_RDR; // receive data available
	H17CharIndex++; // index for bytes being read from the disk image
}

- (void) H17ReadNextByte
{
	int index, n;
	int image_index = H17Drive;
	int track = H17Track;
	byte c = 0x00;
	switch (H17CharIndex)
	{
		case 1: //  volume number
			if (track == 0)
			{
				c = 0x00;
			}
			else
			{
				c = (byte)H17DiskVolume[image_index];
			}
			break;
		case 2: //  track number
			c = (byte)track;
			break;
		case 3:
			c = (byte)H17Sector;
			break;
		case 4:
			c = (byte)H17ReadChecksum;
			break;
		case 5:
			c = 0xFD;
			H17ReadChecksum = c;
			H17ControlBits |= H17CTRL_SYN;
			break;
		case 263:
			c = (byte)H17ReadChecksum;
			break;
		case 264:
			NSLog(@"< Drive=%d Track=%02d Side=%02d Sector=%02d read", image_index, H17Track, H17Side, H17Sector);
			PortData[PORT175Q] &= ~H17STAT_RDR;
			H17CharIndex = 0;
			
//			if (debugMode != 0)
//			{
//				if (H17Sector == 1 && H17Track == 5)
//				{
//					debugMode = 2;
//					bSingleStep = YES;
//				}
//			}
			return;
		default:
			index = [self H17GetImageOffset:image_index :H17Side :H17Track :H17Sector];
			n = index + (H17CharIndex - 7);
			c = DiskImage[image_index][n];
			break;
	}
	[self H17ReadByte:c];
}

- (void) H17Service
{
	if ((PortData[PORT177Q] & H17CTRL_MON) == H17CTRL_MON) // motor on
	{
		if (H17CharIndex != 0)
		{
			// don't do an index or sector step if we're reading a track
			return;
		}
		if ((H17ControlBits & H17CTRL_HOL) == H17CTRL_HOL) // hole detect is on
		{
			// service index hole on routine
			if ((H17DiskTime % H17HoleTime) == 0)
			{
				H17ControlBits &= ~H17CTRL_HOL; // turn off hole detect bit
			}
		}
		else
		{
			// service index hole off routine
            int delay = H17SectorTime;
            switch (H17Index)
            {
                case 9:
                case 10:
                    delay = H17SectorTime / 2;
                    break;
            }
            if ((H17DiskTime % delay) == 0)
            {
				H17Index = (H17Index + 1) % 11;
				if (H17Index != 10)
				{
					H17Sector = H17Index;
				}
				H17ControlBits |= H17CTRL_HOL; // set hole detect bit
            }
		}
	}
}

- (void) H17Reset
{
	PortData[PORT177Q] = 0;
	H17Drive = 0;
	H17ControlBits = 0;
	H17CharIndex = 0;
	H17Index = 0;
	H17Sector = 0;
	H17Track = 0;
}

- (void) startCPU
{
	Z80CPU.IAutoReset = 1;
	Z80CPU.IPeriod = 2048; // (2.048Mhz = 2,048,000) / (1ms = 0.001)
	ResetZ80(&Z80CPU);
	
	if (self.emuThread == nil)
	{
		self.emuThread = [[Z80EmuThread alloc] autorelease];
	}
	if (!self.emuThread.started)
	{
		[self.emuThread start];
	}
/*
	if (self.H19Thread == nil)
	{
		self.H19Thread = [[H19EmuThread alloc] autorelease];
	}
	if (!self.H19Thread.started)
	{
		[self.H19Thread start];
	}
*/
}

- (void) speedToggle:(id)sender
{
	UISwitch *s = (UISwitch *)sender;
	b2MHz = s.on;
	if (b2MHz)
	{
		ccTexParams parms;
		parms.minFilter = GL_NEAREST;
		parms.magFilter = GL_NEAREST;
		parms.wrapS = 0;
		parms.wrapT = 0;
		[H19FontTexture setTexParameters:&parms];
	}
	else
	{
		ccTexParams parms;
		parms.minFilter = GL_LINEAR;
		parms.magFilter = GL_LINEAR;
		parms.wrapS = 0;
		parms.wrapT = 0;
		[H19FontTexture setTexParameters:&parms];
	}
}

- (NSString *) toOctal:(int)addr
{
	NSString *s = [NSString stringWithFormat:@"%03O.%03O", addr / 256, addr % 256];
	return s;
}

- (void) update:(ccTime)deltaTime
{
	float font_width = 8 * FONT_SCALEX;
	float font_height = 10 * FONT_SCALEY;
	int screenHeight = [[CCDirector sharedDirector] winSize].height;
	H19Cursor.position = ccp((font_width / 2) + (H19CursorX * font_width), screenHeight - font_height - (H19CursorY * font_height));

	if (bH19CursorOff)
	{
		H19Cursor.visible = NO;
	}
	else
	{
		H19CursorBlink = (H19CursorBlink + 1) % 40;
		if (H19CursorBlink >= 20)
		{
			H19Cursor.visible = YES;
		}
		else
		{
			H19Cursor.visible = NO;
		}
	}

	if (keyboardBeg != keyboardEnd)
	{
#ifdef IE_Rx_Ready
		[ConsolePort PutData:[HelloWorld keyboardGet]];
		if (ConsolePort.m_bINT)
		{
			Z80CPU.IRequest = INT_RST18;
		}
#else
		int channel = U8250GetBaseChannel(PORT350Q);
		U8250PortRegister[channel][UR_RBR] = [HelloWorld keyboardGet];
		U8250PortRegister[channel][UR_LSR] |= 0x01;
		if ((U8250PortRegister[channel][UR_IER] & IER_RDA) != 0)
		{
			U8250Interrupt(channel, IIR_RDA);
		}
#endif
	}
	
	[self H19DrawScreenBuffer];
	
	// if a disk image is dirty and there is no drive activity, save it to the documents folder
	if ((PortData[PORT177Q] & H17CTRL_MON) == 0) // motor not on
	{
		for (int i = 0; i < 3; i++)
		{
			if (H17Dirty[i] != 0)
			{
				if (selectedDisk[i] != -1)
				{
					NSString *disk_name = nil;
					if (selectedSection[i] == 0)
					{
						disk_name = fileDocsList[selectedDisk[i]];
					}
					else
					{
						disk_name = fileList[selectedDisk[i]];
					}
					[self saveDisk:i :disk_name];
				}
				else
				{
					if (DiskImage[i][0] != 0) // make sure it's not an empty image
					{
						NSString *disk_name = [self getDiskName:i];
						if (disk_name != nil)
						{
							selectedDisk[i] = numDocImages;
							[self saveDisk:i :disk_name];
						}
					}
				}
				H17Dirty[i] = 0;
			}
		}
	}
	
	if (debugMode != 0)
	{
		Z80 *R = &Z80CPU;
		
		int x = 60;
		int y = 0;
		NSString *r = nil;
		if (debugMode == 2)
		{
			r = [NSString stringWithFormat:@"  AF %02X.%02X", R->AF.B.h, R->AF.B.l];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
			r = [NSString stringWithFormat:@"  BC %02X.%02X", R->BC.B.h, R->BC.B.l];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
			r = [NSString stringWithFormat:@"  DE %02X.%02X", R->DE.B.h, R->DE.B.l];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
			r = [NSString stringWithFormat:@"  HL %02X.%02X", R->HL.B.h, R->HL.B.l];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
			r = [NSString stringWithFormat:@"  PC %04X", R->PC.W];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
			r = [NSString stringWithFormat:@"  SP %04X", R->SP.W];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
			r = [NSString stringWithFormat:@"   M %02X", (byte)RAM[R->HL.W]];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
#ifdef DEBUG
			byte dasmstr[256];
			y++;
			int pc = R->PC.W;
			for (int i = 0; i < 12; i++)
			{
				int m = pc;
				pc += DAsm(dasmstr, pc);
				r = [NSString stringWithFormat:@"%04X %s", m, dasmstr];
				[HelloWorldPtr H19PrintAt:x :y+i :@"                    "];
				[HelloWorldPtr H19PrintAt:x :y+i :r];
			}
#endif
		}
		else if (debugMode == 1)
		{
			r = [NSString stringWithFormat:@"PC %04X", R->PC.W];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
			r = [NSString stringWithFormat:@"SP %04X", R->SP.W];
			[HelloWorldPtr H19PrintAt:x :y :r];
			y++;
		}
		NSString *mem = @"RW";
		if ((PortData[PORT362Q] & 0x20) == 0 && (PortData[PORT177Q] & H17CTRL_WEN) == 0)
		{
			mem = @"RO";
		}
		NSString *s = [NSString stringWithFormat:@"(174Q):%02X (175Q):%02X (177Q):O=%02X I=%02X T%02d S%02d RI=%03d MEM=%@ (%d)",
					   PortData[PORT174Q], PortData[PORT175Q], PortData[PORT177Q], H17ControlBits, H17Track, H17Sector, H17CharIndex, mem, H17DiskTime];
		[HelloWorldPtr H19PrintAt:0 :24 :s];
	}
}

@end
