#pragma once

extern int initFmo (cDecoder264* decoder, sSlice* slice);
extern int closeFmo (cDecoder264* decoder);

extern int FmoGetNumberOfSliceGroup (cDecoder264* decoder);
extern int FmoGetLastMBOfPicture (cDecoder264* decoder);
extern int FmoGetLastMBInSliceGroup (cDecoder264* decoder, int SliceGroup);
extern int FmoGetSliceGroupId (cDecoder264* decoder, int mb);
extern int FmoGetNextMBNr (cDecoder264* decoder, int CurrentMbNr);
