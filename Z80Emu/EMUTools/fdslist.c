/** FDSLIST: Famicom DiskSystem Directory Viewer *************/
/**                                                         **/
/**                        fdslist.c                        **/
/**                                                         **/
/** This program will list files contained in a Famicom     **/
/** DiskSystem disk image (.FDS file).                      **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1998-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me if you make any     **/
/**     changes to this file.                               **/
/*************************************************************/
#include <stdio.h>
#include <string.h>

char *Separator = "------------------------------------------------------";

int main(int argc,char *argv[])
{
  static unsigned char DiskHeader[] =
  {
    0x01,0x2A,0x4E,0x49,0x4E,0x54,0x45,0x4E,
    0x44,0x4F,0x2D,0x48,0x56,0x43,0x2A
  };
  unsigned char Buf[64];
  char S[16],T[16];
  int Disk,Files,Start,Size,Total,J,I;
  FILE *F;

  /* If no arguments given, print out help message */
  if(argc<2)
  {
    fprintf(stderr,"FDSLIST .FDS File Processor v.1.1 by Marat Fayzullin\n");
    fprintf(stderr,"Usage: %s file.fds\n",argv[0]);
    return(1);
  }

  /* Open the disk image */
  if(!(F=fopen(argv[1],"rb")))
  {
    fprintf(stderr,"%s: Can't open disk image '%s'\n",argv[0],argv[1]);
    return(2);
  }

  /* Find total disk image size */
  fseek(F,0,SEEK_END);
  Total=ftell(F)/65500;
  J=ftell(F)%65500;

  /* Check out if it is integer number of disks */
  if(J) fprintf(stderr,"%s: %d excessive bytes\n",argv[0],J);

  /* Scan through disks */
  rewind(F);
  for(Disk=0;Disk<Total;Disk++)
  {
    /* Seek to the next disk */
    fseek(F,Disk*65500,SEEK_SET);

    /* Read the disk header */
    if(fread(Buf,1,58,F)!=58)
    {
      fprintf(stderr,"%s: Can't read disk header\n",argv[0]);
      return(3);
    }

    /* Check if disk header ID s valid */
    if(memcmp(Buf,DiskHeader,15))
    {
      fprintf(stderr,"%s: Invalid disk header\n",argv[0]);
      return(4);
    }

    /* Check if file number header ID is valid */
    if(Buf[56]!=2)
    {
      fprintf(stderr,"%s: Invalid file number header\n",argv[0]);
      return(5);
    }

    /* Show disk information */
    memcpy(S,Buf+16,4);S[4]='\0';
    Files=Buf[57];
    printf
    (
      "DISK '%-4s'  Side %c  Files %d  Maker $%02X  Version $%02X\n%s\n",
      S,(Buf[21]&1)+'A',Files,Buf[15],Buf[20],Separator
    );

    /* Scan through the files */
    for(I=0;(I<Files)&&(fread(Buf,1,16,F)==16);I++)
    {
      /* Check if header block ID is valid */
      if(Buf[0]!=3)
      {
        fprintf(stderr,"%s: Invalid file header $%02X\n",argv[0],Buf[0]);
        return(6);
      }

      /* Get name, data location, and size */
      strncpy(S,Buf+3,8);S[8]='\0';
      Start=Buf[11]+256*Buf[12];
      Size=Buf[13]+256*Buf[14];

      /* Check if data block ID is valid */
      J=fgetc(F);
      if(J!=4)
      {
        fprintf(stderr,"%s: Invalid file header $%02X\n",argv[0],J);
        return(7);
      }

      /* List the file */
      sprintf(T,"$%02X?",Buf[15]);
      printf
      (
        "%03d $%02X '%-8s' $%04X-$%04X [%s]\n",
        Buf[1],Buf[2],S,Start,Start+Size-1,
        Buf[15]>2? T:
        Buf[15]>1? "PICTURE":
        Buf[15]>0? "TILES":
                   "CODE"
      );

      /* Seek over the data */
      fseek(F,Size,SEEK_CUR);
    }

    /* Done with a disk */
    puts(Separator);
  }

  /* Done */
  return(0);
}
