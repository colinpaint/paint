#pragma once
class cDecoder;
class cFrameStore;
struct sPicture;

void allocOutput (cDecoder264* decoder);
void freeOutput (cDecoder264* decoder);

void directOutput (cDecoder264* decoder, sPicture* picture);
void writeStoredFrame (cDecoder264* decoder, cFrameStore* frameStore);
