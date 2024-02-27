#pragma once
#include "global.h"

#include "io_video.h"

extern int ParseSizeFromString           (VideoDataFile *input_file, int *xlen, int *ylen, double *fps);
extern void ParseFrameNoFormatFromString (VideoDataFile *input_file);
extern void OpenFrameFile                (VideoDataFile *input_file, int FrameNumberInFile);
extern void OpenFiles                    (VideoDataFile *input_file);
extern void CloseFiles                   (VideoDataFile *input_file);
extern VideoFileType ParseVideoType      (VideoDataFile *input_file);
