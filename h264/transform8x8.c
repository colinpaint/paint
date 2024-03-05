//{{{  includes
#include "global.h"

#include "image.h"
#include "mbAccess.h"
#include "elements.h"
#include "transform8x8.h"
#include "transform.h"
#include "quant.h"
//}}}

//{{{
static void recon8x8 (int** m7, sPixel** mb_rec, sPixel** mpr, int max_imgpel_value, int ioff) {

  int* m_tr  = NULL;
  sPixel* m_rec = NULL;
  sPixel* m_prd = NULL;

  for (int j = 0; j < 8; j++) {
    m_tr = (*m7++) + ioff;
    m_rec = (*mb_rec++) + ioff;
    m_prd = (*mpr++) + ioff;

    *m_rec++ = (sPixel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8));
    *m_rec++ = (sPixel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8));
    *m_rec++ = (sPixel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8));
    *m_rec++ = (sPixel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8));
    *m_rec++ = (sPixel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8));
    *m_rec++ = (sPixel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8));
    *m_rec++ = (sPixel) iClip1(max_imgpel_value, (*m_prd++) + rshift_rnd_sf(*m_tr++, DQ_BITS_8));
    *m_rec   = (sPixel) iClip1(max_imgpel_value, (*m_prd  ) + rshift_rnd_sf(*m_tr  , DQ_BITS_8));
    }
  }
//}}}
//{{{
static void copy8x8 (sPixel** mb_rec, sPixel** mpr, int ioff) {

  for (int j = 0; j < 8; j++)
    memcpy((*mb_rec++) + ioff, (*mpr++) + ioff, 8 * sizeof(sPixel));
  }
//}}}
//{{{
static void recon8x8_lossless (int** m7, sPixel** mb_rec, sPixel** mpr, int max_imgpel_value, int ioff) {

  for (int  j = 0; j < 8; j++) {
    for (int i = ioff; i < ioff + 8; i++)
      (*mb_rec)[i] = (sPixel)lClip1 (max_imgpel_value, ((*m7)[i] + (long)(*mpr)[i]));
    mb_rec++;
    m7++;
    mpr++;
    }
  }
//}}}

//{{{
void itrans8x8 (sMacroblock *currMB, sColorPlane pl, int ioff, int joff) {

  sSlice* currSlice = currMB->p_Slice;
  int** m7     = currSlice->mb_rres[pl];

  if (currMB->is_lossless == TRUE)
    recon8x8_lossless(&m7[joff], &currSlice->mb_rec[pl][joff], &currSlice->mb_pred[pl][joff], currMB->vidParam->max_pel_value_comp[pl], ioff);
  else {
    inverse8x8(&m7[joff], &m7[joff], ioff);
    recon8x8  (&m7[joff], &currSlice->mb_rec[pl][joff], &currSlice->mb_pred[pl][joff], currMB->vidParam->max_pel_value_comp[pl], ioff);
   }
  }
//}}}
//{{{
void icopy8x8 (sMacroblock *currMB, sColorPlane pl, int ioff, int joff) {

  sSlice* currSlice = currMB->p_Slice;
  copy8x8 (&currSlice->mb_rec[pl][joff], &currSlice->mb_pred[pl][joff], ioff);
  }
//}}}
