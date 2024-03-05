#pragma once

extern int mb_pred_intra4x4      (sMacroblock* currMB, ColorPlane curr_plane, sPixel **currImg, sPicture *dec_picture);
extern int mb_pred_intra16x16    (sMacroblock* currMB, ColorPlane curr_plane, sPicture *dec_picture);
extern int mb_pred_intra8x8      (sMacroblock* currMB, ColorPlane curr_plane, sPixel **currImg, sPicture *dec_picture);

extern int mb_pred_skip          (sMacroblock* currMB, ColorPlane curr_plane, sPixel **currImg, sPicture *dec_picture);
extern int mb_pred_sp_skip       (sMacroblock* currMB, ColorPlane curr_plane, sPicture *dec_picture);
extern int mb_pred_p_inter8x8    (sMacroblock* currMB, ColorPlane curr_plane, sPicture *dec_picture);
extern int mb_pred_p_inter16x16  (sMacroblock* currMB, ColorPlane curr_plane, sPicture *dec_picture);
extern int mb_pred_p_inter16x8   (sMacroblock* currMB, ColorPlane curr_plane, sPicture *dec_picture);
extern int mb_pred_p_inter8x16   (sMacroblock* currMB, ColorPlane curr_plane, sPicture *dec_picture);
extern int mb_pred_b_d4x4spatial (sMacroblock* currMB, ColorPlane curr_plane, sPixel **currImg, sPicture *dec_picture);
extern int mb_pred_b_d8x8spatial (sMacroblock* currMB, ColorPlane curr_plane, sPixel **currImg, sPicture *dec_picture);
extern int mb_pred_b_d4x4temporal(sMacroblock* currMB, ColorPlane curr_plane, sPixel **currImg, sPicture *dec_picture);
extern int mb_pred_b_d8x8temporal(sMacroblock* currMB, ColorPlane curr_plane, sPixel **currImg, sPicture *dec_picture);
extern int mb_pred_b_inter8x8    (sMacroblock* currMB, ColorPlane curr_plane, sPicture *dec_picture);
extern int mb_pred_ipcm          (sMacroblock* currMB);
