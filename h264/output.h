#pragma once

extern void allocOutput (cDecoder264* decoder);
extern void freeOutput (cDecoder264* decoder);

extern void directOutput (cDecoder264* decoder, sPicture* picture);
extern void writeStoredFrame (cDecoder264* decoder, sFrameStore* frameStore);
