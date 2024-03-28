#pragma once

extern int mbPredIntra4x4      (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mbPredIntra16x16    (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mbPredIntra8x8      (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);

extern int mbPredSkip          (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mbPredSpSkip       (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mbPredPinter8x8    (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mbPredPinter16x16  (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mbPredPinter16x8   (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mbPredPinter8x16   (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mbPredBd4x4spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mbPredBd8x8spatial (sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mbPredBd4x4temporal(sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mbPredBd8x8temporal(sMacroBlock* mb, eColorPlane plane, sPixel** pixel, sPicture* picture);
extern int mbPredBinter8x8    (sMacroBlock* mb, eColorPlane plane, sPicture* picture);
extern int mbPredIpcm          (sMacroBlock* mb);
