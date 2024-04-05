#pragma once

void forward4x4 (int** block , int** tblock, int posY, int posX);
void inverse4x4 (int** tblock, int** block , int posY, int posX);

void forward8x8 (int** block , int** tblock, int posY, int posX);
void inverse8x8 (int** tblock, int** block , int posX);

void hadamard4x4 (int** block , int** tblock);
void ihadamard4x4 (int** tblock, int** block);

void hadamard4x2 (int** block , int** tblock);
void ihadamard4x2 (int** tblock, int** block);

void hadamard2x2 (int** block , int tblock[4]);
void ihadamard2x2 (int block[4], int tblock[4]);

void icopy8x8 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
void itrans8x8 (sMacroBlock* mb, eColorPlane plane, int ioff, int joff);
