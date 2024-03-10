#pragma once

extern int initFmo (sDecoder* decoder, sSlice* pSlice);
extern int closeFmo (sDecoder* decoder);

extern int FmoGetNumberOfSliceGroup (sDecoder* decoder);
extern int FmoGetLastMBOfPicture (sDecoder* decoder);
extern int FmoGetLastMBInSliceGroup (sDecoder* decoder, int SliceGroup);
extern int FmoGetSliceGroupId (sDecoder* decoder, int mb);
extern int FmoGetNextMBNr (sDecoder* decoder, int CurrentMbNr);
