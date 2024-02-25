//{{{
/*!
 ***************************************************************************
 *
 * \file transform8x8.h
 *
 * \brief
 *    prototypes of 8x8 transform functions
 *
 * \date
 *    9. October 2003
 *
 * \author
 *    Main contributors (see contributors.h for copyright, address and affiliation details)
 *    - Yuri Vatis
 **************************************************************************/
//}}}
#pragma once

extern void itrans8x8   (Macroblock *currMB, ColorPlane pl, int ioff, int joff);
extern void icopy8x8    (Macroblock *currMB, ColorPlane pl, int ioff, int joff);
