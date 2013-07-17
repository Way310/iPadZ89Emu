/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                          C93XX.c                        **/
/**                                                         **/
/** This file contains emulation for the C93xx series of    **/
/** serial EEPROMs. See C93XX.h for declarations.           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2000-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "C93XX.h"
#include <string.h>
#include <stdio.h>

/** Reset93XX ************************************************/
/** Reset the 93xx chip.                                    **/
/*************************************************************/
void Reset93XX(C93XX *D,byte *Data,int Flags)
{
  static const int AddrLength[4] = { 7,8,9,11 };

  D->AddrLen = AddrLength[Flags&C93XX_CHIP]-(Flags&C93XX_16BIT? 1:0);
  D->DataLen = Flags&C93XX_16BIT? 16:8; 
  D->Debug   = Flags&C93XX_DEBUG? 1:0;
  D->Data    = Data;
  D->In      = 0x00000000;
  D->Out     = 0xFFFFFFFF;
  D->Addr    = 0x0000;
  D->Writing = 0;
  D->Reading = 0;
  D->Count   = 0;
  D->EnWrite = 0;
  D->Last    = C93XX_CLOCK|C93XX_DATA;
}

/** Read93XX *************************************************/
/** Read value from the 93xx chip. Only bits 0,1 are used   **/
/** (see #defines).                                         **/
/*************************************************************/
byte Read93XX(C93XX *D)
{
  return(((D->Out>>31)&C93XX_DATA)|C93XX_CLOCK);
}

/** Write93XX ************************************************/
/** Write value V into the 93xx chip. Only bits 0,1,2 are   **/
/** used (see #defines).                                    **/
/*************************************************************/
void Write93XX(C93XX *D,byte V)
{
  static const char *Cmds[4] = { "ENADIS","WRITE","READ","ERASE" };

  /* If CLOCK goes from 0 to 1... */
  if((D->Last^V)&V&C93XX_CLOCK)
  {
    /* If debugging, print received bit */
    if(D->Debug) printf("%c",V&1? '1':'0');

    /* Save last written value */
    D->Last=V;

    /* Shift output bits and add status bit */
    D->Out=(D->Out<<1)|1;

    /* CS has to be up */
    if(!(V&C93XX_CS)) return;

    /* Shift input bits and add new bit */
    D->In=(D->In<<1)|(V&C93XX_DATA);

    /* Looking for a command, commands start with 1 */
    if(!D->Count)
    {
      if(D->In&1) D->Count=2+D->AddrLen;
      return;
    }

    /* If bit counter hits 0... */
    if(!--D->Count)
    {
      /* If finishing READ cycle... */
      if(D->Reading) { D->Reading=0;D->In=0x00000000;return; }

      /* If finishing WRITE cycle... */
      if(D->Writing)
      {
        /* WRITE cycle finished */
        D->Writing=0;

        /* If write enabled... */
        if(D->EnWrite)
        {
          if(D->DataLen==8)
          {
            /* Write received word into EEPROM */
            D->Data[D->Addr]=D->In&0xFF;

            /* If debugging, print received word */
            if(D->Debug)
              printf(" IN%c%02X ",D->EnWrite? '=':'x',D->In&0xFF);
          }
          else
          {
            /* Write received word into EEPROM */
            D->Addr<<=1; 
            D->Data[D->Addr]   = (D->In>>8)&0xFF;
            D->Data[D->Addr+1] = D->In&0xFF;

            /* If debugging, print received word */
            if(D->Debug)
              printf(" IN%c%04X ",D->EnWrite? '=':'x',D->In&0xFFFF);
          }
        }

        D->In=0x00000000;
        return;
      }

      /* If debugging, print received command */
      if(D->Debug)
        printf(" %s(%X) ",Cmds[(D->In>>D->AddrLen)&3],D->In&((1<<D->AddrLen)-1));

      switch((D->In>>D->AddrLen)&3)
      {
        case 0: /* Write enable/disable */
          switch((D->In>>(D->AddrLen-2))&3)
          {
            case 0: /* EWDS */
              D->EnWrite=0;
              break;
            case 1: /* WRAL */
              if(D->EnWrite)
                memset(D->Data,0x00,(D->DataLen==16? 2:1)<<D->AddrLen);
              break;
            case 2: /* ERAL */
              if(D->EnWrite)
                memset(D->Data,0xFF,(D->DataLen==16? 2:1)<<D->AddrLen);
              break;
            case 3: /* EWEN */
              D->EnWrite=1;
              break;
          }
          D->In=0x00000000;
          break;

        case 1: /* WRITE */
          D->Writing=1;
          D->Count=D->DataLen;
          D->Addr=D->In&((1<<D->AddrLen)-1);
          break;

        case 2: /* READ */
          D->Reading=1;
          D->Count=D->DataLen+1;
          D->In&=((1<<D->AddrLen)-1);
          if(D->DataLen==8)
          {
            /* Read word and put it into output shift register */
            D->Out=((int)D->Data[D->In]<<23)|0x7FFFFF;

            /* If debugging, print the word that has been read */
            if(D->Debug) printf("OUT=%02X ",D->Data[D->In]);
          }
          else
          {
            /* Read word and put it into output shift register */
            D->In<<=1;
            D->Out=0x7FFF;
            D->Out|=(int)D->Data[D->In]<<23;
            D->Out|=(int)D->Data[D->In+1]<<15;

            /* If debugging, print the word that has been read */
            if(D->Debug)
              printf("OUT=%04X ",D->Data[D->In]*256+D->Data[D->In+1]);
          }
          break;

        case 3: /* ERASE */
          if(D->EnWrite)
          {
            D->In&=((1<<D->AddrLen)-1);
            if(D->DataLen==8) D->Data[D->In]=0xFF;
            else D->Data[D->In*2]=D->Data[D->In*2+1]=0xFF;
          }
          D->In=0x00000000;
          break;
      }
    }
  }

  /* Save last written value */
  D->Last=V;
}
