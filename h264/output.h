#pragma once

extern void allocOutput (sDecoder* decoder);
extern void freeOutput (sDecoder* decoder);

extern void directOutput (sDecoder* decoder, sPicture* p);
extern void writeStoredFrame (sDecoder* decoder, sFrameStore* frameStore);
