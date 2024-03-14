//{{{  includes
#include "global.h"
#include "memory.h"

#include "biariDecode.h"
//}}}

#define B_BITS    10      // Number of bits to represent the whole coding interval
#define HALF      0x01FE  //(1 << (B_BITS-1)) - 2
#define QUARTER   0x0100  //(1 << (B_BITS-2))

//{{{
sDecodingEnv* aridecoCreateDecodingEnv() {

  sDecodingEnv* dep = calloc (1,sizeof(sDecodingEnv));
  if (!dep)
    no_mem_exit ("aridecoCreateDecodingEnv: dep");

  return dep;
  }
//}}}

//{{{
void aridecoDeleteDecodingEnv (sDecodingEnv* dep) {

  if (dep == NULL)
    error ("Error freeing dep (NULL pointer)");
  else
    free (dep);
  }
//}}}
//{{{
void aridecoDoneDecoding (sDecodingEnv* dep) {
  (*dep->Dcodestrm_len)++;
  }
//}}}

//{{{
static inline unsigned int getbyte (sDecodingEnv* dep) {
  return dep->Dcodestrm[(*dep->Dcodestrm_len)++];
  }
//}}}
//{{{
static inline unsigned int getword (sDecodingEnv* dep) {

  int* len = dep->Dcodestrm_len;
  byte* p_code_strm = &dep->Dcodestrm[*len];
  *len += 2;

  return (*p_code_strm<<8) | *(p_code_strm + 1);
  }
//}}}
//{{{
void aridecoStartDecoding (sDecodingEnv* dep, unsigned char* code_buffer, int firstbyte, int* codeLen) {

  dep->Dcodestrm = code_buffer;
  dep->Dcodestrm_len = codeLen;
  *dep->Dcodestrm_len = firstbyte;

  dep->Dvalue = getbyte(dep);
  dep->Dvalue = (dep->Dvalue << 16) | getword(dep); // lookahead of 2 bytes: always make sure that s buffer
                                                    // contains 2 more bytes than actual s
  dep->DbitsLeft = 15;
  dep->Drange = HALF;
  }
//}}}
//{{{
int aridecoBitsRead (sDecodingEnv* dep) {
  return ((*dep->Dcodestrm_len) << 3) - dep->DbitsLeft;
  }
//}}}

//{{{
unsigned int biarDecodeSymbol (sDecodingEnv* dep, sBiContextType* bi_ct) {

  unsigned int bit = bi_ct->MPS;
  unsigned int* value = &dep->Dvalue;
  unsigned int* range = &dep->Drange;

  uint16* state = &bi_ct->state;
  unsigned int rLPS = rLPS_table_64x4[*state][(*range >> 6) & 0x03];
  int* DbitsLeft = &dep->DbitsLeft;

  *range -= rLPS;
  if (*value < (*range << *DbitsLeft)) {
    // MPS
    *state = AC_next_state_MPS_64[*state]; // next state
    if ( *range >= QUARTER )
      return (bit);
    else {
      *range <<= 1;
      (*DbitsLeft)--;
      }
    }
  else {
    // LPS
    int renorm = renorm_table_32[(rLPS >> 3) & 0x1F];
    *value -= (*range << *DbitsLeft);
    *range = (rLPS << renorm);
    (*DbitsLeft) -= renorm;
    bit ^= 0x01;
    if (!(*state))          // switch meaning of MPS if necessary
      bi_ct->MPS ^= 0x01;
    *state = AC_next_state_LPS_64[*state]; // next state
    }

  if (*DbitsLeft > 0 )
    return (bit);
  else {
    *value <<= 16;
    *value |=  getword (dep);    // lookahead of 2 bytes: always make sure that s buffer
    // contains 2 more bytes than actual s
    (*DbitsLeft) += 16;
    return bit;
    }
  }
//}}}
//{{{
unsigned int biariDecodeSymbolEqProb (sDecodingEnv* dep) {

  int tmp_value;
  unsigned int* value = &dep->Dvalue;

  int* DbitsLeft = &dep->DbitsLeft;
  if (--(*DbitsLeft) == 0) {
    // lookahead of 2 bytes: always make sure that s buffer
    // contains 2 more bytes than actual s
    *value = (*value << 16) | getword( dep );
    *DbitsLeft = 16;
    }

  tmp_value = *value - (dep->Drange << *DbitsLeft);
  if (tmp_value < 0)
    return 0;
  else {
    *value = tmp_value;
    return 1;
    }
  }
//}}}
//{{{
unsigned int biariDecodeFinal (sDecodingEnv* dep) {

  unsigned int range  = dep->Drange - 2;

  int value = dep->Dvalue;
  value -= (range << dep->DbitsLeft);
  if (value < 0) {
    if (range >= QUARTER ) {
      dep->Drange = range;
      return 0;
      }
    else {
      dep->Drange = (range << 1);
      if (--(dep->DbitsLeft) > 0)
        return 0;
      else {
        dep->Dvalue = (dep->Dvalue << 16) | getword (dep); // lookahead of 2 bytes: always make sure that s buffer
                                                           // contains 2 more bytes than actual s
        dep->DbitsLeft = 16;
        return 0;
        }
      }
    }
  else
    return 1;
  }
//}}}

//{{{
void biariInitContext (int qp, sBiContextType* ctx, const char* ini) {

  int pstate = ((ini[0] * qp) >> 4) + ini[1];

  if (pstate >= 64) {
    pstate = imin (126, pstate);
    ctx->state = (uint16)(pstate - 64);
    ctx->MPS = 1;
    }
  else {
    pstate = imax(1, pstate);
    ctx->state = (uint16)(63 - pstate);
    ctx->MPS = 0;
    }
  }
//}}}
