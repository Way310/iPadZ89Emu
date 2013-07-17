/** BDIFF: Binary Comparison Utility *************************/
/**                                                         **/
/**                         bdiff.c                         **/
/**                                                         **/
/** This program will compare two binary files listing the  **/
/** differences.                                            **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include <stdio.h>
#include <string.h>

#ifdef ZLIB
#include <zlib.h>
#define fopen          gzopen
#define fclose         gzclose
int fgetc(FILE *F)
{
  unsigned char N;
  return(gzread(F,&N,1)==1? N:-1);
}
#endif

#define LINEWIDTH 8

int UseANSI,ShowAll;

void Compare(int *C1,int *C2,long Pos)
{
  int I,J;

  if(!ShowAll&&!memcmp(C1,C2,LINEWIDTH*sizeof(int))) return;

  printf("%06X:",Pos*LINEWIDTH);

  for(J=0;J<LINEWIDTH;J++)
    if(C1[J]<0) printf("   ");
    else if(C1[J]==C2[J]) printf(" %02X",C1[J]);
         else printf(UseANSI? " \033[34m%02X\033[0m":"*%02X",C1[J]);

#ifdef MSDOS
  printf(" ³ ");
#else
  printf(" | ");
#endif

  for(J=0;J<LINEWIDTH;J++)
  {
    I=((C1[J]<0x7F)&&(C1[J]>0x1F))? C1[J]:'.';
    if(C1[J]==C2[J]) printf("%c",I);
    else printf(UseANSI? "\033[34m%c\033[0m":"%c",I);
  }

#ifdef MSDOS
  printf(" ³");
#else
  printf(" |");
#endif

  for(J=0;J<LINEWIDTH;J++)
    if(C2[J]<0) printf("   ");
    else if(C2[J]==C1[J]) printf(" %02X",C2[J]);
         else printf(UseANSI? " \033[34m%02X\033[0m":"*%02X",C2[J]);

#ifdef MSDOS
  printf(" ³ ");
#else
  printf(" | ");
#endif

  for(J=0;J<LINEWIDTH;J++)
  {
    I=((C2[J]<0x7F)&&(C2[J]>0x1F))? C2[J]:'.';
    if(C2[J]==C1[J]) printf("%c",I);
    else printf(UseANSI? "\033[34m%c\033[0m":"%c",I);
  }

  printf("\n");
}

int main(int argc,char *argv[])
{
  FILE *F1,*F2;
  int D1,D2,C1[LINEWIDTH],C2[LINEWIDTH],J,N;
  long I;

  if((argc<3)||((argc==3)&&(*argv[1]=='-')))
  {
    fprintf(stderr,"BDIFF Binary Comparison Utility v.2.1 by Marat Fayzullin\n");
#ifdef ZLIB
    fprintf(stderr,"  This program will transparently uncompress GZIPped files.\n");
#endif
    fprintf(stderr,"Usage: %s [-as] file1 file2\n",argv[0]);
    fprintf(stderr,"  -a - Use ANSI escape sequences for colors\n");
    fprintf(stderr,"  -s - Show entire files, not only differences\n");
    return(1);
  }

  UseANSI=ShowAll=0;N=1;

  if(*argv[1]=='-')
  {
    for(J=1;argv[1][J];J++)
      switch(argv[1][J])
      {
        case 'a': UseANSI=1;break;
        case 's': ShowAll=1;break;
        default:
          fprintf(stderr,"%s: Unknown option -%c\n",argv[0],argv[1][J]);
      }
    N=2;
  }

  if(!(F1=fopen(argv[N],"rb")))
  {
    fprintf(stderr,"%s: Couldn't open file \"%s\"\n",argv[0],argv[N]);
    return(1);
  }
  N++;
  if(!(F2=fopen(argv[N],"rb")))
  {
    fprintf(stderr,"%s: Couldn't open file \"%s\"\n",argv[0],argv[N]);
    return(1);
  }

  J=0;I=0;
  do
  {
    C1[J]=D1=fgetc(F1);
    C2[J]=D2=fgetc(F2);
    if((D1>=0)&&(D2>=0))
      if((++J)==LINEWIDTH)
      { Compare(C1,C2,I);J=0;I++; }
  }
  while((D1>=0)&&(D2>=0));

  C1[J]=C2[J]=-1;
  if(J)
  {
    for(;J<LINEWIDTH;J++) C1[J]=C2[J]=-1;
    Compare(C1,C2,I);
  }

  if((D1>=0)&&(D2<0))
    printf("\"%s\" is longer than \"%s\"\n",argv[N-1],argv[N]);
  if((D2>=0)&&(D1<0))
    printf("\"%s\" is longer than \"%s\"\n",argv[N],argv[N-1]);

  fclose(F1);fclose(F2);
  return(0);
}
