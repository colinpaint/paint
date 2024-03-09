 //{{{  includes
 #define _CRT_SECURE_NO_WARNINGS

 #include <stdio.h>
 #include <stdlib.h>
 #include <ctype.h>
 #include <fcntl.h>

 #include "global.h"
 //}}}

//{{{
char Version[] ="mpeg2decode V1.2a, 96/07/19"
;
//}}}
//{{{
char Author[] ="(C) 1996, MPEG Software Simulation Group"
;
//}}}

//{{{
static void Clear_Options() {

  Verbose_Flag = 0;
  Output_Type = 0;
  Output_Picture_Filename = " ";
  hiQdither  = 0;
  Output_Type = 0;
  Frame_Store_Flag = 0;
  Spatial_Flag = 0;
  Lower_Layer_Picture_Filename = " ";
  Reference_IDCT_Flag = 0;
  Trace_Flag = 0;
  Quiet_Flag = 0;
  Ersatz_Flag = 0;
  Substitute_Picture_Filename  = " ";
  Enhancement_Layer_Bitstream_Filename = " ";
  Big_Picture_Flag = 0;
  Main_Bitstream_Flag = 0;
  Main_Bitstream_Filename = " ";
  Verify_Flag = 0;
  Stats_Flag  = 0;
  User_Data_Flag = 0;
  }
//}}}
//{{{
static void Print_Options() {

  printf("Verbose_Flag                         = %d\n", Verbose_Flag);
  printf("Output_Type                          = %d\n", Output_Type);
  printf("Output_Picture_Filename              = %s\n", Output_Picture_Filename);
  printf("hiQdither                            = %d\n", hiQdither);
  printf("Output_Type                          = %d\n", Output_Type);
  printf("Frame_Store_Flag                     = %d\n", Frame_Store_Flag);
  printf("Spatial_Flag                         = %d\n", Spatial_Flag);
  printf("Lower_Layer_Picture_Filename         = %s\n", Lower_Layer_Picture_Filename);
  printf("Reference_IDCT_Flag                  = %d\n", Reference_IDCT_Flag);
  printf("Trace_Flag                           = %d\n", Trace_Flag);
  printf("Quiet_Flag                           = %d\n", Quiet_Flag);
  printf("Ersatz_Flag                          = %d\n", Ersatz_Flag);
  printf("Substitute_Picture_Filename          = %s\n", Substitute_Picture_Filename);
  printf("Enhancement_Layer_Bitstream_Filename = %s\n", Enhancement_Layer_Bitstream_Filename);
  printf("Big_Picture_Flag                     = %d\n", Big_Picture_Flag);
  printf("Main_Bitstream_Flag                  = %d\n", Main_Bitstream_Flag);
  printf("Main_Bitstream_Filename              = %s\n", Main_Bitstream_Filename);
  printf("Verify_Flag                          = %d\n", Verify_Flag);
  printf("Stats_Flag                           = %d\n", Stats_Flag);
  printf("User_Data_Flag                       = %d\n", User_Data_Flag);
  }
//}}}
//{{{
/* option processing */
static void Process_Options (int argc, char *argv[]) {

  int i, LastArg, NextArg;

  /* at least one argument should be present */
  if (argc<2) {
    printf("\n%s, %s\n",Version,Author);
    printf("Usage:  mpeg2decode {options}\n\
            Options: -b  file  main bitstream (base or spatial enhancement layer)\n\
                     -cn file  conformance report (n: level)\n\
                     -e  file  enhancement layer bitstream (SNR or Data Partitioning)\n\
                     -f        store/display interlaced video in frame format\n\
                     -g        concatenated file format for substitution method (-x)\n\
                     -in file  information & statistics report  (n: level)\n\
                     -l  file  file name pattern for lower layer sequence\n\
                               (for spatial scalability)\n\
                     -on file  output format (0:YUV 1:SIF 2:TGA 3:PPM 4:X11 5:X11HiQ)\n\
                     -q        disable warnings to stderr\n\
                     -r        use double precision reference IDCT\n\
                     -t        enable low level tracing to stdout\n\
                     -u  file  print user_data to stdio or file\n\
                     -vn       verbose output (n: level)\n\
                     -x  file  filename pattern of picture substitution sequence\n\n\
            File patterns:  for sequential filenames, \"printf\" style, e.g. rec%%d\n\
                             or rec%%d%%c for fieldwise storage\n\
            Levels:        0:none 1:sequence 2:picture 3:slice 4:macroblock 5:block\n\n\
            Example:       mpeg2decode -b bitstream.mpg -f -r -o0 rec%%d\n\
                     \n");
    exit(0);
    }


  Output_Type = -1;
  i = 1;

  /* command-line options are proceeded by '-' */
  while (i < argc) {
    /* check if this is the last argument */
    LastArg = ((argc-i)==1);

    /* parse ahead to see if another flag immediately follows current
       argument (this is used to tell if a filename is missing) */
    if(!LastArg)
      NextArg = (argv[i+1][0]=='-');
    else
      NextArg = 0;

    /* second character, [1], after '-' is the switch */
    if (argv[i][0]=='-') {
      switch(toupper(argv[i][1])) {
        //{{{
        case 'B':
          Main_Bitstream_Flag = 1;

          if(NextArg || LastArg)
            printf("ERROR: -b must be followed the main bitstream filename\n");
          else
            Main_Bitstream_Filename = argv[++i];
          break;


          if(NextArg || LastArg) {
            printf("ERROR: -e must be followed by filename\n");
            exit(ERROR);
            }
          else
            Enhancement_Layer_Bitstream_Filename = argv[++i];
          break;
        //}}}
        //{{{
        case 'F':
          Frame_Store_Flag = 1;
          break;
        //}}}
        //{{{
        case 'G':
          Big_Picture_Flag = 1;
          break;
        //}}}
        //{{{
        case 'L':  /* spatial scalability flag */
          Spatial_Flag = 1;
          if (NextArg || LastArg) {
            printf("ERROR: -l must be followed by filename\n");
            exit(ERROR);
            }
          else
           Lower_Layer_Picture_Filename = argv[++i];
          break;
        //}}}
        //{{{
        case 'O':
          Output_Type = atoi(&argv[i][2]);
          if((Output_Type==4) || (Output_Type==5))
            Output_Picture_Filename = "";  /* no need of filename */
          else if(NextArg || LastArg) {
            printf("ERROR: -o must be followed by filename\n");
            exit(ERROR);
            }
          else
          /* filename is separated by space, so it becomes the next argument */
            Output_Picture_Filename = argv[++i];
          break;
        //}}}
        //{{{
        case 'Q':
          Quiet_Flag = 1;
          break;
        //}}}
        //{{{
        case 'R':
          Reference_IDCT_Flag = 1;
          break;
        //}}}
        //{{{
        case 'X':
          Ersatz_Flag = 1;
          if (NextArg || LastArg) {
            printf("ERROR: -x must be followed by filename\n");
            exit(ERROR);
            }
           else
            Substitute_Picture_Filename = argv[++i];
          break;
        //}}}
        //{{{
        default:
          fprintf(stderr,"undefined option -%c ignored. Exiting program\n",
            argv[i][1]);
          exit(ERROR);
        //}}}
        } /* switch() */
      } /* if argv[i][0] == '-' */

    i++;
    } /* while() */

    if (Main_Bitstream_Flag != 1)
      printf ("There must be a main bitstream specified (-b filename)\n");

    /* force display process to show frame pictures */
    if ((Output_Type == 4 || Output_Type == 5) && Frame_Store_Flag)
      Display_Progressive_Flag = 1;
    else
      Display_Progressive_Flag = 0;

    /* no output type specified */
    if (Output_Type == -1) {
      Output_Type = 9;
      Output_Picture_Filename = "";
    }
  }
  //}}}

//{{{
static void Initialize_Decoder() {

  /* Clip table */
  if (!(Clip = (unsigned char *)malloc(1024)))
    Error ("Clip[] malloc failed\n");

  Clip += 384;
  for (int i = -384; i < 640; i++)
    Clip[i] = (i<0) ? 0 : ((i>255) ? 255 : i);

  /* IDCT */
  Initialize_Fast_IDCT();
  }
//}}}
//{{{
static int Headers() {

  int ret;
  ld = &base;
  ret = Get_Hdr();
  return ret;
  }
//}}}
//{{{
static void Initialize_Sequence() {

  int cc, size;
  static int Table_6_20[3] = {6,8,12};

  /* force MPEG-1 parameters for proper decoder behavior */
  /* see ISO/IEC 13818-2 section D.9.14 */
  if (!base.MPEG2_Flag) {
    progressive_sequence = 1;
    progressive_frame = 1;
    picture_structure = FRAME_PICTURE;
    frame_pred_frame_dct = 1;
    chroma_format = CHROMA420;
    matrix_coefficients = 5;
    }

  /* round to nearest multiple of coded macroblocks */
  /* ISO/IEC 13818-2 section 6.3.3 sequence_header() */
  mb_width = (horizontal_size+15)/16;
  mb_height = (base.MPEG2_Flag && !progressive_sequence) ? 2*((vertical_size+31)/32)
                                        : (vertical_size+15)/16;
  Coded_Picture_Width = 16*mb_width;
  Coded_Picture_Height = 16*mb_height;

  /* ISO/IEC 13818-2 sections 6.1.1.8, 6.1.1.9, and 6.1.1.10 */
  Chroma_Width = (chroma_format==CHROMA444) ? Coded_Picture_Width : Coded_Picture_Width>>1;
  Chroma_Height = (chroma_format!=CHROMA420) ? Coded_Picture_Height : Coded_Picture_Height>>1;

  /* derived based on Table 6-20 in ISO/IEC 13818-2 section 6.3.17 */
  block_count = Table_6_20[chroma_format-1];

  for (cc = 0; cc<3; cc++) {
    if (cc == 0)
      size = Coded_Picture_Width * Coded_Picture_Height;
    else
      size = Chroma_Width * Chroma_Height;

    if (!(backward_reference_frame[cc] = (unsigned char *)malloc(size)))
      Error ("backward_reference_frame[] malloc failed\n");

    if (!(forward_reference_frame[cc] = (unsigned char *)malloc(size)))
      Error ("forward_reference_frame[] malloc failed\n");
    }
  }
//}}}
//{{{
static void Deinitialize_Sequence() {

  base.MPEG2_Flag=0;
  for (int i = 0; i < 3; i++) {
    free (backward_reference_frame[i]);
    free (forward_reference_frame[i]);
    }
  }
//}}}
//{{{
static int video_sequence (int* Bitstream_Framenumber) {

  int Return_Value;
  int Bitstream_Framenum = *Bitstream_Framenumber;
  int Sequence_Framenum = 0;

  Initialize_Sequence();

  /* decode picture whose header has already been parsed in Decode_Bitstream() */
  Decode_Picture (Bitstream_Framenum, Sequence_Framenum);

  /* update picture numbers */
  if (!Second_Field) {
    Bitstream_Framenum++;
    Sequence_Framenum++;
   }

  /* loop through the rest of the pictures in the sequence */
  while ((Return_Value = Headers())) {
    Decode_Picture(Bitstream_Framenum, Sequence_Framenum);
    if (!Second_Field) {
      Bitstream_Framenum++;
      Sequence_Framenum++;
      }
    }

  /* put last frame */
  if (Sequence_Framenum!=0)
    Output_Last_Frame_of_Sequence (Bitstream_Framenum);

  Deinitialize_Sequence();

  *Bitstream_Framenumber = Bitstream_Framenum;

  return(Return_Value);
  }
//}}}
//{{{
static int Decode_Bitstream() {

  int ret;
  int Bitstream_Framenum = 0;

  for(;;) {
    ret = Headers();
    if (ret==1)
      ret = video_sequence (&Bitstream_Framenum);
    else
      return (ret);
    }
  }
//}}}

//{{{
void Error (char *text) {

  fprintf(stderr,text);
  exit(1);
  }

//}}}
//{{{
void Print_Bits (int code, int bits, int len) {

  for (int i = 0; i < len; i++)
    printf ("%d", (code >> (bits-1-i))&1);
  }
//}}}
//{{{
int main (int argc, char *argv[]) {

  int ret, code;

  Clear_Options();
  Process_Options(argc,argv);
  Print_Options();

  ld = &base; /* select base layer context */

  /* open MPEG base layer bitstream file(s) */
  /* NOTE: this is either a base layer stream or a spatial enhancement stream */
  if ((base.Infile=open (Main_Bitstream_Filename,O_RDONLY|O_BINARY))<0) {
    fprintf (stderr,"Base layer input file %s not found\n", Main_Bitstream_Filename);
    exit (1);
    }

  if (base.Infile != 0) {
    Initialize_Buffer();
    if (Show_Bits (8) == 0x47) {
      sprintf (Error_Text,"Decoder currently does not parse transport streams\n");
      Error (Error_Text);
      }

    next_start_code();
    code = Show_Bits (32);
    switch (code) {
      //{{{
      case SEQUENCE_HEADER_CODE:
        break;
      //}}}
      //{{{
      case PACK_START_CODE:
        System_Stream_Flag = 1;
      //}}}
      //{{{
      case VIDEO_ELEMENTARY_STREAM:
        System_Stream_Flag = 1;
        break;
      //}}}
      //{{{
      default:
        sprintf(Error_Text,"Unable to recognize stream type\n");
        Error(Error_Text);
        break;
      //}}}
      }

    lseek (base.Infile, 0l, 0);
    Initialize_Buffer();
    }

  if (base.Infile!=0)
    lseek (base.Infile, 0l, 0);

  Initialize_Buffer();
  Initialize_Decoder();
  ret = Decode_Bitstream();
  close (base.Infile);

  return 0;
  }
//}}}
