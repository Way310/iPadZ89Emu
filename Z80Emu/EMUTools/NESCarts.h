/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                       NESCarts.h                        **/
/**                                                         **/
/** This file contains definitions of different fields in   **/
/** the .NES file header. Also see NESCarts.c.              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1998-2001                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef NESCARTS_H
#define NESCARTS_H

#define NES_ROMBanks(Header)  Header[4]
#define NES_VROMBanks(Header) Header[5]
#define NES_ROMSize(Header)   ((int)NES_ROMBanks(Header)<<14)
#define NES_VROMSize(Header)  ((int)NES_VROMBanks(Header)<<13)
#define NES_Mapper(Header)    ((Header[6]>>4)|(Header[7]&0xF0))
#define NES_Mirroring(Header) (Header[6]&0x01)
#define NES_Battery(Header)   (Header[6]&0x02)
#define NES_Trainer(Header)   (Header[6]&0x04)
#define NES_4Screens(Header)  (Header[6]&0x08)
#define NES_VSSystem(Header)  (Header[7]&0x01)

/** NES_CRC() ************************************************/
/** Calculate CRC of a given number of bytes.               **/
/*************************************************************/
unsigned short NES_CRC(unsigned char *Data,int N);

#endif /* NESCARTS_H */