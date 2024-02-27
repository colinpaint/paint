#pragma once

extern int fmo_init (VideoParameters *p_Vid, Slice *pSlice);
extern int FmoFinit (VideoParameters *p_Vid);

extern int FmoGetNumberOfSliceGroup (VideoParameters *p_Vid);
extern int FmoGetLastMBOfPicture (VideoParameters *p_Vid);
extern int FmoGetLastMBInSliceGroup (VideoParameters *p_Vid, int SliceGroup);
extern int FmoGetSliceGroupId (VideoParameters *p_Vid, int mb);
extern int FmoGetNextMBNr (VideoParameters *p_Vid, int CurrentMbNr);
