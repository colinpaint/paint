#pragma once

extern void initOutBuffer (sVidParam* vidParam);
extern void freeOutBuffer (sVidParam* vidParam);

extern void writeStoredFrame (sVidParam* vidParam, sFrameStore* fs);
extern void directOutput (sVidParam* vidParam, sPicture* p);
