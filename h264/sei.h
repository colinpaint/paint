#pragma once
class cDecoder;
class cSlice;

void processSei (uint8_t* naluPayload, int naluPayloadLen, cDecoder264* decoder, cSlice* slice);
