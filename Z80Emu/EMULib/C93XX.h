/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          C93XX.h                        **/
/**                                                         **/
/** This file contains emulation for the C93xx series of    **/
/** serial EEPROMs. See C93XX.c for the actual code.        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2000-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef C93XX_H
#define C93XX_H
#ifdef __cplusplus
extern "C" {
#endif

#define C93XX_CHIP  0x03
#define C93XX_16BIT 0x04
#define C93XX_8BIT  0x00
#define C93XX_DEBUG 0x08

#define C93XX_93C46 0x00
#define C93XX_93C56 0x02
#define C93XX_93C57 0x01
#define C93XX_93C66 0x02
#define C93XX_93C86 0x03

#define C93XX_DATA  0x01
#define C93XX_CLOCK 0x02
#define C93XX_CS    0x04

#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef unsigned char byte;
#endif

#ifndef WORD_TYPE_DEFINED
#define WORD_TYPE_DEFINED
typedef unsigned char word;
#endif

typedef struct
{
  unsigned int In,Out; /* Input and output shift registers */

  byte Last;    /* Last value written with Write9346() */
  byte Count;   /* Counter of bits left to shift */
  byte Writing; /* 1 = writing cycle, 0 = getting command */
  byte Reading; /* 1 = reading cycle, 0 = getting command */
  byte EnWrite; /* 1 = write enabled, 0 = write protected */
  word Addr;    /* WRITE operation access address */
  byte AddrLen; /* Address length (bits) for this chip */
  byte DataLen; /* Data length (bits) for this chip */
  byte Debug;   /* 1 = print debugging information, 0 = be quiet */ 
  byte *Data;   /* Memory */
} C93XX;

/** Reset93XX ************************************************/
/** Reset the 93xx chip.                                    **/
/*************************************************************/
void Reset93XX(C93XX *D,byte *Data,int Flags);

/** Write93XX ************************************************/
/** Write value V into the 93xx chip. Only bits 0,1,2 are   **/
/** used (see #defines).                                    **/
/*************************************************************/
void Write93XX(C93XX *D,byte V);

/** Read93XX *************************************************/
/** Read value from the 93xx chip. Only bits 0,1 are used   **/
/** (see #defines).                                         **/
/*************************************************************/
byte Read93XX(C93XX *D);

#ifdef __cplusplus
}
#endif
#endif /* C93XX_H */
