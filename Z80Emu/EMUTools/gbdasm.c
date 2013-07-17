/** GBDASM: GameBoy Disassembler ******************************/
/**                                                          **/
/**                         gbdasm.c                         **/
/**                                                          **/
/** This file contains the source of a portable disassembler **/
/** for the customized Z80 CPU used in GameBoy handheld      **/
/** videogame console.                                       **/
/**                                                          **/
/** Copyright (C) Marat Fayzullin 1995-2001                  **/
/**     You are not allowed to distribute this software      **/
/**     commercially. Please, notify me, if you make any     **/
/**     changes to this file.                                **/
/**************************************************************/

#include "GBCarts.h"

#include <stdio.h>
#include <string.h>

#ifdef ZLIB
#include <zlib.h>
#define fopen          gzopen
#define fclose         gzclose
#define fread(B,N,L,F) gzread(F,B,(L)*(N))
#endif

typedef unsigned char byte;   /* This type is exactly 1 byte  */
typedef unsigned short word;  /* This type is exactly 2 bytes */

static int PrintHex;          /* Print hexadecimal codes      */
static int PrintInfo;         /* Print cartridge information  */
static unsigned long Counter; /* Address counter              */

static int DAsm(char *S,byte *A);
    /* This function will disassemble a single command and    */
    /* return the number of bytes disassembled.               */

static char *Mnemonics[256] =
{
  "NOP","LD BC,#h","LD (BC),A","INC BC","INC B","DEC B","LD B,*h","RLCA",
  "LD (#h),SP","ADD HL,BC","LD A,(BC)","DEC BC","INC C","DEC C","LD C,*h","RRCA",
  "STOP","LD DE,#h","LD (DE),A","INC DE","INC D","DEC D","LD D,*h","RLA",
  "JR @h","ADD HL,DE","LD A,(DE)","DEC DE","INC E","DEC E","LD E,*h","RRA",
  "JR NZ,@h","LD HL,#h","LD (HL+),A","INC HL","INC H","DEC H","LD H,*h","DAA",
  "JR Z,@h","ADD HL,HL","LD A,(HL+)","DEC HL","INC L","DEC L","LD L,*h","CPL",
  "JR NC,@h","LD SP,#h","LD (HL-),A","INC SP","INC (HL)","DEC (HL)","LD (HL),*h","SCF",
  "JR C,@h","ADD HL,SP","LD A,(HL-)","DEC SP","INC A","DEC A","LD A,*h","CCF",
  "LD B,B","LD B,C","LD B,D","LD B,E","LD B,H","LD B,L","LD B,(HL)","LD B,A",
  "LD C,B","LD C,C","LD C,D","LD C,E","LD C,H","LD C,L","LD C,(HL)","LD C,A",
  "LD D,B","LD D,C","LD D,D","LD D,E","LD D,H","LD D,L","LD D,(HL)","LD D,A",
  "LD E,B","LD E,C","LD E,D","LD E,E","LD E,H","LD E,L","LD E,(HL)","LD E,A",
  "LD H,B","LD H,C","LD H,D","LD H,E","LD H,H","LD H,L","LD H,(HL)","LD H,A",
  "LD L,B","LD L,C","LD L,D","LD L,E","LD L,H","LD L,L","LD L,(HL)","LD L,A",
  "LD (HL),B","LD (HL),C","LD (HL),D","LD (HL),E","LD (HL),H","LD (HL),L","HALT","LD (HL),A",
  "LD A,B","LD A,C","LD A,D","LD A,E","LD A,H","LD A,L","LD A,(HL)","LD A,A",
  "ADD B","ADD C","ADD D","ADD E","ADD H","ADD L","ADD (HL)","ADD A",
  "ADC B","ADC C","ADC D","ADC E","ADC H","ADC L","ADC (HL)","ADC A",
  "SUB B","SUB C","SUB D","SUB E","SUB H","SUB L","SUB (HL)","SUB A",
  "SBC B","SBC C","SBC D","SBC E","SBC H","SBC L","SBC (HL)","SBC A",
  "AND B","AND C","AND D","AND E","AND H","AND L","AND (HL)","AND A",
  "XOR B","XOR C","XOR D","XOR E","XOR H","XOR L","XOR (HL)","XOR A",
  "OR B","OR C","OR D","OR E","OR H","OR L","OR (HL)","OR A",
  "CP B","CP C","CP D","CP E","CP H","CP L","CP (HL)","CP A",
  "RET NZ","POP BC","JP NZ,#h","JP #h","CALL NZ,#h","PUSH BC","ADD *h","RST 00h",
  "RET Z","RET","JP Z,#h","PREFIX CBh","CALL Z,#h","CALL #h","ADC *h","RST 08h",
  "RET NC","POP DE","JP NC,#h","DB D3h","CALL NC,#h","PUSH DE","SUB *h","RST 10h",
  "RET C","RETI","JP C,#h","DB DBh","CALL C,#h","DB DDh","SBC *h","RST 18h",
  "LD (FF*h),A","POP HL","LD (FF00h+C),A","DB E3h","DB E4h","PUSH HL","AND *h","RST 20h",
  "ADD SP,@h","LD PC,HL","LD (#h),A","DB EBh","DB ECh","PREFIX EDh","XOR *h","RST 28h",
  "LD A,(FF*h)","POP AF","LD A,(FF00h+C)","DI","DB F4h","PUSH AF","OR *h","RST 30h",
  "LDHL SP,@h","LD SP,HL","LD A,(#h)","EI","DB FCh","DB FDh","CP *h","RST 38h"
};

static char *MnemonicsCB[256] =
{
  "RLC B","RLC C","RLC D","RLC E","RLC H","RLC L","RLC (HL)","RLC A",
  "RRC B","RRC C","RRC D","RRC E","RRC H","RRC L","RRC (HL)","RRC A",
  "RL B","RL C","RL D","RL E","RL H","RL L","RL (HL)","RL A",
  "RR B","RR C","RR D","RR E","RR H","RR L","RR (HL)","RR A",
  "SLA B","SLA C","SLA D","SLA E","SLA H","SLA L","SLA (HL)","SLA A",
  "SRA B","SRA C","SRA D","SRA E","SRA H","SRA L","SRA (HL)","SRA A",
  "SWAP B","SWAP C","SWAP D","SWAP E","SWAP H","SWAP L","SWAP (HL)","SWAP A",
  "SRL B","SRL C","SRL D","SRL E","SRL H","SRL L","SRL (HL)","SRL A",
  "BIT 0,B","BIT 0,C","BIT 0,D","BIT 0,E","BIT 0,H","BIT 0,L","BIT 0,(HL)","BIT 0,A",
  "BIT 1,B","BIT 1,C","BIT 1,D","BIT 1,E","BIT 1,H","BIT 1,L","BIT 1,(HL)","BIT 1,A",
  "BIT 2,B","BIT 2,C","BIT 2,D","BIT 2,E","BIT 2,H","BIT 2,L","BIT 2,(HL)","BIT 2,A",
  "BIT 3,B","BIT 3,C","BIT 3,D","BIT 3,E","BIT 3,H","BIT 3,L","BIT 3,(HL)","BIT 3,A",
  "BIT 4,B","BIT 4,C","BIT 4,D","BIT 4,E","BIT 4,H","BIT 4,L","BIT 4,(HL)","BIT 4,A",
  "BIT 5,B","BIT 5,C","BIT 5,D","BIT 5,E","BIT 5,H","BIT 5,L","BIT 5,(HL)","BIT 5,A",
  "BIT 6,B","BIT 6,C","BIT 6,D","BIT 6,E","BIT 6,H","BIT 6,L","BIT 6,(HL)","BIT 6,A",
  "BIT 7,B","BIT 7,C","BIT 7,D","BIT 7,E","BIT 7,H","BIT 7,L","BIT 7,(HL)","BIT 7,A",
  "RES 0,B","RES 0,C","RES 0,D","RES 0,E","RES 0,H","RES 0,L","RES 0,(HL)","RES 0,A",
  "RES 1,B","RES 1,C","RES 1,D","RES 1,E","RES 1,H","RES 1,L","RES 1,(HL)","RES 1,A",
  "RES 2,B","RES 2,C","RES 2,D","RES 2,E","RES 2,H","RES 2,L","RES 2,(HL)","RES 2,A",
  "RES 3,B","RES 3,C","RES 3,D","RES 3,E","RES 3,H","RES 3,L","RES 3,(HL)","RES 3,A",
  "RES 4,B","RES 4,C","RES 4,D","RES 4,E","RES 4,H","RES 4,L","RES 4,(HL)","RES 4,A",
  "RES 5,B","RES 5,C","RES 5,D","RES 5,E","RES 5,H","RES 5,L","RES 5,(HL)","RES 5,A",
  "RES 6,B","RES 6,C","RES 6,D","RES 6,E","RES 6,H","RES 6,L","RES 6,(HL)","RES 6,A",
  "RES 7,B","RES 7,C","RES 7,D","RES 7,E","RES 7,H","RES 7,L","RES 7,(HL)","RES 7,A",
  "SET 0,B","SET 0,C","SET 0,D","SET 0,E","SET 0,H","SET 0,L","SET 0,(HL)","SET 0,A",
  "SET 1,B","SET 1,C","SET 1,D","SET 1,E","SET 1,H","SET 1,L","SET 1,(HL)","SET 1,A",
  "SET 2,B","SET 2,C","SET 2,D","SET 2,E","SET 2,H","SET 2,L","SET 2,(HL)","SET 2,A",
  "SET 3,B","SET 3,C","SET 3,D","SET 3,E","SET 3,H","SET 3,L","SET 3,(HL)","SET 3,A",
  "SET 4,B","SET 4,C","SET 4,D","SET 4,E","SET 4,H","SET 4,L","SET 4,(HL)","SET 4,A",
  "SET 5,B","SET 5,C","SET 5,D","SET 5,E","SET 5,H","SET 5,L","SET 5,(HL)","SET 5,A",
  "SET 6,B","SET 6,C","SET 6,D","SET 6,E","SET 6,H","SET 6,L","SET 6,(HL)","SET 6,A",
  "SET 7,B","SET 7,C","SET 7,D","SET 7,E","SET 7,H","SET 7,L","SET 7,(HL)","SET 7,A"
};

/** DAsm() ****************************************************/
/** This function will disassemble a single command and      **/
/** return the number of bytes disassembled.                 **/
/**************************************************************/
int DAsm(char *S,byte *A)
{
  char H[10],*T,*P;
  byte *B,J;

  B=A;

  switch(*B)
  {
    case 0xCB: B++;T=MnemonicsCB[*B++];break;
    default:   T=Mnemonics[*B++];
  }

  if(P=strchr(T,'*'))
  {
    strncpy(S,T,P-T);S[P-T]='\0';
    sprintf(H,"%02X",*B++);
    strcat(S,H);strcat(S,P+1);
  }
  else
    if(P=strchr(T,'@'))
    {
      strncpy(S,T,P-T);S[P-T]='\0';
      strcat(S,*B&0x80? "-":"+");
      J=*B&0x80? 256-*B:*B;B++;
      sprintf(H,"%02X",J);
      strcat(S,H);strcat(S,P+1);
    }
    else
      if(P=strchr(T,'#'))
      {
        strncpy(S,T,P-T);S[P-T]='\0';
        sprintf(H,"%04X",B[0]+256*B[1]);
        strcat(S,H);strcat(S,P+1);
        B+=2;
      }
      else strcpy(S,T);

  return(B-A);
}

/** main() ****************************************************/
/** This is the main function from which execution starts.   **/
/**************************************************************/
int main(int argc,char *argv[])
{
  FILE *F;
  int N,I,J;
  byte Buf[512];
  char S[128],*P;

  PrintInfo=PrintHex=0;
  Counter=0L;

  for(N=1;(N<argc)&&(*argv[N]=='-');N++)
    switch(argv[N][1])
    {
      case 'o': sscanf(argv[N],"-o%lx",&Counter);
                Counter&=0xFFFFFFFFL;break;
      default:
        for(J=1;argv[N][J];J++)
          switch(argv[N][J])
          {
            case 'h': PrintHex=1;break;
            case 'i': PrintInfo=1;break;
            default:
              fprintf(stderr,"%s: Unknown option -%c\n",argv[0],argv[N][J]);
          }
    }

  if(N==argc)
  {
    fprintf(stderr,"GBDASM GameBoy Disassembler v.3.4 by Marat Fayzullin\n");
#ifdef ZLIB
    fprintf(stderr,"  This program will transparently uncompress GZIPped files.\n");
#endif
    fprintf(stderr,"Usage: %s [-hi] [-oOrigin] file\n",argv[0]);
    fprintf(stderr,"  -h - Print hexadecimal values\n");
    fprintf(stderr,"  -i - Print cartridge info\n");
    fprintf(stderr,"  -o - Count addresses from a given origin (hex)\n");
    return(1);
  }
    
  if(PrintInfo&&(F=fopen(argv[N],"rb")))
  {
    if(fread(Buf,1,512,F)==512)
    {
      printf(";--- Cartridge Information ------------------------\n");
      printf("; Name:\t\t%s\n",GB_Name(Buf));

      if(P=GB_Maker(Buf)) printf("; Producer:\t%s\n",P);
      else printf("; Producer:\t%04Xh\n",GB_MakerID(Buf));

      printf("; Version:\t%d\n",GB_Version(Buf));
      printf("; MBC Type:\t%s\n",GB_Type(Buf));
      printf("; ROM Size:\t%dkB\n",GB_ROMSize(Buf)/1024);
      printf("; RAM Size:\t%dkB\n",GB_RAMSize(Buf)/1024);
      printf("; CRC:\t\t%04Xh\n",GB_CRC(Buf));
      printf("; Complement:\t%02Xh\n",GB_CMP(Buf));
      printf("; Battery:\t%s\n",GB_Battery(Buf)? "YES":"NO");
      printf("; Timer:\t%s\n",GB_Timer(Buf)? "YES":"NO");
      printf("; Rumble:\t%s\n",GB_Rumble(Buf)? "YES":"NO");
      printf("; Japanese:\t%s\n",GB_Japanese(Buf)? "YES":"NO");
      printf("; SuperGB:\t%s\n",GB_SuperGB(Buf)? "YES":"NO");
      printf("; ColorGB:\t%s\n",GB_ColorGB(Buf)? "YES":"NO");
      printf(";--------------------------------------------------\n");
    }
    fclose(F);
  }

  if(!(F=fopen(argv[N],"rb")))
  { printf("\n%s: Can't open file %s\n",argv[0],argv[N]);return(1); }
  
  for(N=0;N+=fread(Buf+N,1,16-N,F);)
  {
    if(N<16) memset(Buf+N,0,16-N);
    I=DAsm(S,Buf);
    printf("%02X.%04lX:",(Counter>>14)&0xFF,Counter&0x3FFF);
    if(PrintHex)
    {
      for(J=0;J<I;J++) printf(" %02X",Buf[J]);
      if(I<3) printf("\t");
    }
    printf("\t%s\n",S);
    Counter+=I;
    N=N>I? N-I:0;
    for(J=0;J<N;J++) Buf[J]=Buf[J+I];
  }

  fclose(F);return(0);
}
