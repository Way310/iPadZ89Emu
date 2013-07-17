/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                       NESCarts.h                        **/
/**                                                         **/
/** This file contains definitions of different fields in   **/
/** the .NES file header. Also see NESCarts.c.              **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1998-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef NESCARTS_H
#define NESCARTS_H
#ifdef __cplusplus
extern "C" {
#endif

#define NESMAP_NONE     0
#define NESMAP_MMC1     1
#define NESMAP_UNROM    2
#define NESMAP_CNROM    3
#define NESMAP_MMC3     4
#define NESMAP_MMC5     5
#define NESMAP_F4XX     6
#define NESMAP_AOROM    7
#define NESMAP_F3XX     8
#define NESMAP_MMC2     9
#define NESMAP_MMC4     10
#define NESMAP_CDREAMS  11
#define NESMAP_CPROM    13
#define NESMAP_100IN1   15
#define NESMAP_BANDAI   16
#define NESMAP_F8XX     17
#define NESMAP_JALECO   18 /* Jaleco SS8806 */
#define NESMAP_NAMCOT   19 /* Namcot 106 */
#define NESMAP_FDS      20 /* Famicom DiskSystem */
#define NESMAP_VRC4A    21
#define NESMAP_VRC2A    22
#define NESMAP_VRC2B    23
#define NESMAP_VRC6     24
#define NESMAP_VRC4B    25
#define NESMAP_G101     32 /* Item G-101 */
#define NESMAP_TAITO    33 /* Taito TC0190/TC0350 */
#define NESMAP_NINA1    34
#define NESMAP_RAMBO1   64 /* Tengen RAMBO-1 */
#define NESMAP_H3001    65 /* Irem H-3001 */
#define NESMAP_GNROM    66
#define NESMAP_SUNSOFT  68 /* SunSoft #4 */
#define NESMAP_FME7     69 /* SunSoft FME-7 */
#define NESMAP_CAMERICA 71
#define NESMAP_74HC161  78 /* Irem 74HC161/32 */
#define NESMAP_NINA3    79
#define NESMAP_NINA6    81
#define NESMAP_HKSF3    91 /* Pirate HK-SF3 */

#define NES_ROMBanks(Header)  ((int)Header[4])
#define NES_VROMBanks(Header) ((int)Header[5])
#define NES_RAMBanks(Header)  ((int)Header[8])
#define NES_ROMSize(Header)   (NES_ROMBanks(Header)<<14)
#define NES_VROMSize(Header)  (NES_VROMBanks(Header)<<13)
#define NES_Mapper(Header)    ((Header[6]>>4)|(Header[7]&0xF0))
#define NES_Mirroring(Header) (Header[6]&0x01)
#define NES_Battery(Header)   (Header[6]&0x02)
#define NES_Trainer(Header)   (Header[6]&0x04)
#define NES_4Screens(Header)  (Header[6]&0x08)
#define NES_VSSystem(Header)  (Header[7]&0x01)
#define NES_PAL(Header)       (Header[9]&0x01)
#define NES_NTSC(Header)      !(Header[9]&0x01)

/** NES_CRC() ************************************************/
/** Calculate CRC of a given number of bytes.               **/
/*************************************************************/
unsigned short NES_CRC(const unsigned char *Data,int N);

#ifdef __cplusplus
}
#endif
#endif /* NESCARTS_H */
