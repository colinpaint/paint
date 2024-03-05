#pragma once

extern int fmo_init (sVidParam* vidParam, sSlice* pSlice);
extern int FmoFinit (sVidParam* vidParam);

extern int FmoGetNumberOfSliceGroup (sVidParam* vidParam);
extern int FmoGetLastMBOfPicture (sVidParam* vidParam);
extern int FmoGetLastMBInSliceGroup (sVidParam* vidParam, int SliceGroup);
extern int FmoGetSliceGroupId (sVidParam* vidParam, int mb);
extern int FmoGetNextMBNr (sVidParam* vidParam, int CurrentMbNr);
