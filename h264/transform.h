#pragma once

extern void forward4x4 (int** block , int** tblock, int posY, int posX);
extern void inverse4x4 (int** tblock, int** block , int posY, int posX);

extern void forward8x8 (int** block , int** tblock, int posY, int posX);
extern void inverse8x8 (int** tblock, int** block , int posX);

extern void hadamard4x4 (int** block , int** tblock);
extern void ihadamard4x4 (int** tblock, int** block);

extern void hadamard4x2 (int** block , int** tblock);
extern void ihadamard4x2 (int** tblock, int** block);

extern void hadamard2x2 (int** block , int tblock[4]);
extern void ihadamard2x2 (int block[4], int tblock[4]);

extern void icopy8x8 (sMacroblock* mb, eColorPlane pl, int ioff, int joff);
extern void itrans8x8 (sMacroblock* mb, eColorPlane pl, int ioff, int joff);
