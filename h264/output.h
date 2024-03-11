#pragma once

extern void allocOutput (sDecoder* decoder);
extern void freeOutput (sDecoder* decoder);

extern void directOutput (sDecoder* decoder, sPicture* picture);
extern void writeStoredFrame (sDecoder* decoder, sFrameStore* frameStore);
