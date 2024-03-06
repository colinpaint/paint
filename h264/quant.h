#pragma once

extern void allocQuant (sCodingParam* codingParam);
extern void freeQuant (sCodingParam* codingParam);

extern void assignQuantParams (sSlice* currslice);
extern void CalculateQuant4x4Param (sSlice* currslice);
extern void CalculateQuant8x8Param (sSlice* currslice);
