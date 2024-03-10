#pragma once

extern void allocOutput (sDecoder* vidParam);
extern void freeOutput (sDecoder* vidParam);

extern void directOutput (sDecoder* vidParam, sPicture* p);
extern void writeStoredFrame (sDecoder* vidParam, sFrameStore* frameStore);
