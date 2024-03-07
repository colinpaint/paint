#pragma once

extern int mb_pred_intra4x4      (sMacroblock* curMb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture);
extern int mb_pred_intra16x16    (sMacroblock* curMb, eColorPlane curPlane, sPicture* picture);
extern int mb_pred_intra8x8      (sMacroblock* curMb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture);

extern int mb_pred_skip          (sMacroblock* curMb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture);
extern int mb_pred_sp_skip       (sMacroblock* curMb, eColorPlane curPlane, sPicture* picture);
extern int mb_pred_p_inter8x8    (sMacroblock* curMb, eColorPlane curPlane, sPicture* picture);
extern int mb_pred_p_inter16x16  (sMacroblock* curMb, eColorPlane curPlane, sPicture* picture);
extern int mb_pred_p_inter16x8   (sMacroblock* curMb, eColorPlane curPlane, sPicture* picture);
extern int mb_pred_p_inter8x16   (sMacroblock* curMb, eColorPlane curPlane, sPicture* picture);
extern int mb_pred_b_d4x4spatial (sMacroblock* curMb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture);
extern int mb_pred_b_d8x8spatial (sMacroblock* curMb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture);
extern int mb_pred_b_d4x4temporal(sMacroblock* curMb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture);
extern int mb_pred_b_d8x8temporal(sMacroblock* curMb, eColorPlane curPlane, sPixel** curPixel, sPicture* picture);
extern int mb_pred_b_inter8x8    (sMacroblock* curMb, eColorPlane curPlane, sPicture* picture);
extern int mb_pred_ipcm          (sMacroblock* curMb);
