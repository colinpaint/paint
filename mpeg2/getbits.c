//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "global.h"
//}}}

//{{{
void Initialize_Buffer() {

  ld->Incnt = 0;
  ld->Rdptr = ld->Rdbfr + 2048;
  ld->Rdmax = ld->Rdptr;

#ifdef VERIFY
  /*  only the verifier uses this particular bit counter
   *  Bitcnt keeps track of the current parser position with respect
   *  to the video elementary stream being decoded, regardless
   *  of whether or not it is wrapped within a systems layer stream
   */
  ld->Bitcnt = 0;
#endif

  ld->Bfr = 0;
  Flush_Buffer(0); /* fills valid data into bfr */
  }
//}}}
//{{{
void Fill_Buffer() {

  int Buffer_Level;

  Buffer_Level = read(ld->Infile,ld->Rdbfr,2048);
  ld->Rdptr = ld->Rdbfr;

  if (System_Stream_Flag)
    ld->Rdmax -= 2048;

  /* end of the bitstream file */
  if (Buffer_Level < 2048) {
    /* just to be safe */
    if (Buffer_Level < 0)
      Buffer_Level = 0;

    /* pad until the next to the next 32-bit word boundary */
    while (Buffer_Level & 3)
      ld->Rdbfr[Buffer_Level++] = 0;

  /* pad the buffer with sequence end codes */
    while (Buffer_Level < 2048) {
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE>>24;
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE>>16;
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE>>8;
      ld->Rdbfr[Buffer_Level++] = SEQUENCE_END_CODE&0xff;
      }
    }
  }
//}}}

//{{{
/* MPEG-1 system layer demultiplexer */
int Get_Byte() {

  while(ld->Rdptr >= ld->Rdbfr+2048) {
    read(ld->Infile,ld->Rdbfr,2048);
    ld->Rdptr -= 2048;
    ld->Rdmax -= 2048;
    }

  return *ld->Rdptr++;
  }
//}}}
//{{{
/* extract a 16-bit word from the bitstream buffer */
int Get_Word() {

  int Val = Get_Byte();
  return (Val<<8) | Get_Byte();
  }
//}}}

unsigned int Show_Bits (int N) { return ld->Bfr >> (32-N); }
unsigned int Get_Bits1() { return Get_Bits(1); }

//{{{
void Flush_Buffer (int N) {

  int Incnt;
  ld->Bfr <<= N;
  Incnt = ld->Incnt -= N;
  if (Incnt <= 24) {
    if (System_Stream_Flag && (ld->Rdptr >= ld->Rdmax-4)) {
      do {
        if (ld->Rdptr >= ld->Rdmax)
          Next_Packet();
        ld->Bfr |= Get_Byte() << (24 - Incnt);
        Incnt += 8;
        }
      while (Incnt <= 24);
      }
    else if (ld->Rdptr < ld->Rdbfr+2044) {
      do {
        ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
        Incnt += 8;
        }
      while (Incnt <= 24);
      }
    else {
      do {
        if (ld->Rdptr >= ld->Rdbfr+2048)
          Fill_Buffer();
        ld->Bfr |= *ld->Rdptr++ << (24 - Incnt);
        Incnt += 8;
        }
      while (Incnt <= 24);
      }
    ld->Incnt = Incnt;
    }
  }
//}}}
//{{{
unsigned int Get_Bits (int N) {

  unsigned int Val = Show_Bits(N);
  Flush_Buffer(N);
  return Val;
  }
//}}}
