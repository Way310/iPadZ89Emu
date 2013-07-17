/** GBALIST: GameBoy Advance Cartridge Lister/Tester *********/   
/**                                                         **/
/**                         gbalist.c                       **/
/**                                                         **/
/** This program will list and optionally test a group of   **/
/** cartridges finding those which fail complement check.   **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2000-2001                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "GBACarts.h"

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
char *ErrorLine = "³%-20s³%s %-44s %s³\n";
char *DataLine  = "³%-20s³ %-16s ³ %c%c%c%c/%c%c ³ %02X ³ %02X ³ %02X ³";
char *TopLine   = "ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÂÄÄÄÄÂÄÄÄÄÂÄÄÄÄ¿";
char *HeadLine  = "³ File               ³ Name             ³ IDs     ³ UC ³ DT ³ VE ³";
char *MidLine   = "ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÅÄÄÄÄÅÄÄÄÄÅÄÄÄÄ´";
char *BotLine   = "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÁÄÄÄÄÁÄÄÄÄÁÄÄÄÄÙ";
#else
char *ErrorLine = "|%-20s|%s %-44s %s|\n";
char *DataLine  = "|%-20s| %-16s | %c%c%c%c/%c%c | %02X | %02X | %02X |";
char *TopLine   = "+--------------------+------------------+---------+----+----+----+";
char *HeadLine  = "| File               | Name             | IDs     | UC | DT | VE |";
char *MidLine   = "+--------------------+------------------+---------+----+----+----+";
char *BotLine   = "+--------------------+------------------+---------+----+----+----+";
#endif

char *ErrorHTML = "<A HREF=\"%s\">%s %-44s %s</A>\n";
char *DataHTML  = "<A HREF=\"%s\">%-16s</A>  %c%c%c%c/%c%c  %02X  %02X  %02X ";
char *TopHTML   = "<PRE><HR><B>Name              IDs      UC  DT  VE </B><HR>";
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
  return(0);
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
    fprintf(stderr,"GBALIST .GBA File Processor v.1.1 by Marat Fayzullin\n");
#ifdef ZLIB
    fprintf(stderr,"  This program will transparently uncompress GZIPped files.\n");
#endif
    fprintf(stderr,"Usage: %s [-acfhp] files...\n", argv[0]);
    fprintf(stderr,"  -a - Use ANSI escape sequences for colors\n");
    fprintf(stderr,"  -c - Check CMP\n");
    fprintf(stderr,"  -f - Fix CMP\n");
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
  if(UseHTML) puts(TopHTML);
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
        /* If requested, checking manufacturer's code */
        if(ShowProducer&&(P=GBA_Maker(Header)))
        { sprintf(S,"[by %s]",P);strcat(Comment,S); }

        /* Checking complement byte */
        K=GBA_RealCMP(Header);
        if(K!=GBA_CMP(Header))
        {
          sprintf(S,"[Real CMP = %02X]",K);
          strcat(Comment,S);NeedsFix=1;
        }
      }

      /* Loading data if necessary */
      if(I&&(CheckCRC||FixCRC))
      {
      }

      /* Closing the file */
      if(F) fclose(F);

      /* Printing out the information line */
      if(I)
      {
        /* Print out the line */
        printf
        (
          UseHTML? DataHTML:DataLine,
          FileName,
          GBA_Title(Header),
          GBA_GameID(Header)>>24,
          (GBA_GameID(Header)>>16)&0xFF,
          (GBA_GameID(Header)>>8)&0xFF,
          GBA_GameID(Header)&0xFF,
          GBA_MakerID(Header)>>8,
          GBA_MakerID(Header)&0xFF,
          GBA_UnitID(Header),
          GBA_DevType(Header),
          GBA_Version(Header)
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