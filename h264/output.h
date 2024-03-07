#pragma once

extern void allocOutput (sVidParam* vidParam);
extern void freeOutput (sVidParam* vidParam);

extern void directOutput (sVidParam* vidParam, sPicture* p);
extern void writeStoredFrame (sVidParam* vidParam, sFrameStore* frameStore);
