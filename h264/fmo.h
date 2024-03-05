#pragma once

extern int fmo_init (VideoParameters* pVid, Slice *pSlice);
extern int FmoFinit (VideoParameters* pVid);

extern int FmoGetNumberOfSliceGroup (VideoParameters* pVid);
extern int FmoGetLastMBOfPicture (VideoParameters* pVid);
extern int FmoGetLastMBInSliceGroup (VideoParameters* pVid, int SliceGroup);
extern int FmoGetSliceGroupId (VideoParameters* pVid, int mb);
extern int FmoGetNextMBNr (VideoParameters* pVid, int CurrentMbNr);
