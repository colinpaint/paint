#pragma once

extern int mb_pred_intra4x4      (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mb_pred_intra16x16    (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mb_pred_intra8x8      (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);

extern int mb_pred_skip          (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mb_pred_sp_skip       (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mb_pred_p_inter8x8    (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mb_pred_p_inter16x16  (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mb_pred_p_inter16x8   (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mb_pred_p_inter8x16   (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mb_pred_b_d4x4spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mb_pred_b_d8x8spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mb_pred_b_d4x4temporal(sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mb_pred_b_d8x8temporal(sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mb_pred_b_inter8x8    (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mb_pred_ipcm          (sMacroBlock* mb);
