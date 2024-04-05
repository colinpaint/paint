#pragma once

void allocOutput (cDecoder264* decoder);
void freeOutput (cDecoder264* decoder);

void directOutput (cDecoder264* decoder, sPicture* picture);
void writeStoredFrame (cDecoder264* decoder, sFrameStore* frameStore);
