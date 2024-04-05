#pragma once
#include "global.h"

void readCoef4x4cavlcNormal (sMacroBlock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);
void readCoef4x4cavlc444 (sMacroBlock* mb, int block_type, int i, int j, int levarr[16], int runarr[16], int *number_coefficients);

void setReadCompCoefCavlc (sMacroBlock* mb);
