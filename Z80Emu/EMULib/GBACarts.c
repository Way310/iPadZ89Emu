/** GameBoy Advance Cartridge Info ***************************/
/**                                                         **/
/**                       GBACarts.c                        **/
/**                                                         **/
/** This file contains functions to extract information     **/
/** from GameBoy Advance cartridge images. Also see         **/
/** GBACarts.h.                                             **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2000-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "GBACarts.h"
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
/** It is used by the GBA_Maker() function.                 **/
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

/** GBA_Title() **********************************************/
/** Extract and truncate cartridge title. Returns a pointer **/
/** to the internal buffer!                                 **/
/*************************************************************/
char *GBA_Title(const unsigned char *Header)
{
  static char Buf[32];

  strncpy(Buf,Header+0xA0,12);
  Buf[12]='\0';
  return(Buf);
}

/** GBA_RealCRC() ********************************************/
/** Calculate CRC of a cartridge.                           **/
/*************************************************************/
unsigned short GBA_RealCRC(const unsigned char *Data,int Length)
{
  unsigned short CRC;
  int J;

  for(J=0,CRC=0x0000;J<Length;J++) CRC+=Data[J];
  return(CRC);
}

/** GBA_RealCMP() ********************************************/
/** Calculate CMP of a cartridge.                           **/
/*************************************************************/
unsigned char GBA_RealCMP(const unsigned char *Header)
{
  unsigned char CMP;
  int J;

  for(J=0xA0,CMP=0xE7;J<=0xBC;J++) CMP-=Header[J];
  return(CMP);
}

/** GBA_Country() ********************************************/
/** Return the name of a country for which the game has     **/
/** been made. Returns 0 if CountryID is not known.         **/
/*************************************************************/
char *GBA_Country(const unsigned char *Header)
{
  int J,ID;

  /* Fetch the ID */
  ID=GBA_CountryID(Header);
  /* Look up the name */
  for(J=0;Countries[J].Name;J++)
    if(Countries[J].ID==ID) return(Countries[J].Name);
  /* Name not found */
  return(0);
}

/** GBA_Maker() **********************************************/
/** Return the name of a company that has made the game.    **/
/** Returns 0 if MakerID is not known.                      **/
/*************************************************************/
char *GBA_Maker(const unsigned char *Header)
{
  int J,ID;

  /* Fetch the ID */
  ID=GBA_MakerID(Header);
  /* Look up the name */
  for(J=0;Companies[J].Name;J++)
    if(Companies[J].ID==ID) return(Companies[J].Name);
  /* Name not found */
  return(0);
}


