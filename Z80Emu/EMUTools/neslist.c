/** NESLIST: NES Cartridge Lister/Tester *********************/
/**                                                         **/
/**                        neslist.c                        **/
/**                                                         **/
/** This program will list a group of cartridges finding    **/
/** those which have invalid sizes, headers, etc.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2001                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "NESCarts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef ZLIB
#include <zlib.h>
#endif

typedef unsigned char byte;

#ifdef MSDOS
char *ErrorLine = "³ %-28s ³%s %-21s %s³\n";
char *DataLine  = "³ %-28s ³%s³%c%c%c%c%c³%4dkB³%s³";
char *TopLine   = "ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÂÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÄ¿";
char *HeadLine  = "³ File                         ³Map³Type ³ ROM  ³ VROM ³";
char *MidLine   = "ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÅÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÄ´";
char *BotLine   = "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÁÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÄÙ";
#else
char *ErrorLine = "| %-28s |%s %-21s %s|\n";
char *DataLine  = "| %-28s |%s|%c%c%c%c%c|%4dkB|%s|";
char *TopLine   = "+------------------------------+---+-----+------+------+";
char *HeadLine  = "| File                         |Map|Type | ROM  | VROM |";
char *MidLine   = "+------------------------------+---+-----+------+------+";
char *BotLine   = "+------------------------------+---+-----+------+------+";
#endif

int UseANSI,WriteHDR,WritePRM,CheckSize,CheckDouble;
int SuspCount,EmptyCount;

/** Error() **************************************************/
/** Print out the error message about the file.             **/
/*************************************************************/
void Error(char *File,char *Text)
{ printf(ErrorLine,File,UseANSI? "\033[31m":"",Text,UseANSI? "\033[0m":""); }

/** ChangeExt() **********************************************/
/** Change the file extension.                              **/
/*************************************************************/
char *ChangeExt(char *S,char *Ext)
{
  if(strrchr(S,'.')) strcpy(strrchr(S,'.'),Ext);
  else strcat(S,Ext);
  return(S);
}

/** WriteOutPRM() ********************************************/
/** Write out Pasofami files (.PRM,.PRG,.CHR,.700).         **/
/*************************************************************/
char *WriteOutPRM(char *Name,byte *Header,byte *Data)
{
                            /*0123456789ABCDEF*/
  static char ROMType[16]  = "  T N           ";
  static char VROMType[16] = "  T T           ";
  static char GfxType[16]  = "  C N           ";

  char S[512];
  FILE *F;
  int J;

  strncpy(S,Name,511);S[511]='\0';
  if(!(F=fopen(ChangeExt(S,".prm"),"wb")))
    return("[Couldn't write .PRM]");

  fprintf
  (
    F,"%c%c%c%c%c %c%c%c%c%c%c%c%c%c %c%c         %c%02s%c%c\015\n",
    NES_Mirroring(Header)? 'V':'H', /* Mirroring */
    ROMType[NES_Mapper(Header)&15], /* ROM Mapper Type */
    VROMType[NES_Mapper(Header)&15],/* VROM Mapper Type */
    ' ',                            /* Music Mode */
    GfxType[NES_Mapper(Header)&15], /* Something related to graphics */
    ' ',                            /* Validity ? */
    ' ',                            /* IRQ Control ? */
    ' ',                            /* Something related to graphics */
    ' ',                            /* Display Validity ? */
    'S',                            /* Speed (NMI) Control */
    'L',                            /* Default Sprite Size */
    'R',                            /* Default Foreground/Background */
    ' ',                            /* Break Order ? */
    NES_Battery(Header)? 'E':' ',   /* Preserve Extension RAM */
    'S',                            /* Something related to interrupts */
    NES_Mapper(Header)? 'M':' ',    /* Bank-switched ROM ? */
    'X',                            /* Partial Horizontal Scroll ? */
    "02",                           /* Don't scroll up to this scanline ? */
    '2',                            /* Line to do a scroll in ? */
    'A'                             /* Comment ? */
  );
  fprintf(F,"1234567890123456789012345678901234\015\n\032");
  fclose(F);

  if(NES_Trainer(Header))
  {
    if(F=fopen(ChangeExt(S,".700"),"wb"))
    {
      J=(fwrite(Data,1,512,F)==512);
      fclose(F);
      if(!J) return("[Error writing .700]");
    }
    else return("[Couldn't write .700]");
    Data+=0x200;
  }

  J=NES_ROMSize(Header);
  if(F=fopen(ChangeExt(S,".prg"),"wb"))
  {
    J=(fwrite(Data,1,J,F)==J);
    fclose(F);
    if(!J) return("[Error writing .PRG]");
  }
  else return("[Couldn't write .PRG]");

  Data+=NES_ROMSize(Header);
  J=NES_VROMSize(Header);
  if(F=fopen(ChangeExt(S,".chr"),"wb"))
  {
    J=(fwrite(Data,1,J,F)==J);
    fclose(F);
    if(!J) return("[Error writing .CHR]");
  }
  else return("[Couldn't write .CHR]");

  return(NULL);
}

/** WriteOutHDR() ********************************************/
/** Write out a .HDR file.                                  **/
/*************************************************************/
char *WriteOutHDR(char *Name,byte *Header)
{
  char S[512];
  FILE *F;
  int J;

  strncpy(S,Name,511);S[511]='\0';
  if(!(F=fopen(ChangeExt(S,".hdr"),"wb")))
    return("[Couldn't write .HDR]");

  J=(fwrite(Header,1,16,F)==16);
  fclose(F);
  return(J? NULL:"[Error writing .HDR]");
}

/** Doubled() ************************************************/
/** Check if any parts of the .NES file are doubled and     **/
/** write out the .FIX file if so.                          **/
/*************************************************************/
char *Doubled(char *Name,byte *Header,byte *Data)
{
  static char Text[128];
  int ROMSize,VROMSize,Count,J;
  byte NewHeader[16],*P;
  char S[512];
  FILE *F;

  /* Make a copy of the header */
  memcpy(NewHeader,Header,16);

  /* Check ROM for doubling */
  P=NES_Trainer(Header)? Data+0x200:Data;
  for(ROMSize=NES_ROMSize(Header)/2;!(ROMSize%0x4000)&&ROMSize;ROMSize/=2)
  {
    for(J=Count=0;J<ROMSize;J++)
      if(P[J]==P[ROMSize+J]) Count++;
    if(Count<ROMSize) break;
  }
  ROMSize/=0x2000;

  /* Check VROM for doubling */
  P+=NES_ROMSize(Header);
  for(VROMSize=NES_VROMSize(Header)/2;!(VROMSize%0x2000)&&VROMSize;VROMSize/=2)
  {
    for(J=Count=0;J<VROMSize;J++)
      if(P[J]==P[VROMSize+J]) Count++;
    if(Count<VROMSize) break;
  }
  VROMSize/=0x1000;

  /* If doubled, write out .FIX file */
  if((ROMSize<NES_ROMBanks(Header))||(VROMSize<NES_VROMBanks(Header)))
  {
    strncpy(S,Name,511);S[511]='\0';
    if(!(F=fopen(ChangeExt(S,".fix"),"wb")))
      return("[Couldn't write .FIX]");

    NewHeader[4]=ROMSize;NewHeader[5]=VROMSize;
    if(fwrite(NewHeader,1,16,F)!=16)
    { fclose(F);return("[Error writing .FIX]"); }

    if(NES_Trainer(Header))
    {
      if(fwrite(Data,1,512,F)!=512)
      { fclose(F);return("[Error writing .FIX]"); }
      Data+=0x200;
    }

    if(ROMSize)
      if(fwrite(Data,1,ROMSize*0x4000,F)!=ROMSize*0x4000)
      { fclose(F);return("[Error writing .FIX]"); }
    Data+=NES_ROMSize(Header);

    if(VROMSize)
      if(fwrite(Data,1,VROMSize*0x2000,F)!=VROMSize*0x2000)
      { fclose(F);return("[Error writing .FIX]"); }

    sprintf
    (
      Text,"[ROMx%d,VROMx%d]",
      NES_ROMBanks(Header)/ROMSize,
      VROMSize? NES_VROMBanks(Header)/VROMSize:0
    );
    fclose(F);
    return(Text);
  }

  return(NULL);
}

#ifdef ZLIB
#define fopen          gzopen
#define fclose         gzclose
#define fread(B,N,L,F) gzread(F,B,(L)*(N))
#endif

/** main() ***************************************************/
/** Main function.                                          **/
/*************************************************************/
int main(int argc,char *argv[])
{
  byte Header[16],*Data,C;
  char S[256],T[256],Comment[1024],*P,*FileName;
  FILE *F;
  int J,I,K;

  /* If no arguments given, print out help message */
  if(argc<2)
  {
    fprintf(stderr,"NESLIST .NES File Processor v.3.2 by Marat Fayzullin\n");
#ifdef ZLIB
    fprintf(stderr,"  This program will transparently uncompress GZIPped files.\n");
#endif
    fprintf(stderr,"Usage: %s [-ahcpd] files...\n",argv[0]);
    fprintf(stderr,"  -a - Use ANSI escape sequences for colors\n");
    fprintf(stderr,"  -h - Write out separate headers for files\n");
    fprintf(stderr,"  -c - Check file sizes\n");
    fprintf(stderr,"  -p - Generate PASOFAMI files: .PRM,.PRG,.CHR,.700\n");
    fprintf(stderr,"  -d - Check ROM/VROM for doubling and truncate\n");
    return(1);
  }

  /* Set default values */
  UseANSI=WriteHDR=WritePRM=CheckSize=CheckDouble=0;

  /* Parse command line options */
  for(I=1;I<argc;I++)
    if(*argv[I]=='-')
      for(J=1;argv[I][J];J++)
        switch(argv[I][J])
        {
          case 'a': UseANSI=1;break;
          case 'h': WriteHDR=1;break;
          case 'p': WritePRM=1;break;
          case 'c': CheckSize=1;break;
          case 'd': CheckDouble=1;break;
          default:
            fprintf(stderr,"%s: Unknown option -%c\n",argv[0],argv[I][J]);
        }

  /* Print out the list header */
  puts(TopLine);puts(HeadLine);puts(MidLine);

  /* Go through the files */
  for(J=1;J<argc;J++)
    if(*argv[J]!='-')
    {
      I=1;*Comment='\0';

      /* Finding the name of a file */
      for(P=argv[J]+strlen(argv[J]);P>=argv[J];P--)
        if((*P=='\\')||(*P=='/')) break;
      FileName=P+1;

      /* Opening a file */
      if(!(F=fopen(argv[J],"rb")))
      { I=0;Error(FileName,"Couldn't open file"); }

      /* Reading the header */
      if(I)
        if(fread(Header,1,16,F)!=16)
        { I=0;Error(FileName,"Couldn't read header"); }

      /* Checking the header and reading the values */
      if(I)
        if(strncmp(Header,"NES\032",4))
        { I=0;Error(FileName,"Invalid header"); }
        else
          if(!NES_ROMBanks(Header))
          { I=0;Error(FileName,"Zero ROM size"); }

      /* Checking reserved space */
      if(I)
        for(K=8;K<16;K++)
          if(Header[K])
          { strcat(Comment,"[Trash in the header]");break; }

      /* Writing .HDR file (.NES header) */
      if(I&&WriteHDR)
        if(P=WriteOutHDR(argv[J],Header)) strcat(Comment,P);

      /* Loading data if necessary */
      if(I&&(CheckSize||CheckDouble||WritePRM))
      {
        /* Calculating data size */
        K=NES_ROMSize(Header)+NES_VROMSize(Header)+(NES_Trainer(Header)? 0x200:0x000);

        /* Allocating memory for the data */
        if(!(Data=malloc(K)))
          strcat(Comment,"[Couldn't allocate memory]");

        /* Reading the data */
        if(Data)
          if(fread(Data,1,K,F)!=K)
          { free(Data);Data=NULL;strcat(Comment,"[File too short]"); }

        /* Checking for trash */
        if(Data)
          if(fread(&C,1,1,F)) strcat(Comment,"[File too long]");

        /* Checking for doubling */
        if(Data&&CheckDouble)
          if(P=Doubled(argv[J],Header,Data)) strcat(Comment,P);

        /* Writing Pasofami files (.PRM,.PRG,.CHR,.700) */
        if(Data&&WritePRM)
          if(P=WriteOutPRM(argv[J],Header,Data)) strcat(Comment,P);

        if(Data) free(Data);
      }

      /* Closing the file */
      if(F) fclose(F);

      /* Printing out the information line */
      if(I)
      {
        /* Make up strings */
        sprintf(S,"%4dkB",NES_VROMBanks(Header)*8);
        sprintf(T,"%3d",NES_Mapper(Header));

        /* Print out the line */
        printf
        (
          DataLine,
          FileName,NES_Mapper(Header)? T:"   ",
          NES_4Screens(Header)? '4':NES_Mirroring(Header)? 'V':'H',
          NES_Battery(Header)? 'B':'.',
          NES_Trainer(Header)? 'T':'.',
          NES_VSSystem(Header)? 'S':'.',
          '.',
          NES_ROMBanks(Header)*16,
          NES_VROMBanks(Header)? S:"  None"
        );

        /* Print out comment if present */
        if(*Comment)
          printf
          (
            "%s %s%s\n",UseANSI? "\033[35m":"",
            Comment,UseANSI? "\033[0m":""
          );
          else puts("");
      }
    }

  puts(BotLine);
  return(0);
}
