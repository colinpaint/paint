//{{{  includes
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "config.h"
#include "global.h"
//}}}

//{{{
void Write_Frame (unsigned char *src[], int frame) {

  if (progressive_sequence || progressive_frame || Frame_Store_Flag) {
    /* progressive */
    //sprintf(outname,Output_Picture_Filename,frame,'f');
    //store_one(outname,src,0,Coded_Picture_Width,vertical_size);
    }
  else {
    /* interlaced */
    //sprintf(outname,Output_Picture_Filename,frame,'a');
    //store_one(outname,src,0,Coded_Picture_Width<<1,vertical_size>>1);

    //sprintf(outname,Output_Picture_Filename,frame,'b');
    //store_one(outname,src, Coded_Picture_Width,Coded_Picture_Width<<1,vertical_size>>1);
    }
  }
//}}}
