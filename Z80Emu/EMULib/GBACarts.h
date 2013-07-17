/** GameBoy Advance Cartridge Info ***************************/
/**                                                         **/
/**                       GBACarts.h                        **/
/**                                                         **/
/** This file contains functions to extract information     **/
/** from GameBoy Advance cartridge images. Also see         **/
/** GBACarts.c.                                             **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2000-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef GBACARTS_H
#define GBACARTS_H
#ifdef __cplusplus
extern "C" {
#endif

#define GBA_QHVALUE(H,A) \
  (((int)H[A]<<24)|((int)H[A+1]<<16)|((int)H[A+2]<<8)|H[A+3])
#define GBA_QLVALUE(H,A) \
  (((int)H[A+3]<<24)|((int)H[A+2]<<16)|((int)H[A+1]<<8)|H[A])
#define GBA_WHVALUE(H,A) \
  (((int)H[A]<<8)|H[A+1])
#define GBA_WLVALUE(H,A) \
  (((int)H[A+1]<<8)|H[A])

#define GBA_Start(Header) \
  (((GBA_QLVALUE(Header,0)&0x00FFFFFF)<<2)+0x08000008)

#define GBA_GameID(Header)    GBA_QHVALUE(Header,0xAC)
#define GBA_CountryID(Header) Header[0xAF]
#define GBA_MakerID(Header)   GBA_WHVALUE(Header,0xB0)
#define GBA_Valid(Header)     (Header[0xB2]==0x96)
#define GBA_UnitID(Header)    Header[0xB3]
#define GBA_DevType(Header)   Header[0xB4]
#define GBA_Version(Header)   Header[0xBC]
#define GBA_CMP(Header)       Header[0xBD]
#define GBA_CRC(Header)       GBA_WHVALUE(Header,0xBE)
#define GBA_Japanese(Header)  ((GBA_GameID(Header)&0xFF)=='J')
#define GBA_European(Header)  ((GBA_GameID(Header)&0xFF)=='P')
#define GBA_American(Header)  ((GBA_GameID(Header)&0xFF)=='E')
#define GBA_German(Header)    ((GBA_GameID(Header)&0xFF)=='D')

/** GBA_Title() **********************************************/
/** Extract and truncate cartridge title. Returns a pointer **/
/** to the internal buffer!                                 **/
/*************************************************************/
char *GBA_Title(const unsigned char *Header);

/** GBA_Country() ********************************************/
/** Return the name of a country for which the game has     **/
/** been made. Returns 0 if CountryID is not known.         **/
/*************************************************************/
char *GBA_Country(const unsigned char *Header);

/** GBA_Maker() **********************************************/
/** Return the name of a company that has made the game.    **/
/** Returns 0 if MakerID is not known.                      **/
/*************************************************************/
char *GBA_Maker(const unsigned char *Header);

/** GBA_RealCRC() ********************************************/
/** Calculate CRC of a cartridge.                           **/
/*************************************************************/
unsigned short GBA_RealCRC(const unsigned char *Data,int Length);

/** GBA_RealCMP() ********************************************/
/** Calculate CMP of a cartridge.                           **/
/*************************************************************/
unsigned char GBA_RealCMP(const unsigned char *Header);

#ifdef __cplusplus
}
#endif
#endif /* GBACARTS_H */

