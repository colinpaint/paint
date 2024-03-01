//{{{
/*!
 ***********************************************************************
 * \file errorconcealment.c
 *
 * \brief
 *    Implements error concealment scheme for H.264 decoder
 *
 * \date
 *    6.10.2000
 *
 * \version
 *    1.0
 *
 * \note
 *    This simple error concealment implemented in this decoder uses
 *    the existing dependencies of syntax elements.
 *    In case that an element is detected as false this elements and all
 *    dependend elements are marked as elements to conceal in the p_Vid->ec_flag[]
 *    array. If the decoder requests a new element by the function
 *    readSyntaxElement_xxxx() this array is checked first if an error concealment has
 *    to be applied on this element.
 *    In case that an error occured a concealed element is given to the
 *    decoding function in macroblock().
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Sebastian Purreiter   <sebastian.purreiter@mch.siemens.de>
 ***********************************************************************
 */
//}}}
//{{{  includes
#include "global.h"
#include "elements.h"
//}}}

//{{{
int set_ec_flag (VideoParameters* p_Vid, int se) {

  switch (se) {
    case SE_HEADER :
      p_Vid->ec_flag[SE_HEADER] = EC_REQ;
    case SE_PTYPE :
      p_Vid->ec_flag[SE_PTYPE] = EC_REQ;
    case SE_MBTYPE :
      p_Vid->ec_flag[SE_MBTYPE] = EC_REQ;
    //{{{
    case SE_REFFRAME :
      p_Vid->ec_flag[SE_REFFRAME] = EC_REQ;
      p_Vid->ec_flag[SE_MVD] = EC_REQ; // set all motion vectors to zero length
      se = SE_CBP_INTER;      // conceal also Inter texture elements
      break;
    //}}}
    //{{{
    case SE_INTRAPREDMODE :
      p_Vid->ec_flag[SE_INTRAPREDMODE] = EC_REQ;
      se = SE_CBP_INTRA;      // conceal also Intra texture elements
      break;
    //}}}
    //{{{
    case SE_MVD :
      p_Vid->ec_flag[SE_MVD] = EC_REQ;
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
      p_Vid->ec_flag[SE_CBP_INTRA] = EC_REQ;
    case SE_LUM_DC_INTRA :
      p_Vid->ec_flag[SE_LUM_DC_INTRA] = EC_REQ;
    case SE_CHR_DC_INTRA :
      p_Vid->ec_flag[SE_CHR_DC_INTRA] = EC_REQ;
    case SE_LUM_AC_INTRA :
      p_Vid->ec_flag[SE_LUM_AC_INTRA] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTRA :
      p_Vid->ec_flag[SE_CHR_AC_INTRA] = EC_REQ;
      break;
    //}}}

    case SE_CBP_INTER :
      p_Vid->ec_flag[SE_CBP_INTER] = EC_REQ;
    case SE_LUM_DC_INTER :
      p_Vid->ec_flag[SE_LUM_DC_INTER] = EC_REQ;
    case SE_CHR_DC_INTER :
      p_Vid->ec_flag[SE_CHR_DC_INTER] = EC_REQ;
    case SE_LUM_AC_INTER :
      p_Vid->ec_flag[SE_LUM_AC_INTER] = EC_REQ;
    //{{{
    case SE_CHR_AC_INTER :
      p_Vid->ec_flag[SE_CHR_AC_INTER] = EC_REQ;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTER :
      p_Vid->ec_flag[SE_DELTA_QUANT_INTER] = EC_REQ;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA :
      p_Vid->ec_flag[SE_DELTA_QUANT_INTRA] = EC_REQ;
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
void reset_ec_flags (VideoParameters* p_Vid) {

  for (int i = 0; i < SE_MAX_ELEMENTS; i++)
    p_Vid->ec_flag[i] = NO_EC;
  }
//}}}

//{{{
int get_concealed_element (VideoParameters* p_Vid, SyntaxElement* sym) {

  if (p_Vid->ec_flag[sym->type] == NO_EC)
    return NO_EC;

  switch (sym->type) {
    //{{{
    case SE_HEADER :
      sym->len = 31;
      sym->inf = 0; // Picture Header
      break;
    //}}}

    case SE_PTYPE : // inter_img_1
    case SE_MBTYPE : // set COPY_MB
    //{{{
    case SE_REFFRAME :
      sym->len = 1;
      sym->inf = 0;
      break;
    //}}}
    case SE_INTRAPREDMODE :
    //{{{
    case SE_MVD :
      sym->len = 1;
      sym->inf = 0;  // set vector to zero length
      break;
    //}}}
    //{{{
    case SE_CBP_INTRA :
      sym->len = 5;
      sym->inf = 0; // codenumber 3 <=> no CBP information for INTRA images
      break;
    //}}}
    case SE_LUM_DC_INTRA :
    case SE_CHR_DC_INTRA :
    case SE_LUM_AC_INTRA :
    //{{{
    case SE_CHR_AC_INTRA :
      sym->len = 1;
      sym->inf = 0;  // return EOB
      break;
    //}}}
    //{{{
    case SE_CBP_INTER :
      sym->len = 1;
      sym->inf = 0; // codenumber 1 <=> no CBP information for INTER images
      break;
    //}}}
    case SE_LUM_DC_INTER :
    case SE_CHR_DC_INTER :
    case SE_LUM_AC_INTER :
    //{{{
    case SE_CHR_AC_INTER :
      sym->len = 1;
      sym->inf = 0;  // return EOB
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTER:
      sym->len = 1;
      sym->inf = 0;
      break;
    //}}}
    //{{{
    case SE_DELTA_QUANT_INTRA:
      sym->len = 1;
      sym->inf = 0;
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
