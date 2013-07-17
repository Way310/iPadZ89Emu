/** BEHEAD: File Truncation Utility **************************/
/**                                                         **/
/**                         behead.c                        **/
/**                                                         **/
/** This program will cut a given number of bytes off the   **/
/** beginning of a file.                                    **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1998-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me if you make any     **/
/**     changes to this file.                               **/
/*************************************************************/
#include <stdio.h>

int main(int argc,char *argv[])
{
  FILE *FI,*FO;
  int Head,J;

  /* If no arguments given, print out help message */
  if((argc>4)||(argc<2))
  {
    fprintf(stderr,"BEHEAD File Truncation Utility v.1.1 by Marat Fayzullin\n");
    fprintf(stderr,"Usage: %s bytes [infile [outfile]]\n",argv[0]);
    return(1);
  }

  /* Find number of bytes to cut */
  Head=atoi(argv[1]);
  if(Head<0) Head=0;

  /* Open file for reading */
  if(argc<3) FI=stdin;
  else
    if(!(FI=fopen(argv[2],"rb")))
    {
      fprintf
      (
        stderr,
        "%s: Can't open file '%s' for reading\n",
        argv[0],argv[2]
      );
      return(2);
    }

  /* Read given number of char from the file */
  while(Head&&(fgetc(FI)>=0)) Head--;

  /* Error if too few bytes */
  if(Head)
  {
    fprintf(stderr,"%s: Input file too short\n",argv[0]);
    if(FI!=stdin) fclose(FI);
    return(4);
  }

  /* Open file for writing */
  if(argc<4) FO=stdout;
  else
    if(!(FO=fopen(argv[3],"wb")))
    {
      fprintf
      (
        stderr,
        "%s: Can't open file '%s' for writing\n",
        argv[0],argv[3]
      );
      if(FI!=stdin) fclose(FI);
      return(3);
    }

  /* Copy remaining bytes */
  while((J=fgetc(FI))>=0) fputc(J,FO);

  /* Done */
  if(FI!=stdin)  fclose(FI);
  if(FO!=stdout) fclose(FO);
  return(0);
}
