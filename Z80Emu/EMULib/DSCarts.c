/** Nintendo Dual Screen Cartridge Info **********************/
/**                                                         **/
/**                        DSCarts.c                        **/
/**                                                         **/
/** This file contains functions to extract information     **/
/** from Nintendo Dual Screen cartridge images. Also see    **/
/** DSCarts.h.                                              **/
/**                                                         **/
/** This file uses information from Rafael Vuijk.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2000-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "DSCarts.h"
#include <string.h>

/** Countries ************************************************/
/** This is a list of known countries for which cartridges  **/
/** are produced.                                           **/
/*************************************************************/
static const struct { unsigned char ID; char *Name; } Countries[] =
{
  { 'J',"JPN" },
  { 'E',"USA" },
  { 'P',"EUR" },
  { 'D',"NOE" },
  { 'F',"NOE" },
  { 'I',"ITA" },
  { 'S',"SPA" },
  { 'H',"HOL" },
  { 'K',"KOR" },
  { 'X',"EUU" },
  { '\0',0 }
};

/** Companies ************************************************/
/** This is a list of known producers and their MakerIDs.   **/
/** It is used by the DS_Maker() function.                  **/
/*************************************************************/
static const struct { int ID; char *Name; } Companies[] =
{
  { 0x3031,"Nintendo (1)" },
  { 0x3038,"Capcom" },
  { 0x3041,"Jaleco" },
  { 0x3041,"Coconuts" },
  { 0x3048,"Star Fish" },
  { 0x3138,"Hudson Soft" },
  { 0x3230,"Destination Software" },
  { 0x3238,"Kemco (Japan)" },
  { 0x324C,"TAM" },
  { 0x324D,"Gu / Gajin" },
  { 0x3251,"MediaKite" },
  { 0x3333,"Nintendo (2)" },
  { 0x3431,"UBI Soft" },
  { 0x3436,"70" },
  { 0x3437,"Spectrum Holobyte" },
  { 0x345A,"Crave Entertainment" },
  { 0x3530,"Absolute Entertainment" },
  { 0x3531,"Acclaim" },
  { 0x3532,"Activision" },
  { 0x3532,"American Sammy" },
  { 0x3534,"Take-Two Interactive" },         /* @@@ Gametek? */
  { 0x3535,"Park Place" },
  { 0x3536,"LJN" },
  { 0x3541,"Bitmap Brothers / Mindscape" },
  { 0x3544,"Midway" },                       /* @@@ Tradewest? */
  { 0x3547,"Majesco Sales" },
  { 0x355A,"Conspiracy Entertainment" },
  { 0x3548,"3DO" },
  { 0x354C,"NewKid Co" },
  { 0x354D,"Telegames" },
  { 0x3551,"LEGO Software" },
  { 0x3630,"Titus" },
  { 0x3631,"Virgin" },
  { 0x3637,"Ocean" },
  { 0x3639,"Electronic Arts" },
  { 0x3646,"ElectroBrain" },
  { 0x3648,"BBC (?)" },
  { 0x364C,"BAM!" },
  { 0x364D,"Studio 3" },
  { 0x3653,"TDK Mediactive" },
  { 0x3655,"DreamCatcher" },
  { 0x3657,"SEGA (US)" },
  { 0x3659,"Light And Shadow" },
  { 0x3730,"Infogrames" },
  { 0x3732,"Broderbund (1)" },
  { 0x3735,"Carlton International Media (?)" },
  { 0x3738,"THQ" },
  { 0x3739,"Accolade" },
  { 0x3741,"Triffix Entertainment" },
  { 0x3744,"Universal Interactive Studios" },
  { 0x3746,"Kemco (US)" },
  { 0x3747,"Denki" },
  { 0x374D,"Asmik (3)" },
  { 0x3833,"Lozc" },
  { 0x3843,"Bullet-Proof Software" },
  { 0x3843,"Vic Tokai" },
  { 0x3850,"SEGA (Japan)" },
  { 0x3933,"Tsuburava" },
  { 0x3939,"Victor Interactive Studios" },   /* @@@ ARC? */
  { 0x3942,"Tecmo" },
  { 0x3943,"Imagineer" },
  { 0x394E,"Marvelous Entertainment" },
  { 0x3950,"KeyNet" },
  { 0x4134,"Konami (1)" },
  { 0x4136,"Kawada" },
  { 0x4137,"Takara" },
  { 0x4139,"Technos Japan" },
  { 0x4141,"Broderbund (2)" },
  { 0x4142,"Namco (1)" },
  { 0x4146,"Namco (2)" },
  { 0x4147,"Media Rings" },
  { 0x4231,"ASCII / Nexoft" },
  { 0x4234,"Enix" },
  { 0x4236,"HAL" },
  { 0x4242,"Sunsoft" },
  { 0x4244,"Imagesoft" },
  { 0x4246,"Sammy" },
  { 0x424C,"MTO" },
  { 0x4250,"Global A" },
  { 0x4330,"Taito" },
  { 0x4332,"Kemco (?)" },
  { 0x4333,"SquareSoft" },
  { 0x4335,"Data East" },
  { 0x4336,"Tonkin House" },
  { 0x4338,"KOEI" },
  { 0x4341,"Palcom / Ultra" },
  { 0x4342,"VAP" },
  { 0x4345,"FCI / Pony Canyon" },
  { 0x4431,"Sofel" },
  { 0x4432,"Quest" },
  { 0x4439,"Banpresto" },
  { 0x4441,"Tomy" },
  { 0x4444,"NCS" },
  { 0x4446,"Altron" },
  { 0x4531,"Towachiki" },
  { 0x4535,"Epoch" },
  { 0x4537,"Athena" },
  { 0x4538,"Asmik (1)" },
  { 0x4541,"King Records" },
  { 0x4542,"Atlus" },
  { 0x4545,"IGS" },
  { 0x454C,"Spike" },
  { 0x454D,"Konami (2)" },
  { 0x4558,"Asmik (2)" },
  { 0,0 }
};

/** CRCTable *************************************************/
/** This data is used to compute logo and header CRCs.      **/
/*************************************************************/
static const unsigned short CRCTable[256] =
{
  0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,
  0xC601,0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,
  0xCC01,0x0CC0,0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,
  0x0A00,0xCAC1,0xCB81,0x0B40,0xC901,0x09C0,0x0880,0xC841,
  0xD801,0x18C0,0x1980,0xD941,0x1B00,0xDBC1,0xDA81,0x1A40,
  0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,0x1DC0,0x1C80,0xDC41,
  0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,0x1680,0xD641,
  0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,0x1040,
  0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
  0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,
  0x3C00,0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,
  0xFA01,0x3AC0,0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,
  0x2800,0xE8C1,0xE981,0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,
  0xEE01,0x2EC0,0x2F80,0xEF41,0x2D00,0xEDC1,0xEC81,0x2C40,
  0xE401,0x24C0,0x2580,0xE541,0x2700,0xE7C1,0xE681,0x2640,
  0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,0x2080,0xE041,
  0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,0x6240,
  0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
  0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,
  0xAA01,0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,
  0x7800,0xB8C1,0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,
  0xBE01,0x7EC0,0x7F80,0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,
  0xB401,0x74C0,0x7580,0xB541,0x7700,0xB7C1,0xB681,0x7640,
  0x7200,0xB2C1,0xB381,0x7340,0xB101,0x71C0,0x7080,0xB041,
  0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,0x5280,0x9241,
  0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,0x5440,
  0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
  0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,
  0x8801,0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,
  0x4E00,0x8EC1,0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,
  0x4400,0x84C1,0x8581,0x4540,0x8701,0x47C0,0x4680,0x8641,
  0x8201,0x42C0,0x4380,0x8341,0x4100,0x81C1,0x8081,0x4040
};

/** DS_Title() ***********************************************/
/** Extract and truncate cartridge title. Returns a pointer **/
/** to the internal buffer!                                 **/
/*************************************************************/
char *DS_Title(const unsigned char *Header)
{
  static char Buf[32];

  strncpy(Buf,Header,12);
  Buf[12]='\0';
  return(Buf);
}

/** DS_RealLogoCRC() *****************************************/
/** Calculate CRC of the logo.                              **/
/*************************************************************/
unsigned short DS_RealLogoCRC(const unsigned char *Header)
{
  int J,I;

  for(J=0xFFFF,I=0;I<156;I++)
    J = (J>>8)^CRCTable[(J^DS_Logo(Header)[I])&0xFF];

  return(J&0xFFFF);
}

/** DS_RealHeaderCRC() ***************************************/
/** Calculate CMP of a cartridge.                           **/
/*************************************************************/
unsigned short DS_RealHeaderCRC(const unsigned char *Header)
{
  int J,I;

  for(J=0xFFFF,I=0;I<0x15E;I++)
    J = (J>>8)^CRCTable[(J^Header[I])&0xFF];

  return(J&0xFFFF);
}

/** DS_RealSecureCRC() ***************************************/
/** Calculate CRC of the data.                              **/
/*************************************************************/
unsigned short DS_RealSecureCRC(const unsigned char *Header)
{
  int J,I;

  for(J=0xFFFF,I=0x4000;I<0x8000;I++)
    J = (J>>8)^CRCTable[(J^Header[I])&0xFF];

  return(J&0xFFFF);
}

/** DS_Country() *********************************************/
/** Return the name of a country for which the game has     **/
/** been made. Returns 0 if CountryID is not known.         **/
/*************************************************************/
char *DS_Country(const unsigned char *Header)
{
  int J,ID;

  /* Fetch the ID */
  ID=DS_CountryID(Header);
  /* Look up the name */
  for(J=0;Countries[J].Name;J++)
    if(Countries[J].ID==ID) return(Countries[J].Name);
  /* Name not found */
  return(0);
}

/** DS_Maker() ***********************************************/
/** Return the name of a company that has made the game.    **/
/** Returns 0 if MakerID is not known.                      **/
/*************************************************************/
char *DS_Maker(const unsigned char *Header)
{
  int J,ID;

  /* Fetch the ID */
  ID=DS_MakerID(Header);
  /* Look up the name */
  for(J=0;Companies[J].Name;J++)
    if(Companies[J].ID==ID) return(Companies[J].Name);
  /* Name not found */
  return(0);
}


