/** FAM2FDS: Famicom DiskSystem Disk Image Converter *********/
/**                                                         **/
/**                        fam2fds.c                        **/
/**                                                         **/
/** This program will convert DiskSystem disk images in     **/
/** .FAM format to the .FDS format supported by iNES.       **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1998-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me if you make any     **/
/**     changes to this file.                               **/
/*************************************************************/
#include <stdio.h>
#include <string.h>

int main(int argc,char *argv[])
{
  FILE *FIn,*FOut;
  unsigned char *Buf,Name[4];
  int J,I,NewGame;

  /* If no arguments given, print out help message */
  if(argc!=3)
  {
    fprintf(stderr,"FAM2FDS File Convertor v.1.1 by Marat Fayzullin\n");
    fprintf(stderr,"Usage: %s file.fam file.fds\n",argv[0]);
    return(1);
  }

  if(!(FIn=fopen(argv[1],"rb"))) return(2);
  if(!(FOut=fopen(argv[2],"wb"))) return(3);

  if(fseek(FIn,-262192,SEEK_END))
  { fclose(FIn);fclose(FOut);return(4); }

  if(!(Buf=(unsigned char *)malloc(66000)))
  { fclose(FIn);fclose(FOut);return(5); }

  for(J=0;J<4;J++)
  {
    /* Attempt reading a disk image */
    printf("Disk %d: READ:",J);
    if(fread(Buf,1,65500,FIn)!=65500) break;

    /* Check if game is the same */
    if(J) NewGame=memcmp(Name,Buf+16,4);
    else
    {
      memcpy(Name,Buf+16,4);
      NewGame=0;
    }

    /* Check disk image for validity */
    printf("OK TEST1:");
    if(Buf[0]!=1) break;
    printf("OK TEST2:");
    if(Buf[56]!=2) break;
    printf("OK TEST3:");
    if(Buf[58]!=3) break;
    printf("OK SIDE:");
    printf("%s ",!NewGame&&(Buf[21]!=(J&1))? "WRONG?":"OK");

    /* Write a disk image */
    printf("WRITE:");
    if(fwrite(Buf,1,65500,FOut)!=65500) break;

    /* Down with an image */
    puts(NewGame? "NEW GAME?":"OK");
  }

  if(J<4) puts("FAILED");
  free(Buf);
  fclose(FIn);
  fclose(FOut);
  return(0);
}