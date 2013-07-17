/** UNDOUBLE: File Truncation Utility ************************/
/**                                                         **/
/**                        undouble.c                       **/
/**                                                         **/
/** This program will truncate doubled files.               **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1998-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me if you make any     **/
/**     changes to this file.                               **/
/*************************************************************/
#include <stdio.h>

int main(int argc,char *argv[])
{
  unsigned char *Buf;
  unsigned long J,I,Total;
  FILE *F;

  /* If no arguments given, print out help message */
  if((argc<2)||(argc>3))
  {
    fprintf(stderr,"UNDOUBLE File Truncation Utility v.1.2 by Marat Fayzullin\n");
    fprintf(stderr,"Usage: %s infile [outfile]\n",argv[0]);
    return(1);
  }

  /* Open file for reading */
  if(!(F=fopen(argv[1],"rb")))
  {
    fprintf
    (
      stderr,
      "%s: Can't open file '%s' for reading\n",
      argv[0],argv[1]
    );
    return(2);
  }

  /* Find out file size */
  fseek(F,0,SEEK_END);
  Total=J=ftell(F);
  rewind(F);

  /* File is empty or has odd length, do not undouble */
  if(!Total||(Total%2)) { fclose(F);return(0); }

  /* Allocate memory */
  if(!(Buf=(unsigned char *)malloc(Total)))
  {
    fprintf(stderr,"%s: Can't allocate %lu bytes\n",argv[0],Total);
    fclose(F);
    return(3);
  }

  /* Read the file */
  if(fread(Buf,1,Total,F)!=Total)
  {
    fprintf
    (
      stderr,
      "%s: Can't read file '%s'\n",
      argv[0],
      F==stdin? "stdin":argv[1]
    );
    fclose(F);
    free(Buf);
    return(4);
  }

  /* Cut the file until its size is odd */
  do
  {
    /* Half the buffer size */
    J/=2;

    /* Check the buffer */
    for(I=0;(I<J)&&(Buf[I]==Buf[J+I]);I++);

    /* Halves are different! Break out */
    if(I<J) { J*=2;break; }
  }
  while(!(J&1));

  /* Done reading file */
  if(F!=stdin) fclose(F);

  /* Open file for writing */
  if(argc<3) F=stdout;
  else
    if(!(F=fopen(argv[2],"wb")))
    {
      fprintf
      (
        stderr,
        "%s: Can't open file '%s' for writing\n",
        argv[0],
        argv[2]
      );
      free(Buf);
      return(5);
    }

  /* Write into file */
  if(fwrite(Buf,1,J,F)!=J)
  {
    fprintf
    (
      stderr,
      "%s: Can't write file '%s'\n",
      argv[0],
      F==stdout? "stdout":argv[2]
    );
    fclose(F);
    free(Buf);
    return(6);
  }

  /* Done */
  if(J<Total) fprintf(stderr,"%s: Found %dx%lu chunks\n",argv[0],Total/J,J);
  if(F!=stdout) fclose(F);
  free(Buf);
  return(0);
}
