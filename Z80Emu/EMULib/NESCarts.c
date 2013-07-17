/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                       NESCarts.c                        **/
/**                                                         **/
/** This file contains functions to process NES cartridges  **/
/** in the .NES file format. Also see NESCarts.h.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1998-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "NESCarts.h"

/** NES_CRC() ************************************************/
/** Calculate 16bit CRC of a given number of bytes.         **/
/*************************************************************/
unsigned short NES_CRC(const unsigned char *Data,int N)
{
  unsigned short R;
  int J;

  for(J=0,R=0;J<N;J++) R+=*Data++;
  return(R&0xFFFF);
}
