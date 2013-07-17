//
//  U8250.m
//  Z89Emu
//
//  Created by Les Bird on 3/30/11.
//  Copyright 2011 Les Bird. All rights reserved.
//

#import "U8250.h"


@implementation U8250

@synthesize m_bINT;

-(void)InitUART
{
	//	Set the Divisor Latch to zero.
	
	m_nDLL = 0;
	m_nDLH = 0;
	
	//	Set all input pins to defaults:
	
	m_bDCD = FALSE;				//	Carrier detect input
	m_bRI = FALSE;				//	Ring indicator input
	m_bDSR = FALSE;				//	Data Set Ready input
	m_bCTS = FALSE;				//	Clear to Send input
	
	//	Initialize Receive and Transmit data registers:
	
	m_cTxData = 0;				//	Transmitter holding register
	m_cRxData = 0;				//	Receiver Data Buffer register
	m_cRxD = 0;					//	Received character input (shift reg.)
	m_cTxD = 0;					//	Transmit character output (shift reg.)
	
	//	Set data handshaking signals to idle state:
	
	m_bTDR = FALSE;				//	Transmit data ready (output)
	
	//	Call Reset() to initialize the remaining data members.
	
	[self Reset];
}

-(void)Reset
{
	//	Set default content in all status and control registers.
	
	m_nIER = 0;
	m_nIID = 0x001;
	m_nLCR = 0;
	m_nMCR = 0;
	m_nLSR = 0x060;
	
	//	Set all output pins to FALSE (because m_nMCR is zero):
	
	m_bDTR = FALSE;				//	Data Terminal Ready output
	m_bRTS = FALSE;				//	Request to Send output
	m_bOUT1 = FALSE;			//	OUT1 output
	m_bOUT2 = FALSE;			//	OUT2 output
	
	//	De-assert loopback flag (because m_nXXX is zero):
	
	m_bLoopback = FALSE;		//	Loopback mode bit
	
	//	De-assert interrupt (because m_nIER is zero):
	
	m_bINT = FALSE;				//	Interrupt output
	
	//	Set m_bRI FALSE (because the H8-4 board connects it to OUT1 which we have
	//	just set FALSE):
	
	m_bRI = FALSE;
	
	//	Set the Rx Error flags and MSR deltas to FALSE.
	
	m_bPE = FALSE;				//	Parity Error detected
	m_bFE = FALSE;				//	Framing Error detected
	m_bOE = FALSE;				//	Overrun Error detected
	m_bBRK = FALSE;				//	BREAK detected
	m_bDDCD = FALSE;			//	Delta DCD
	m_bDCTS = FALSE;			//	Delta CTS
	m_bTERI = FALSE;			//	Trailing edge of RI
	m_bDDSR = FALSE;			//	Delta DSR
	
	//	Set the Tx flags.
	
	m_bTHRE = TRUE;				//	Transmit holding register is empty
	m_bTSE = TRUE;				//	Transmit Shift Register is empty
	
	//	Set the receiver data ready bit
	
	m_bRBR = FALSE;				//	Received data buffer is empty
	m_bRSR = FALSE;				//	Receiver shift register is empty
	
	//	Set MSR bits from the input pin states:
	
	m_nMSR = (m_bDCD ? 0x080 : 0) |
	(m_bRI ? 0x040 : 0) |
	(m_bDSR ? 0x020 : 0) |
	(m_bCTS ? 0x010 : 0);
}

-(void)Write:(int)nAddress :(char)nData
{
	switch(nAddress & 0x07)
	{
		case 0:
			if (m_nLCR & DLA)
				
			{
				
				//	Note: we can program the DLL, but since we aren't using serialization, the
				//	simulation always seems to be at the right baudrate.  (We do use a timer
				//	elsewhere in the simulation to achieve a data rate of about 1000 characters
				//	per second on receive -- effectively programming the UART for 9600 baud.)
				
				m_nDLL = nData & 0x0FF;
			}
			else
			{
				
				//	Write serial output register.  If serial shift register is empty, transfer
				//	character to shift register.  Else, just set a flag so the transfer can
				//	take place when the shift register is read.  Reset TRE if set.
				//	Side effect -- clear Tx Empty status if tranfer to shift register happens.
				//	Exception -- if we're in "send break" mode, the shift register can't be
				//	loaded so simply set m_bTHRE to FALSE.
				
				m_cTxData = nData;
				if (!(m_nLCR & Send_Break))
				{
					if (m_bTSE || m_bLoopback)
					{
						m_cTxD = m_cTxData;
						if (m_bLoopback)
						{
							
							//	In loopback mode, also copy the data to the receiver shift register (and
							//	"cascade" it to the RBR if needed) and set the receiver handshaking signals.
							//	Toggle the "data seen" flags for the transmitter, too.
							
							m_cRxD = m_cTxD;
							if (m_bRBR && m_bRSR)
							{
								m_bOE = TRUE;
							}
							else if (!m_bRBR)
							{
								m_cRxData = m_cTxD;
								m_bRSR = FALSE;
							}
							else
							{
								m_bRSR = TRUE;
							}
							m_bRBR = TRUE;
							m_bTSE = TRUE;
							m_bTHRE = TRUE;
						}
						else
						{
							m_bTHRE = TRUE;
							m_bTSE = FALSE;
						}
					}
					else
					{
						m_bTHRE = FALSE;
					}
				}
				else
				{
					m_bTHRE = FALSE;
				}
			}
			break;
		case 1:
			if (m_nLCR & DLA)
			{
				
				//	See note about setting m_nDLL, above.
				
				m_nDLH = nData & 0x0FF;
			}
			else
			{
				m_nIER = nData & 0x00F;
			}
			break;
		case 2:
			
			//	This register is read-only.  Ignore any attempt to write it.
			
			break;
		case 3:
			m_nLCR = nData & 0x0FF;
			if (m_bLoopback)
			{
				
				//	If Send Break is set, set the break detection flag and update the LSR.
				
				m_bBRK = nData & Send_Break;
				m_nLSR = ((m_bTSE ? 0x040 : 0) |
						  (m_bTHRE ? 0x020 : 0) |
						  (m_bBRK ? 0x010 : 0) |
						  (m_bFE ? 0x008 : 0) |
						  (m_bPE ? 0x004 : 0) |
						  (m_bOE ? 0x002 : 0) |
						  (m_bRBR ? 0x001 : 0));
			}
			break;
		case 4:
			m_nMCR = nData & 0x01F;
			m_bLoopback = nData & MCR_Loop;
			if (m_bLoopback)
			{
				
				//	Calculate MSR deltas from MSR bits and nData bits and then set new
				//	MSR Deltas and MSR bits.
				
				m_bDCTS = (m_bCTS != (m_nMCR & MCR_RTS));
				m_bDDSR = (m_bDSR != (m_nMCR & MCR_DTR));
				m_bTERI = (m_bRI & ((m_nMCR & MCR_OUT1) == 0));
				m_bDDCD = (m_bDCD != (m_nMCR & MCR_OUT2));
				m_bCTS = (m_nMCR & MCR_RTS);
				m_bDSR = (m_nMCR & MCR_DTR);
				m_bRI = (m_nMCR & MCR_OUT1);
				m_bDCD = (m_nMCR & MCR_OUT2);
				m_nMSR = (m_bDCD ? 0x080 : 0) |
				(m_bRI ? 0x040 : 0) |
				(m_bDSR ? 0x020 : 0) |
				(m_bCTS ? 0x010 : 0) |
				(m_bDDCD ? 0x008 : 0) |
				(m_bTERI ? 0x004 : 0) |
				(m_bDDSR ? 0x002 : 0) |
				(m_bDCTS ? 0x001 : 0);
			}
			
			//	Set the modem control output signals from the MCR bits that control them.
			
			m_bDTR = (m_nMCR & MCR_DTR);
			m_bRTS = (m_nMCR & MCR_RTS);
			m_bOUT1 = (m_nMCR & MCR_OUT1);
			m_bOUT2 = (m_nMCR & MCR_OUT2);
			break;
		case 5:
			
			//	Not sure that writing the LSR will reset existing error conditions when
			//	writing zero bits, force error conditions when writing a one bit or both.
			
			m_nLSR = nData & 0x07F;
			
			break;
		case 6:
			
			//	Not sure that writing the MSR will reset existing status values when
			//	writing zero bits, force status values when writing a one bit or both.
			
			m_nMSR = nData & 0x0FF;
			break;
		case 7:
			
			//	This address is undefined, ignore writes.
			
			break;
	}
	
	//	Almost any write operation could cause an interrupt, so always test for one.
	
	m_bINT = [self InterruptState];
}

-(char)Read:(int)nAddress
{
	int nData;
	
	switch(nAddress & 0x07)
	{
		case 0:
			if (m_nLCR & DLA)
			{
				nData = m_nDLL;
			}
			else
			{
				
				//	Read serial input register.  Consume a character, if one is present.
				//	Side effect: clear Rx Data interrupt if Rx Shift register is then empty.
				
				nData = 0;
				if (!m_bRBR && !m_bRSR)
				{
					
					//	Just give him the old data.
					
					nData = m_cRxData;
					m_bRSR = FALSE;
					m_bRBR = FALSE;
				}
				else if (m_bRBR && !m_bRSR)
				{
					
					//	Give him the new data and mark both registers as empty now.
					
					nData = m_cRxData;
					m_bRSR = FALSE;
					m_bRBR = FALSE;
				}
				else if (!m_bRBR && m_bRSR)
				{
					
					//	Should have already been caught, but...
					//	Transfer the RSR to the RBR, reset m_bRSR and give him the data.  m_bRBR
					//	remains FALSE because we just gave him the data.
					
					m_cRxData = m_cRxD;
					nData = m_cRxData;
					m_bRSR = FALSE;
					m_bRBR = FALSE;
				}				
				else /* (m_bRBR && m_bRSR)	*/
				{
					
					//	Give him the RBR and then transfer the character in the RSR to the RBR.
					//	m_bRBR remains set.  De-assert m_bRSR.
					
					nData = m_cRxData;
					m_cRxData = m_cRxD;
					m_bRSR = FALSE;
					m_bRBR = TRUE;
				}
			}
			break;
		case 1:
			if (m_nLCR & DLA)
			{
				nData = m_nDLH;
			}
			else
			{
				nData = m_nIER;
			}
			break;
		case 2:
			
			//	Before returning IID, be sure it's up to date.
			
			m_bINT = [self InterruptState];
			m_nIID = [self InterruptVector];
			nData = m_nIID;
			break;
		case 3:
			nData = m_nLCR;
			
			//	Note: we can set and read all bits, but most are not useful because we
			//	aren't actually sending data serially, so we always seem to be using "8N1"
			//	even when we try to program something else.
			
			break;
		case 4:
			nData = m_nMCR;
			break;
		case 5:
			
			//	Ensure m_nLSR is up to date.
			
			m_nLSR = ((m_bTSE ? 0x040 : 0) |
					  (m_bTHRE ? 0x020 : 0) |
					  (m_bBRK ? 0x010 : 0) |
					  (m_bFE ? 0x008 : 0) |
					  (m_bPE ? 0x004 : 0) |
					  (m_bOE ? 0x002 : 0) |
					  (m_bRBR ? 0x001 : 0));
			nData = m_nLSR;
			
			//	Side effect -- clear Rx line error status bits.
			
			m_nLSR &= 0x61;
			m_bBRK = FALSE;
			m_bFE = FALSE;
			m_bPE = FALSE;
			m_bOE = FALSE;
			break;
		case 6:
			
			//	Ensure the MSR is up to date.
			
			m_nMSR = (m_bDCD ? 0x080 : 0) |
			(m_bRI ? 0x040 : 0) |
			(m_bDSR ? 0x020 : 0) |
			(m_bCTS ? 0x010 : 0) |
			(m_bDDCD ? 0x008 : 0) |
			(m_bTERI ? 0x004 : 0) |
			(m_bDDSR ? 0x002 : 0) |
			(m_bDCTS ? 0x001 : 0);
			nData = m_nMSR;
			
			//	Side effect -- Modem status interrupt is reset and delta bits are cleared.
			
			m_nMSR &= 0x0F0;
			m_bDDCD = FALSE;
			m_bTERI = FALSE;
			m_bDDSR = FALSE;
			m_bDCTS = FALSE;
			break;
		case 7:
			
			//	This address is undefined, the actual H8 hardware will return 377Q.
			
			nData = 0x0FF;
			break;
	}
	
	//	Almost any read could clear an interrupt, so re-test interrupt state.
	
	m_bINT = [self InterruptState];

	return (char)nData;
}

-(BOOL)InterruptState
{
	
	//	The interrrupt state is determined by enable and status bits:
	//	Enable bits are from IER bits 0-3:
	//	  0		Rx Data Ready				A character is received
	//	  1		Tx holding reg. empty		There is room for another char to send
	//	  2		Rx status					PE, FE, overrun and/or BREAK
	//	  3		Modem status				Change in state of CTS, DSR, RI or CD
	//
	//	Significant status bits are:
	//	IER0: m_bRBR
	//	IER1: m_bTHRE
	//	IER2: (m_bPE || m_bFE || m_bOE || m_bBRK)
	//	IER3: (m_bDDCD || m_bDCTS || m_bTERI || m_bDDSR)
	
	return (((m_nIER & IE_Rx_Ready) && m_bRBR) ||
			((m_nIER & IE_Tx_Empty) && m_bTHRE) ||
			((m_nIER & IE_Rx_Status) && (m_bPE || m_bFE || m_bOE || m_bBRK)) ||
			((m_nIER & IE_Modem_Delta) &&
			 (m_bDDCD || m_bDCTS || m_bTERI || m_bDDSR)));
}

-(int)InterruptVector
{
	if (![self InterruptState]) return 0x001;
	else if ((m_nIER & IE_Rx_Ready) && m_bRBR) return 0x006;
	else if ((m_nIER & IE_Tx_Empty) && m_bTHRE) return 0x004;
	else if ((m_nIER & IE_Rx_Status) && (m_bPE || m_bFE || m_bOE || m_bBRK))
		return 0x002;
	else return 0x000;
}

-(BOOL)InterruptSignal
{
	m_bINT = [self InterruptState];

	return m_bINT;
}

-(char)GetData
{
	char cNewTxD = 0;
	if (!m_bLoopback && !m_bTSE)
	{
		cNewTxD = m_cTxD;
		m_bTSE = TRUE;
	}
	m_bINT = [self InterruptState];
	
	return cNewTxD;
}

-(void)PutData:(char)cRxD
{
	//	Update cRxD and LSR status
	
	if (cRxD != 0)
	{
		
		//	There was a character worth reading.  (We ate any NUL characters.)
		
		if (m_bRBR && m_bRSR)
		{
			
			//	Overrun error -- no place to put the new character.  The previous contents
			//	of the shift register will be lost.
			
			m_cRxD = cRxD;
		}
		else if (m_bRSR && !m_bRBR)
		{
			
			//	Can transfer the RSR to the RBR and then re-load the RSR.  But the Receiver
			//	section of the UART is now "full up" and an overrun will result on the next
			//	character if the 
			
			m_cRxData = m_cRxD;
			m_cRxD = cRxD;
			m_bRSR = TRUE;
			m_bRBR = TRUE;
		}
		else if (!m_bRSR && m_bRBR)
		{
			
			//	RBR is full but we can put the character in the RSR anyway.
			
			m_cRxD = cRxD;
			m_bRSR = TRUE;
		}
		else
		{
			
			//	Rx shift register and Rx data Reg were both empty; pass character through to
			//	Rx data reg and set m_bRBR.  m_bRSR remains FALSE, indicating that the next
			//	character will not cause an overrun condition.
			
			m_cRxData = cRxD;
			m_bRSR = FALSE;
			m_bRBR = TRUE;
		}
	}

	m_bINT = [self InterruptState];
}

@end
