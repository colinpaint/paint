//{{{  includes
#include "global.h"
#include "syntaxElement.h"
//}}}

//{{{
int setEcFlag (cDecoder264* decoder, int se) {

  switch (se) {
    case SE_HEADER :
      decoder->ecFlag[SE_HEADER] = EC_REQ;
    case SE_PTYPE :
      decoder->ecFlag[SE_PTYPE] = EC_REQ;
    case SE_MBTYPE :
      decoder->ecFlag[SE_MBTYPE] = EC_REQ;
    //{{{
    case SE_REFFRAME :
      decoder->ecFlag[SE_REFFRAME] = EC_REQ;
      decoder->ecFlag[SE_MVD] = EC_REQ; // set all motion vectors to zero length
      se = SE_CBP_INTER;      // conceal also Inter texture elements
      break;
    //}}}
    //{{{
    case SE_INTRAPREDMODE :
      decoder->ecFlag[SE_INTRAPREDMODE] = EC_REQ;
      se = SE_CBP_INTRA;      // conceal also Intra texture elements
      break;
    //}}}
    //{{{
    case SE_MVD :
      decoder->ecFlag[SE_MVD] = EC_REQ;
      se = SE_CBP_INTER;      // conceal also Inter texture elements
      break;
    //}}}
    //{{{
    default:
      break;
    //}}}
    }

  switch (se) {
    case SE_CBP_INTRA :
      decoder->ecFlag[SE_CBP_INTRA] = EC_REQ;
    case SE_LUM_DC_INTRA :
      decoder->ecFlag[SE_LUM_DC_INTRA] = EC_REQ;
    case SE_CHR_DC_INTRA :
      decoder->ecFlag[SE_CHR_DC_INTRA] = EC_REQ;
    case SE_LUM_AC_INTRA :
      decoder->ecFlag[SE_LUM_AC_INTRA] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTRA :
      decoder->ecFlag[SE_CHR_AC_INTRA] = EC_REQ;
      break;
    //}}}

    case SE_CBP_INTER :
      decoder->ecFlag[SE_CBP_INTER] = EC_REQ;
    case SE_LUM_DC_INTER :
      decoder->ecFlag[SE_LUM_DC_INTER] = EC_REQ;
    case SE_CHR_DC_INTER :
      decoder->ecFlag[SE_CHR_DC_INTER] = EC_REQ;
    case SE_LUM_AC_INTER :
      decoder->ecFlag[SE_LUM_AC_INTER] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTER :
      decoder->ecFlag[SE_CHR_AC_INTER] = EC_REQ;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTER :
      decoder->ecFlag[SE_DELTA_QUANT_INTER] = EC_REQ;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA :
      decoder->ecFlag[SE_DELTA_QUANT_INTRA] = EC_REQ;
      break;
    //}}}
    //{{{
    default:
      break;
    }
    //}}}

  return EC_REQ;
  }
//}}}
//{{{
void reset_ecFlags (cDecoder264* decoder) {

  for (int i = 0; i < SE_MAX_ELEMENTS; i++)
    decoder->ecFlag[i] = NO_EC;
  }
//}}}

//{{{
int get_concealed_element (cDecoder264* decoder, sSyntaxElement* se) {

  if (decoder->ecFlag[se->type] == NO_EC)
    return NO_EC;

  switch (se->type) {
    //{{{
    case SE_HEADER :
      se->len = 31;
      se->inf = 0; // Picture Header
      break;
    //}}}

    case SE_PTYPE : // inter_img_1
    case SE_MBTYPE : // set COPY_MB
    //{{{
    case SE_REFFRAME :
      se->len = 1;
      se->inf = 0;
      break;
    //}}}
    case SE_INTRAPREDMODE :
    //{{{
    case SE_MVD :
      se->len = 1;
      se->inf = 0;  // set vector to zero length
      break;
    //}}}
    //{{{
    case SE_CBP_INTRA :
      se->len = 5;
      se->inf = 0; // codenumber 3 <=> no CBP information for INTRA images
      break;
    //}}}
    case SE_LUM_DC_INTRA :
    case SE_CHR_DC_INTRA :
    case SE_LUM_AC_INTRA :
    //{{{
    case SE_CHR_AC_INTRA :
      se->len = 1;
      se->inf = 0;  // return EOB
      break;
    //}}}
    //{{{
    case SE_CBP_INTER :
      se->len = 1;
      se->inf = 0; // codenumber 1 <=> no CBP information for INTER images
      break;
    //}}}
    case SE_LUM_DC_INTER :
    case SE_CHR_DC_INTER :
    case SE_LUM_AC_INTER :
    //{{{
    case SE_CHR_AC_INTER :
      se->len = 1;
      se->inf = 0;  // return EOB
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTER:
      se->len = 1;
      se->inf = 0;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA:
      se->len = 1;
      se->inf = 0;
      break;
    //}}}
    //{{{
    default:
      break;
    //}}}
    }

  return EC_REQ;
  }
//}}}
