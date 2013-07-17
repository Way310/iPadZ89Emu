/** Nintendo Dual Screen Cartridge Info **********************/
/**                                                         **/
/**                        DSCarts.h                        **/
/**                                                         **/
/** This file contains functions to extract information     **/
/** from Nintendo Dual Screen cartridge images. Also see    **/
/** DSCarts.c.                                              **/
/**                                                         **/
/** This file uses information from Rafael Vuijk.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2000-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef DSCARTS_H
#define DSCARTS_H
#ifdef __cplusplus
extern "C" {
#endif

#define DS_QHVALUE(H,A) \
  (((int)H[A]<<24)|((int)H[A+1]<<16)|((int)H[A+2]<<8)|H[A+3])
#define DS_QLVALUE(H,A) \
  (((int)H[A+3]<<24)|((int)H[A+2]<<16)|((int)H[A+1]<<8)|H[A])
#define DS_WHVALUE(H,A) \
  (((int)H[A]<<8)|H[A+1])
#define DS_WLVALUE(H,A) \
  (((int)H[A+1]<<8)|H[A])

typedef struct
{
  unsigned int Offset;
  unsigned short FileID;
  unsigned short ParentID;
} DS_NameTable;

#define DS_GameID(Header)     DS_QHVALUE(Header,0x0C)
#define DS_CountryID(Header)  Header[0x0F]
#define DS_MakerID(Header)    DS_WHVALUE(Header,0x10)
#define DS_UnitID(Header)     Header[0x12]
#define DS_DevType(Header)    Header[0x13]
#define DS_DevSize(Header)    Header[0x14]
#define DS_Version(Header)    Header[0x1E]
#define DS_ARM9Code(Header)   ((Header)+DS_QLVALUE(Header,0x20))
#define DS_ARM9Start(Header)  DS_QLVALUE(Header,0x24)
#define DS_ARM9Addr(Header)   DS_QLVALUE(Header,0x28)
#define DS_ARM9Size(Header)   DS_QLVALUE(Header,0x2C)
#define DS_ARM7Code(Header)   ((Header)+DS_QLVALUE(Header,0x30))
#define DS_ARM7Start(Header)  DS_QLVALUE(Header,0x34)
#define DS_ARM7Addr(Header)   DS_QLVALUE(Header,0x38)
#define DS_ARM7Size(Header)   DS_QLVALUE(Header,0x3C)
#define DS_FNT(Header)        ((Header)+DS_QLVALUE(Header,0x40))
#define DS_FNTSize(Header)    DS_QLVALUE(Header,0x44)
#define DS_FAT(Header)        ((Header)+DS_QLVALUE(Header,0x48))
#define DS_FATSize(Header)    DS_QLVALUE(Header,0x4C)
#define DS_ARM9OvData(Header) ((Header)+DS_QLVALUE(Header,0x50))
#define DS_ARM9OvSize(Header) DS_QLVALUE(Header,0x54)
#define DS_ARM7OvData(Header) ((Header)+DS_QLVALUE(Header,0x58))
#define DS_ARM7OvSize(Header) DS_QLVALUE(Header,0x5C)
#define DS_ROMInfo1(Header)   ((Header)+0x60)
#define DS_Icon(Header)       ((Header)+DS_QLVALUE(Header,0x68))
#define DS_SecureCRC(Header)  DS_WLVALUE(Header,0x6C)
#define DS_ROMInfo2(Header)   DS_WLVALUE(Header,0x6E)
#define DS_ROMSize(Header)    DS_QLVALUE(Header,0x80)
#define DS_HeaderSize(Header) DS_QLVALUE(Header,0x84)
#define DS_Logo(Header)       ((Header)+0xC0)
#define DS_LogoCRC(Header)    DS_WLVALUE(Header,0xC0+156)
#define DS_HeaderCRC(Header)  DS_WLVALUE(Header,0xC0+158)

#define DS_RootID             ((unsigned short)0xF000)
#define DS_IsFileID(ID)       ((ID)<DE_RootID)
#define DS_IsRootID(ID)       ((ID)==DE_RootID)
#define DS_IsDirID(ID)        ((ID)>=DE_RootID)

/** Name Table and FAT operations ****************************/
#define FNT_Count(Header)     \
  DS_WLVALUE(Header,DS_QLVALUE(Header,0x40)+6)
#define FNT_FID(Header,N)     \
  DS_WLVALUE(Header,DS_QLVALUE(Header,0x40)+8*(N)+4)
#define FNT_PID(Header,N)     \
  DS_WLVALUE(Header,DS_QLVALUE(Header,0x40)+8*(N)+6)
#define FNT_Entries(Header,N) \
  (DS_FNT(Header)+DS_QLVALUE(Header,DS_QLVALUE(Header,0x40)+8*(N)))
#define FAT_File(Header,ID)   \
  ((Header)+DS_QLVALUE(Header,DS_QLVALUE(Header,0x48)+8*(ID)))
#define FAT_FSize(Header,ID)  \
  (DS_QLVALUE(Header,DS_QLVALUE(Header,0x48)+8*(ID)+4) \
  -DS_QLVALUE(Header,DS_QLVALUE(Header,0x48)+8*(ID))+1)

/** Directory entry operations *******************************/
#define DE_IsFile(E)          !(*(E)&0x80)
#define DE_IsDir(E)           !!(*(E)&0x80)
#define DE_NameLen(E)         (*(E)&0x7F)
#define DE_Name(E)            ((E)+1)
#define DE_EOD(E)             !DE_NameLen(E)
#define DE_SubDirID(E)        \
  (unsigned short)(*((E)+DE_NameLen(E)+1)+*((E)+DE_NameLen(E)+2)*256)
#define DE_Next(E)            \
  (DE_EOD(E)? 0:((E)+DE_NameLen(E)+(DE_IsDir(E)? 3:1)))

/** DS_Title() ***********************************************/
/** Extract and truncate cartridge title. Returns a pointer **/
/** to the internal buffer!                                 **/
/*************************************************************/
char *DS_Title(const unsigned char *Header);

/** DS_Country() *********************************************/
/** Return the name of a country for which the game has     **/
/** been made. Returns 0 if CountryID is not known.         **/
/*************************************************************/
char *DS_Country(const unsigned char *Header);

/** DS_Maker() ***********************************************/
/** Return the name of a company that has made the game.    **/
/** Returns 0 if MakerID is not known.                      **/
/*************************************************************/
char *DS_Maker(const unsigned char *Header);

/** DS_RealHeaderCRC() ***************************************/
/** Calculate CRC of the header.                            **/
/*************************************************************/
unsigned short DS_RealHeaderCRC(const unsigned char *Header);

/** DS_RealLogoCRC() *****************************************/
/** Calculate CRC of the logo.                              **/
/*************************************************************/
unsigned short DS_RealLogoCRC(const unsigned char *Header);

/** DS_RealSecureCRC() ***************************************/
/** Calculate CRC of the data.                              **/
/*************************************************************/
unsigned short DS_RealSecureCRC(const unsigned char *Header);

#ifdef __cplusplus
}
#endif
#endif /* DSCARTS_H */
