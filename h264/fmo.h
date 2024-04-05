#pragma once

int initFmo (cDecoder264* decoder, cSlice* slice);
int closeFmo (cDecoder264* decoder);

int FmoGetNumberOfSliceGroup (cDecoder264* decoder);
int FmoGetLastMBOfPicture (cDecoder264* decoder);
int FmoGetLastMBInSliceGroup (cDecoder264* decoder, int SliceGroup);
int FmoGetSliceGroupId (cDecoder264* decoder, int mb);
int FmoGetNextMBNr (cDecoder264* decoder, int CurrentMbNr);
