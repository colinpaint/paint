#pragma once
class cDecoder;
class cSlice;

void processSei (uint8_t* payload, int naluLen, cDecoder264* decoder, cSlice* slice);
