#pragma once

extern int initFmo (sDecoder* vidParam, sSlice* pSlice);
extern int FmoFinit (sDecoder* vidParam);

extern int FmoGetNumberOfSliceGroup (sDecoder* vidParam);
extern int FmoGetLastMBOfPicture (sDecoder* vidParam);
extern int FmoGetLastMBInSliceGroup (sDecoder* vidParam, int SliceGroup);
extern int FmoGetSliceGroupId (sDecoder* vidParam, int mb);
extern int FmoGetNextMBNr (sDecoder* vidParam, int CurrentMbNr);
