/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                         uPD765.c                        **/
/**                                                         **/
/** This file contains emulation for the i8272/uPD765 disk  **/
/** controller produced by Intel et al. See uPD765.h for    **/
/** implementation.                                         **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 2007-2008                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef UPD765_H
#define UPD765_H

#include "FDIDisk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UPD765_KEEP    0x00
#define UPD765_INIT    0x01
#define UPD765_EJECT   0x02
#define UPD765_A       0x00
#define UPD765_B       0x10

#define UPD765_STATUS  0
#define UPD765_DATA    1
#define UPD765_COMMAND 1

typedef struct
{
  byte R[2];         /* Registers (0=status, 1=data)         */
  byte Cmd[16];      /* Command is being read here           */
  byte ST[4];        /* Status registers                     */
  byte PCN;          /* Current track number after SEEK      */
  byte *Ptr;         /* Data pointer when reading/writing    */
  int DWLength;      /* Data left to write or 0 if idle      */
  int DRLength;      /* Data left to read or 0 if idle       */
  int CWLength;      /* Command bytes left to write or 0     */
  int CRLength;      /* Status bytes left to read or 0       */
  FDIDisk *Disk[4];  /* Floppy disk images for each drive    */
  int Track[4];      /* Current track for each drive         */
  int Side[4];       /* Current side for each drive          */
  byte Model;        /* 0: uPD765A, otherwise uPD765B        */
  byte IRQ;          /* 1: IRQ is pending                    */

  byte Verbose;      /* 1: Print debugging messages          */
} uPD765;

/** ResetUPD765() ********************************************/
/** Reset uPD765. When Disks=UPD765_INIT, also initialize   **/
/** disks. When Disks=UPD765_EJECT, eject inserted disks,   **/
/** freeing memory.                                         **/
/*************************************************************/
void ResetUPD765(uPD765 *D,FDIDisk *Disks,byte Eject);

/** ReadUPD765() *********************************************/
/** Read value from a uPD765 register A. Returns read data  **/
/** on success or 0xFF on failure (bad register address).   **/
/*************************************************************/
byte ReadUPD765(register uPD765 *D,register byte A);

/** WriteUPD765() ********************************************/
/** Write value V into uPD765 register A. Returns DRQ/IRQ   **/
/** values.                                                 **/
/*************************************************************/
byte WriteUPD765(register uPD765 *D,register byte A,register byte V);

/** ExecUPD765() *********************************************/
/** Execute given uPD765 command.                           **/
/*************************************************************/
void ExecUPD765(register uPD765 *D,register const byte *Cmd);

#ifdef __cplusplus
}
#endif
#endif /* UPD765_H */
