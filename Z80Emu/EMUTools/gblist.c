/** GBLIST: GameBoy Cartridge Lister/Tester ******************/   
/**                                                         **/
/**                         gblist.c                        **/
/**                                                         **/
/** This program will list and optionally test a group of   **/
/** cartridges finding those which fail CRC or complement   **/
/** check, or have invalid size.                            **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-2001                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "GBCarts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef ZLIB
#include <zlib.h>
#endif

typedef unsigned char byte;
typedef unsigned short word;

#ifdef MSDOS
char *ErrorLine = "³%-20s³%s %-54s %s³\n";
char *DataLine  = "³%-20s³ %-16s ³%c%c%c%c%c%c%c%c%c%c³%4dkB³%5s³%02X³%2X³%04X-%02X³";
char *TopLine   = "ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÂÄÄÄÄÄÂÄÄÂÄÄÂÄÄÄÄÄÄÄ¿";
char *HeadLine  = "³ File               ³ Name             ³ Type     ³ ROM  ³ RAM ³MK³VE³CRC-CMP³";
char *MidLine   = "ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÅÄÄÄÄÄÅÄÄÅÄÄÅÄÄÄÄÄÄÄ´";
char *BotLine   = "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÁÄÄÄÄÄÁÄÄÁÄÄÁÄÄÄÄÄÄÄÙ";
#else
char *ErrorLine = "|%-20s|%s %-54s %s|\n";
char *DataLine  = "|%-20s| %-16s |%c%c%c%c%c%c%c%c%c%c|%4dkB|%5s|%02X|%2X|%04X-%02X|";
char *TopLine   = "+--------------------+------------------+----------+------+-----+--+--+-------+";
char *HeadLine  = "| File               | Name             | Type     | ROM  | RAM |MK|VE|CRC-CMP|";
char *MidLine   = "+--------------------+------------------+----------+------+-----+--+--+-------+";
char *BotLine   = "+--------------------+------------------+----------+------+-----+--+--+-------+";
#endif

char *ErrorHTML = "<A HREF=\"%s\">%s %-53s %s</A>\n";
char *DataHTML  = "<A HREF=\"%s\">%-16s</A>  %c%c%c%c%c%c%c%c%c%c %4dkB %5s %02X %2X %04X-%02X  ";
char *TopHTML   = "<PRE>";
char *HeadHTML  = "<HR><B>Name              Type       ROM    RAM  MK VE CRC-CMP</B><HR>";
char *MidHTML   = "";
char *BotHTML   = "<HR>\n</PRE>";

int CheckCRC,UseANSI,UseHTML,FixCRC,ShowProducer;

/** Error() **************************************************/
/** Print out the error message about the file.             **/
/*************************************************************/
void Error(char *File,char *Text)
{
  printf
  (
    UseHTML? ErrorHTML:ErrorLine,
    File,
    UseHTML? "<I>":UseANSI? "\033[31m":"",
    Text,
    UseHTML? "</I>":UseANSI? "\033[0m":""
  );
}

/** ChangeExt() **********************************************/
/** Change the file extension.                              **/
/*************************************************************/
char *ChangeExt(char *S,char *Ext)
{
  if(strrchr(S,'.')) strcpy(strrchr(S,'.'),Ext);
  else strcat(S,Ext);
  return(S);
}

/** WriteOutFIX() ********************************************/
/** Write out a .FIX file.                                  **/
/*************************************************************/
char *WriteOutFIX(char *Name,byte *Data)
{
  FILE *F;
  char S[512];
  word C;
  int L;

  Data[0x14D]=GB_RealCMP(Data);
  C=GB_RealCRC(Data);
  Data[0x14E]=C>>8;
  Data[0x14F]=C&0xFF;

  strncpy(S,Name,511);S[511]='\0';
  if(!(F=fopen(ChangeExt(S,".fix"),"wb")))
    return("[Couldn't write .FIX]");

  L=GB_ROMSize(Data);
  L=(fwrite(Data,1,L,F)==L);
  fclose(F);
  return(L? NULL:"[Error writing .FIX]");
}

#ifdef ZLIB
#define fopen          gzopen
#define fclose         gzclose
#define fread(B,N,L,F) gzread(F,B,(L)*(N))
#endif

int main(int argc,char *argv[])
{
  char S[256],Comment[1024],*P,*FileName;
  int J,I,K,L,NeedsFix;
  byte Header[512],*Data,C;
  FILE *F;

  if(argc<2)
  {
    fprintf(stderr,"GBLIST .GB File Processor v.3.4 by Marat Fayzullin\n");
#ifdef ZLIB
    fprintf(stderr,"  This program will transparently uncompress GZIPped files.\n");
#endif
    fprintf(stderr,"Usage: %s [-acfhp] files...\n", argv[0]);
    fprintf(stderr,"  -a - Use ANSI escape sequences for colors\n");
    fprintf(stderr,"  -c - Check CMP/CRC and file sizes\n");
    fprintf(stderr,"  -f - Fix CMP/CRC and file sizes\n");
    fprintf(stderr,"  -h - Output results in HTML format\n");
    fprintf(stderr,"  -p - Show supposed producer\n");
    return(1);
  }

  /* Set default values */
  UseANSI=UseHTML=CheckCRC=FixCRC=ShowProducer=0;

  /* Parse command line options */
  for(I=1;I<argc;I++)
    if(*argv[I]=='-')
      for(J=1;argv[I][J];J++)
        switch(argv[I][J])
        {
          case 'a': UseANSI=1;break;
          case 'c': CheckCRC=1;break;
          case 'f': FixCRC=1;break;
          case 'h': UseHTML=1;break;
          case 'p': ShowProducer=1;break;
          default:
            fprintf(stderr,"%s: Unknown option -%c\n",argv[0],argv[I][J]);
        }

  /* No ANSI colors in HTML mode */
  if(UseHTML) UseANSI=0;

  /* Print out the list header */
  if(UseHTML) { puts(TopHTML);puts(HeadHTML);puts(MidHTML); }
  else        { puts(TopLine);puts(HeadLine);puts(MidLine); }
          
  /* Go through the files */
  for(J=1;J<argc;J++)
    if(*argv[J]!='-')
    {
      I=1;NeedsFix=0;*Comment='\0';

      /* Finding out the name of a file */
      for(P=argv[J]+strlen(argv[J]);P>=argv[J];P--)
        if((*P=='\\')||(*P=='/')) break;
      FileName=P+1;

      /* Opening a file */
      if(!(F=fopen(argv[J],"rb")))
      { I=0;Error(FileName,"Couldn't open file"); }

      /* Reading the header */
      if(I)
        if(fread(Header,1,512,F)!=512)
        { I=0;Error(FileName,"Couldn't read header"); }

      /* Checking the header and reading the values */
      if(I)
      {
        /* Checking ROM size */
        if(!GB_ROMBanks(Header))
        { I=0;Error(FileName,"Zero ROM size"); }

        /* If requested, checking manufacturer's code */
        if(ShowProducer&&(P=GB_Maker(Header)))
        { sprintf(S,"[by %s]",P);strcat(Comment,S); }

        /* Checking complement byte */
        K=GB_RealCMP(Header);
        if(K!=GB_CMP(Header))
        {
          sprintf(S,"[Real CMP = %02X]",K);
          strcat(Comment,S);NeedsFix=1;
        }

        /* Checking for type consistency */
        if(!GB_ValidType(Header))
          strcat(Comment,"[Bad cartridge type]");

        /* Checking for RAM size consistency */
        if(Header[0x149]>5)
          strcat(Comment,"[Bad RAM size]");

        /* Checking for ROM size consistency */
        if(GB_ROMBanks(Header)>512)
          strcat(Comment,"[Bad ROM size]");

        /* Checking for Rumble Pack consistency */
        if(GB_Rumble(Header)&&(GB_RAMSize(Header)>0x10000))
          strcat(Comment,"[Too much RAM for rumble cart]");
      }

      /* Loading data if necessary */
      if(I&&(CheckCRC||FixCRC))
      {
        /* Calculating data size */
        K=GB_ROMSize(Header);

        /* Allocating memory for the data */
        if(K>8192*1024)
        { Data=NULL;strcat(Comment,"[File too long to fix CRC]"); }
        else if(!(Data=malloc(K)))
               strcat(Comment,"[Couldn't allocate memory]");

        if(Data)
        {
          /* Copying the header */
          memcpy(Data,Header,512);K-=512;

          /* Reading the data */
          if(fread(Data+512,1,K,F)!=K)
          { free(Data);Data=NULL;strcat(Comment,"[File too short]"); }
        }

        if(Data)
        {
          /* Checking for trash */
          if(fread(&C,1,1,F))
          { strcat(Comment,"[File too long]");NeedsFix=1; }

          /* Checking CRC */
          K=GB_RealCRC(Data);
          if(K!=GB_CRC(Header))
          {
            sprintf(S,"[Real CRC = %04X]",K);
            strcat(Comment,S);NeedsFix=1;
          }

          /* If needed, write out a .FIX file */
          if(FixCRC&&NeedsFix) WriteOutFIX(argv[J],Data);

          free(Data);
        }
      }

      /* Closing the file */
      if(F) fclose(F);

      /* Printing out the information line */
      if(I)
      {
        /* Make up RAM size string */
        K=GB_RAMSize(Header);       
        if(!K) S[0]='\0';
        else if(K<1024) sprintf(S,"%dB",K);
             else sprintf(S,"%dkB",K/1024);

        /* Print out the line */
        printf
        (
          UseHTML? DataHTML:DataLine,
          FileName,
          GB_Name(Header),
          GB_OnlyColor(Header)? '.':'G',
          GB_SuperGB(Header)?   'S':'.',
          GB_ColorGB(Header)?   'C':'.',
          GB_Japanese(Header)?  'J':'.',
          GB_Timer(Header)?     'T':'.',
          GB_Rumble(Header)?    'U':'.',
          /* ROM always present */ 'R',
          GB_RAMSize(Header)?   'W':'.',
          GB_Battery(Header)?   'B':'.',
          GB_MBC1(Header)?      '1':
          GB_MBC2(Header)?      '2':
          GB_MBC3(Header)?      '3':
          GB_MBC4(Header)?      '4':
          GB_MBC5(Header)?      '5':
          GB_HuC1(Header)?      'A':
          GB_HuC3(Header)?      'B':'.',
          GB_ROMSize(Header)/1024,
          S,
          GB_MakerID(Header),
          GB_Version(Header),
          GB_CRC(Header),
          GB_CMP(Header)
        );

        /* Print out comment if present */
        if(!*Comment) puts("");
        else
          printf
          (
            "%s %s%s\n",
            UseHTML? "<I>":UseANSI? "\033[35m":"",
            Comment,
            UseHTML? "</I>":UseANSI? "\033[0m":""
          );
      }
    }

  puts(UseHTML? BotHTML:BotLine);
  return(0);
}