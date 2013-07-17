/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         uPD765.c                        **/
/**                                                         **/
/** This file contains emulation for the i8272/uPD765 disk  **/
/** controller produced by Intel et al. See uPD765.h for    **/
/** declarations.                                           **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2007-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include "uPD765.h"
#include <stdio.h>

#define F_FDD0_BUSY  0x01
#define F_FDD1_BUSY  0x02
#define F_FDD2_BUSY  0x04
#define F_FDD3_BUSY  0x08
#define F_FDC_BUSY   0x10
#define F_EXECMODE   0x20
#define F_DATAOUT    0x40
#define F_DATAMSTR   0x80

                             /* STATUS register at IRQ:        */
#define F0_DRIVE     0x03    /* Current disk drive number      */
#define F0_SIDE      0x04    /* Current side number            */
#define F0_NOTREADY  0x08    /* Disk drive not ready           */
#define F0_FDDFAULT  0x10    /* Disk drive failed              */
#define F0_SEEKEND   0x20    /* SEEK operation finished        */
#define F0_RESULT    0xC0    /* Command execution result:      */
#define F0_SUCCESS   0x00    /* Command completed successfully */
#define F0_FAILURE   0x40    /* Command execution failed       */
#define F0_BADCMD    0x80    /* Bad command issued             */
#define F0_WSNREADY  0xC0    /* Disk drive was not ready       */

#define F1_NOADDR    0x01    /* Address mark not found         */
#define F1_READONLY  0x02    /* The disk is write-protected    */
#define F1_NOSECTOR  0x04    /* Sector not found or deleted    */
#define F1_OVERRRUN  0x10    /* DRQ hasn't been handled        */
#define F1_BADCRC    0x20    /* Sector ID or data CRC wrong    */
#define F1_ENDTRACK  0x80    /* End of the track reached       */

#define F3_DRIVE     0x03
#define F3_SIDE      0x04
#define F3_TWOSIDES  0x08
#define F3_TRACK0    0x10
#define F3_READY     0x20
#define F3_READONLY  0x40
#define F3_FAULT     0x80

/** ResetUPD765() ********************************************/
/** Reset uPD765. When Disks=UPD765_INIT, also initialize   **/
/** disks. When Disks=UPD765_EJECT, eject inserted disks,   **/
/** freeing memory.                                         **/
/*************************************************************/
void ResetUPD765(uPD765 *D,FDIDisk *Disks,byte Eject)
{
  int J;

  /* Reset controller state */
  D->R[0]     = 0x00;
  D->R[1]     = 0x00;
  D->ST[0]    = 0x00;
  D->ST[1]    = 0x00;
  D->ST[2]    = 0x00;
  D->ST[3]    = 0x00;
  D->DRLength = 0;
  D->DWLength = 0;
  D->CRLength = 0;
  D->CWLength = 0;
  D->Model    = Eject&UPD765_B;
  D->IRQ      = 0;
  Eject      &= 3;

  /* For all drives... */
  for(J=0;J<4;++J)
  {
    /* Reset drive-dependent state */
    D->Disk[J]  = Disks? &Disks[J]:0;
    D->Track[J] = 0;
    D->Side[J]  = 0;
    /* Initialize disk structure, if requested */
    if((Eject==UPD765_INIT)&&D->Disk[J])  InitFDI(D->Disk[J]);
    /* Eject disk image, if requested */
    if((Eject==UPD765_EJECT)&&D->Disk[J]) EjectFDI(D->Disk[J]);
  }
}

/** ReadUPD765() *********************************************/
/** Read value from a uPD765 register A. Returns read data  **/
/** on success or 0xFF on failure (bad register address).   **/
/*************************************************************/
byte ReadUPD765(register uPD765 *D,register byte A)
{
  int Drive;

  switch(A)
  {
     case UPD765_STATUS: /* STATUS register */
//       printf("uPD765: Read STATUS register\n");
       /* Reset pending IRQ */
       D->IRQ=0;
       /* Read status */
       return(
         (D->DRLength||D->DWLength? F_EXECMODE:0)
       | (D->DRLength||D->CRLength? F_DATAOUT:0)
       | F_DATAMSTR
       );

     case UPD765_DATA: /* DATA register */
//       printf("uPD765: Read DATA register\n");
       /* If there is data to read... */
       if(D->DRLength)
       {
         /* Read data */
         A=*D->Ptr++;
         /* If data has been fully read... */
         if(!--D->DRLength)
         {
           if(D->Verbose) printf("uPD765: DONE reading data\n");
           /* At the end of data read, return status */
           Drive       = D->Cmd[1]&0x03;
           D->Cmd[6]   = D->ST[0] = D->Cmd[1]&0x07;
           D->Cmd[5]   = D->ST[1] = 0x00;
           D->Cmd[4]   = D->ST[2] = 0x00;
           D->Cmd[3]   = D->Disk[Drive]->Header[0];
           D->Cmd[2]   = D->Disk[Drive]->Header[1];
           D->Cmd[1]   = D->Disk[Drive]->Header[2];
           D->Cmd[0]   = D->Disk[Drive]->Header[3];
           D->CRLength = 7;
         }
         /* Generate an IRQ */
         D->IRQ=1;
         /* Return read data */
         return(A);
       }

       /* If there is status to read... */
       if(D->CRLength)
       {
         /* Generate an IRQ */
         D->IRQ=1;
         return(D->Cmd[--D->CRLength]);
       }

       /* Return default value */
       return(0x00);
  }

  /* Wrong register address */
  return(0xFF);
}

/** WriteUPD765() ********************************************/
/** Write value V into uPD765 register A. Returns DRQ/IRQ   **/
/** values.                                                 **/
/*************************************************************/
byte WriteUPD765(register uPD765 *D,register byte A,register byte V)
{
  int Drive;

  switch(A)
  {
     case UPD765_COMMAND: /* COMMAND register */
//       printf("uPD765: Write 0x%02X to COMMAND register\n",V);
       /* If reading data/command from the controller, ignore writes */
       if(D->DRLength||D->CRLength) break;

       /* If expecting data... */
       if(D->DWLength)
       {
         /* Write data */
         *D->Ptr++=V;
         /* If data has been fully written... */
         if(!--D->DWLength)
         {
           if(D->Verbose) printf("uPD765: DONE writing data\n");
           /* At the end of data read, return status */
           Drive       = D->Cmd[1]&0x03;
           D->Cmd[6]   = D->ST[0] = D->Cmd[1]&0x07;
           D->Cmd[5]   = D->ST[1] = 0x00;
           D->Cmd[4]   = D->ST[2] = 0x00;
           D->Cmd[3]   = D->Disk[Drive]->Header[0];
           D->Cmd[2]   = D->Disk[Drive]->Header[1];
           D->Cmd[1]   = D->Disk[Drive]->Header[2];
           D->Cmd[0]   = D->Disk[Drive]->Header[3];
           D->CRLength = 7;
         }
         /* Generate an IRQ */
         D->IRQ=1;
         break;
       }

       /* If reading command parameters... */
       if(D->CWLength)
       {
         /* Continue accepting command parameters */
         *D->Ptr++=V;
         /* If command has been fully written, execute it */
         if(!--D->CWLength) ExecUPD765(D,D->Cmd);
         /* Generate an IRQ */
         D->IRQ=1;
         break;
       }

       /* Expecting a new command */
       D->Ptr=D->Cmd+1;
       switch(D->Cmd[0]=V)
       {
         case 0x03: /* SPECIFY */
         case 0x0F: /* SEEK */
           /* Two more bytes needed */
           D->CWLength=2;
           break;
         case 0x04:
         case 0x0C: /* DRIVE-STATUS */
         case 0x07: /* RECALIBRATE */
         case 0x0A:
         case 0x4A: /* READ-ID */
           /* One more byte needed */
           D->CWLength=1;
           break;
         case 0x08: /* IRQ-STATUS */
         default:   /* INVALID */
         case 0x10: case 0x30: case 0x50: case 0x70:
         case 0x90: case 0xB0: case 0xD0: case 0xF0:
           /* VERSION */
           ExecUPD765(D,D->Cmd);
           break;

         case 0x06: case 0x26: case 0x46: case 0x66:
         case 0x86: case 0xA6: case 0xC6: case 0xE6:
           /* READ-DATA */
         case 0x4C: case 0x8C: case 0xCC:
           /* READ-DELETED-DATA */
         case 0x05: case 0x45: case 0x85: case 0xC5:
           /* WRITE-DATA */
         case 0x09: case 0x49: case 0x89: case 0xC9:
           /* WRITE-DELETED-DATA */
         case 0x02: case 0x22: case 0x42: case 0x62:
           /* READ-DIAG */
           /* These commands require 8 more bytes */
           D->CWLength=8;
           break;
       }

       /* Done */
       break;
  }

  /* Done */
  return(1);
}

/** ExecUPD765() *********************************************/
/** Execute given uPD765 command.                           **/
/*************************************************************/
void ExecUPD765(register uPD765 *D,register const byte *Cmd)
{
  int Drive,Head,J;

  /* Generate an IRQ */
  D->IRQ=1;

  switch(Cmd[0])
  {
    case 0x06: case 0x26: case 0x46: case 0x66:
    case 0x86: case 0xA6: case 0xC6: case 0xE6:
      /* READ-DATA */
    case 0x4C: case 0x8C: case 0xCC:
      /* READ-DELETED-DATA */
      /* Arguments: CMD,DISK,TRACK,HEAD,SECTOR,LENGTH,EOT,GQL,DTL */
      Drive = Cmd[1]&0x03;
      J     = 0x80<<Cmd[5];
      /* Seek to requested sector */
      D->Ptr=SeekFDI(D->Disk[Drive],Cmd[3],D->Track[Drive],Cmd[3],Cmd[2],Cmd[4]);
      if(D->Verbose) printf("uPD765: READ-%sDATA %d bytes from %c:%d:%d:%d ==> %s\n",Cmd[0]&0x08? "DELETED-":"",J,Drive+'A',Cmd[3],Cmd[2],Cmd[4],D->Ptr? "OK":"FAILED");
      /* If seek failed... */
      if(!D->Ptr)
      {
        D->Cmd[6]   = D->ST[0] =
          (D->Cmd[1]&0x07)
        | (D->Disk[Drive]? 0:F0_NOTREADY)
        | F0_FAILURE;
        D->Cmd[5]   = D->ST[1] =
          (D->Disk[Drive]? F1_NOSECTOR:0);
        D->Cmd[4]   = D->ST[2] = 0x00;
        D->Cmd[3]   = 0x00;
        D->Cmd[2]   = 0x00;
        D->Cmd[1]   = 0x00;
        D->Cmd[0]   = 0x00;
        D->CRLength = 7;
      }
      /* Do normal read */
      else
      {
        /* Amount of data to be read from the sector */
        D->DRLength = J;
        /* No status bytes yet */
        D->CRLength = 0;
      }
      break;

    case 0x05: case 0x45: case 0x85: case 0xC5:
      /* WRITE-DATA */
    case 0x09: case 0x49: case 0x89: case 0xC9:
      /* WRITE-DELETED-DATA */
      /* Arguments: CMD,DISK,TRACK,HEAD,SECTOR,LENGTH,EOT,GQL,DTL */
      Drive = Cmd[1]&0x03;
      J     = 0x80<<Cmd[5];
      /* Seek to requested sector */
      D->Ptr=SeekFDI(D->Disk[Drive],Cmd[3],D->Track[Drive],Cmd[3],Cmd[2],Cmd[4]);
      if(D->Verbose) printf("uPD765: WRITE-%sDATA %d bytes to %c:%d:%d:%d ==> %s\n",Cmd[0]&0x08? "DELETED-":"",J,Drive+'A',Cmd[3],Cmd[2],Cmd[4],D->Ptr? "OK":"FAILED");
      /* If seek failed... */
      if(!D->Ptr)
      {
        D->Cmd[6]   = D->ST[0] =
          (D->Cmd[1]&0x07)
        | (D->Disk[Drive]? 0:F0_NOTREADY)
        | F0_FAILURE;
        D->Cmd[5]   = D->ST[1] =
          (D->Disk[Drive]? F1_NOSECTOR:0);
        D->Cmd[4]   = D->ST[2] = 0x00;
        D->Cmd[3]   = 0x00;
        D->Cmd[2]   = 0x00;
        D->Cmd[1]   = 0x00;
        D->Cmd[0]   = 0x00;
        D->CRLength = 7;
      }
      /* Do normal write */
      else
      {
        /* Amount of data to be written to the sector */
        D->DWLength=J;
        /* No status bytes yet */
        D->CRLength=0;
      }
      break;

    case 0x02: case 0x22: case 0x42: case 0x62:
      /* READ-DIAG */
      /* Arguments: CMD,DISK,TRACK,HEAD,SECTOR,LENGTH,EOT,GQL,DTL */
      Drive = Cmd[1]&0x03;
      J     = 0x80<<Cmd[5];
      /* Seek to requested sector */
      D->Ptr=SeekFDI(D->Disk[Drive],Cmd[3],D->Track[Drive],Cmd[3],Cmd[2],Cmd[4]);
      if(D->Verbose) printf("uPD765: READ-DIAG %d bytes from %c:%d:%d:%d ==> %s\n",J,Drive+'A',Cmd[3],Cmd[2],Cmd[4],D->Ptr? "OK":"FAILED");
      /* If seek failed... */
      if(!D->Ptr)
      {
        D->Cmd[6]   = D->ST[0] =
          (D->Cmd[1]&0x07)
        | (D->Disk[Drive]? 0:F0_NOTREADY)
        | F0_FAILURE;
        D->Cmd[5]   = D->ST[1] =
          (D->Disk[Drive]? F1_NOSECTOR:0);
        D->Cmd[4]   = D->ST[2] = 0x00;
        D->Cmd[3]   = 0x00;
        D->Cmd[2]   = 0x00;
        D->Cmd[1]   = 0x00;
        D->Cmd[0]   = 0x00;
        D->CRLength = 7;
      }
      /* Do normal read */
      else
      {
        /* Amount of data to be read from the sector */
        D->DRLength = J;
        /* No status bytes yet */
        D->CRLength = 0;
      }
      break;

    case 0x03: /* SPECIFY */
      if(D->Verbose) printf("uPD765: SPECIFY\n");
      /* We can ignore this command */
      D->CRLength = 0;
      break;

    case 0x04:
    case 0x0C: /* DRIVE-STATUS */
      Drive = Cmd[1]&0x03;
      Head  = (Cmd[1]&0x04)>>2;
      if(D->Verbose) printf("uPD765: DRIVE-STATUS drive %c:%d\n",Drive+'A',Head);
      /* Return ST3 */
      D->CRLength = 1;
      D->Cmd[0]   = D->ST[3] =
        (Cmd[0]&0x07)
      | (D->Track[Drive]? F3_TRACK0:0)
      | (D->Disk[Drive]?  F3_READY:0)
      | (D->Disk[Drive]&&(D->Disk[Drive]->Sides>1)? F3_TWOSIDES:0);
      break;

    case 0x07:
    case 0x17: /* RECALIBRATE */
      Drive = Cmd[1]&0x03;
      if(D->Verbose) printf("uPD765: RECALIBRATE drive %c:\n",Drive+'A');
      /* Head goes to track #0 */
      D->Track[Drive] = 0;
      D->CRLength     = 0;
      /* Set status at the end of RECALIBRATE */
      D->ST[0] = Drive|F0_SEEKEND;
      break;

    case 0x0F: /* SEEK */
      Drive = Cmd[1]&0x03;
      Head  = (Cmd[1]&0x04)>>1;
      J     = D->Disk[Drive]&&(Cmd[2]<D->Disk[Drive]->Tracks);
      if(D->Verbose) printf("uPD765: SEEK drive %c:%d to track %d ==> %s\n",Drive+'A',Head,Cmd[2],J? "OK":"FAILED");
      /* If track exists... */
      if(J)
      {
        /* Head goes to a given track */
        D->Track[Drive] = Cmd[2];
        D->Side[Drive]  = Head;
        D->CRLength     = 0;
      }
      /* Set status at the end of SEEK */
      D->ST[0] = (Cmd[1]&0x07)|F0_SEEKEND;
      break;

    case 0x10: case 0x30: case 0x50: case 0x70:
    case 0x90: case 0xB0: case 0xD0: case 0xF0:
      /* VERSION */
      if(D->Verbose) printf("uPD765: VERSION\n");
      D->Cmd[0]   = D->Model|0x80;
      D->CRLength = 1;
      break;

    case 0x08: /* IRQ-STATUS */
      if(D->Verbose) printf("uPD765: IRQ-STATUS\n");
      if(D->ST[0]&F0_SEEKEND)
      {
        /* Return ST0 and the last SEEKed track */
        D->CRLength = 2;
        D->Cmd[1]   = D->ST[0];
        D->Cmd[0]   = D->Track[D->ST[0]&0x03];
        D->ST[0]    = (D->ST[0]&~(F0_RESULT|F0_SEEKEND))|F0_BADCMD;
      }
      else
      {
        /* Treat as invalid command if no IRQ */
        D->CRLength = 1;
        D->Cmd[0]   = F0_BADCMD;
      }
      break;

    case 0x0A:
    case 0x4A: /* READ-ID */
      Drive = Cmd[1]&0x03;
      Head  = (Cmd[1]&0x04)>>1;
      /* Seek to the first sector */
      for(J=0;J<256;++J)
        if(SeekFDI(D->Disk[Drive],Head,D->Track[Drive],Head,D->Track[Drive],J)) break;
      if(D->Verbose) printf("uPD765: READ-ID drive %c:%d ==> %s\n",Drive+'A',Head,J<256? "OK":"FAILED");
      /* If seek failed... */
      if(J>=256)
      {
      }
      else
      {
        /* Return first sector IDs */
        D->Cmd[6]   = D->ST[0] = 0x00;
        D->Cmd[5]   = D->ST[1] = 0x00;
        D->Cmd[4]   = D->ST[2] = 0x00;
        D->Cmd[3]   = D->Disk[Drive]->Header[0];
        D->Cmd[2]   = D->Disk[Drive]->Header[1];
        D->Cmd[1]   = D->Disk[Drive]->Header[2];
        D->Cmd[0]   = D->Disk[Drive]->Header[3];
        D->CRLength = 7;
      }
      break;

    default: /* INVALID */
      if(D->Verbose) printf("uPD765: Unknown command 0x%02X\n",Cmd[0]);
      D->Cmd[0]   = F0_BADCMD;
      D->CRLength = 1;
      break;
  }
}
