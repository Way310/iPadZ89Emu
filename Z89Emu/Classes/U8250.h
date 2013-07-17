//
//  U8250.h
//  Z89Emu
//
//  Created by Les Bird on 3/30/11.
//  Copyright 2011 Les Bird. All rights reserved.
//

#import <Foundation/Foundation.h>

//	#defines for "interesting" control bits:

//	IER bit assignments
#define IE_Rx_Ready			0x001
#define	IE_Tx_Empty			0x002
#define	IE_Rx_Status		0x004
#define	IE_Modem_Delta		0x008
//	IID bit assignments
#define	INT_Pending			0x001
#define	INT_Id				0x006
//	IID interrupt Id values
#define	INT_ID_Rx_Status	0x006
#define	INT_ID_Rx_Data		0x004
#define	INT_ID_Tx_Empty		0x002
#define	INT_ID_Modem		0x000
//	LCR bit assignments
#define	DLA					0x080
#define	Send_Break			0x040
#define	Stick_Parity		0x020
#define	Ignore_Parity		0x010
#define	Odd_Parity			0x008
#define	Two_Stop_Bits		0x004		//	1.5 if WL_5
#define	Word_Length			0x003
//	Word_Lengths
#define	WL_8				0x003
#define	WL_7				0x002
#define	WL_6				0x001
#define	WL_5				0x000
//	MCR bits
#define	MCR_Loop			0x010
#define	MCR_OUT2			0x008
#define	MCR_OUT1			0x004
#define	MCR_RTS				0x002
#define	MCR_DTR				0x001
//	LSR Bits
#define	TSE					0x040
#define	TXE					0x020
#define	Rx_Break			0x010
#define	Rx_FE				0x008
#define	Rx_PE				0x004
#define	Rx_OE				0x002
#define	Rx_Ready			0x001
//	MSR Bits
#define	MSR_CD				0x080
#define	MSR_RI				0x040
#define	MSR_DSR				0x020
#define	MSR_CTS				0x010
#define	MSR_CD_Changed		0x008
#define	MSR_RI_Falls		0x004
#define	MSR_DSR_Changed		0x002
#define	MSR_CTS_Changed		0x001

@interface U8250 : NSObject
{
	//	Attributes:
	BOOL	m_bINT;				//	Interrupt output
	BOOL	m_bLoopback;		//	Loopback mode bit
	char	m_cTxData;			//	Transmitter holding register
	char	m_cRxData;			//	Receiver Data register
	int		m_nDLL;				//	Divisor Latch 7-0
	int		m_nDLH;				//	Divisor Latch 15-8
	int		m_nIER;				//	Interrupt Enable Reg.
	int		m_nIID;				//	Interrupt ID Reg.
	int		m_nLCR;				//	Line Control Reg.
	int		m_nMCR;				//	Modem Control Reg.
	int		m_nLSR;				//	Line Status Reg.
	int		m_nMSR;				//	Modem Status Reg.
	BOOL	m_bDCD;				//	Carrier detect input
	BOOL	m_bRI;				//	Ring indicator input
	BOOL	m_bDSR;				//	Data Set Ready input
	BOOL	m_bCTS;				//	Clear to Send input
	BOOL	m_bDTR;				//	Data Terminal Ready output
	BOOL	m_bRTS;				//	Request to Send output
	BOOL	m_bOUT1;			//	OUT1 output
	BOOL	m_bOUT2;			//	OUT2 output
	BOOL	m_bDDCD;			//	Delta DCD (status)
	BOOL	m_bDCTS;			//	Delta CTS (status)
	BOOL	m_bTERI;			//	Trailing edge of RI (status)
	BOOL	m_bDDSR;			//	Delta DSR (status)
	BOOL	m_bTHRE;			//	Transmit holding register empty (status)
	BOOL	m_bTSE;				//	Transmit Shift Register Empty (status)
	BOOL	m_bRBR;				//	Receiver Buffer Register Ready (status)
	BOOL	m_bPE;				//	Parity Error detected (status)
	BOOL	m_bFE;				//	Framing Error detected (status)
	BOOL	m_bOE;				//	Overrun Error detected (status)
	BOOL	m_bBRK;				//	BREAK detected (status)
	char	m_cRxD;				//	Received character input (housekeeping)
	BOOL	m_bRSR;				//	Receiver shift register ready (housekeeping)
	char	m_cTxD;				//	Transmit character output (housekeeping)
	BOOL	m_bTDR;				//	Transmit data ready (housekeeping)	
}

@property BOOL m_bINT;

-(void)InitUART;
-(void)Reset;
-(void)Write:(int)nAddress :(char)nData;
-(char)Read:(int)nAddress;
-(BOOL)InterruptSignal;
-(BOOL)InterruptState;
-(int)InterruptVector;
-(char)GetData;
-(void)PutData:(char)cRxD;
//-(BOOL)UpdateDeviceFromUART:(BOOL)&bDTR :(BOOL)&bRTS :(BOOL)&bOUT1 :(BOOL)&bOUT2 :(char)&cTxD :(BOOL)&bSendBreak;
//-(void)UpdateUARTFromDevice:(BOOL)bUpdateMSR :(BOOL)bDCD :(BOOL)bRI :(BOOL)bDSR :(BOOL)bCTS :(BOOL)bUpdateLSR :(char)cRxD :(BOOL)bCauseOE :(BOOL)bCausePE :(BOOL)bCauseFE :(BOOL)bCauseBreak;

@end
