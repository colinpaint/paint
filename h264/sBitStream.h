#pragma once
struct sDataPartition;
struct sMacroBlock;
struct sCabacDecode;
class cDecoder;
class cSlice;

//{{{  enum eSyntaxElementType
// almost the same as syntaxElements.h but not quite
enum eSyntaxElementType {
  SE_HEADER,
  SE_PTYPE,
  SE_MBTYPE,
  SE_REFFRAME,
  SE_INTRAPREDMODE,
  SE_MVD,
  SE_CBP,
  SE_LUM_DC_INTRA,
  SE_CHR_DC_INTRA,
  SE_LUM_AC_INTRA,
  SE_CHR_AC_INTRA,
  SE_LUM_DC_INTER,
  SE_CHR_DC_INTER,
  SE_LUM_AC_INTER,
  SE_CHR_AC_INTER,
  SE_DELTA_QUANT,
  SE_BFRAME,
  SE_EOS,
  SE_MAX_ELEMENTS = 20
  };
//}}}
//{{{  H.264 syntax elements
//  definition of H.264 syntax elements
//  order of elements follow dependencies for picture reconstruction
// *  old element     | new elements
// *  ----------------+-------------------------------------------------------------------
// *  TYPE_HEADER     | SE_HEADER, SE_PTYPE
// *  TYPE_MBHEADER   | SE_MBTYPE, SE_REFFRAME, SE_INTRAPREDMODE
// *  TYPE_MVD        | SE_MVD
// *  TYPE_CBP        | SE_CBP_INTRA, SE_CBP_INTER
// *  SE_DELTA_QUANT_INTER
// *  SE_DELTA_QUANT_INTRA
// *  TYPE_COEFF_Y    | SE_LUM_DC_INTRA, SE_LUM_AC_INTRA, SE_LUM_DC_INTER, SE_LUM_AC_INTER
// *  TYPE_2x2DC      | SE_CHR_DC_INTRA, SE_CHR_DC_INTER
// *  TYPE_COEFF_C    | SE_CHR_AC_INTRA, SE_CHR_AC_INTER
// *  TYPE_EOS        | SE_EOS
#define SE_HEADER            0
#define SE_PTYPE             1
#define SE_MBTYPE            2
#define SE_REFFRAME          3
#define SE_INTRAPREDMODE     4
#define SE_MVD               5
#define SE_CBP_INTRA         6
#define SE_LUM_DC_INTRA      7
#define SE_CHR_DC_INTRA      8
#define SE_LUM_AC_INTRA      9
#define SE_CHR_AC_INTRA      10
#define SE_CBP_INTER         11
#define SE_LUM_DC_INTER      12
#define SE_CHR_DC_INTER      13
#define SE_LUM_AC_INTER      14
#define SE_CHR_AC_INTER      15
#define SE_DELTA_QUANT_INTER 16
#define SE_DELTA_QUANT_INTRA 17
#define SE_BFRAME            18
#define SE_EOS               19
#define SE_MAX_ELEMENTS      20

#define NO_EC                0   // no error conceal necessary
#define EC_REQ               1   // error conceal required
//}}}
//{{{
//{{{  maximum possible dataPartition modes as defined in kSyntaxElementToDataPartitionIndex[][]

/*!
 *  \brief  lookup-table to assign different elements to dataPartition
 *  \note   here we defined up to 6 different dps similar to
 *          document Q15-k-18 described in the PROGFRAMEMODE.
 *          The Sliceheader contains the PSYNC information. \par
 *
 *          Elements inside a dataPartition are not ordered. They are
 *          ordered by occurence in the bitStream.
 *          Assumption: Only dplosses are considered. \par
 *
 *          The texture elements luminance and chrominance are
 *          not ordered in the progressive form
 *          This may be changed in image.c \par
 *
 *          We also defined the proposed internet dataPartition mode
 *          of Stephan Wenger here. To select the desired mode
 *          uncomment one of the two following lines. \par
 *
 *  -IMPORTANT:
 *          Picture- or Sliceheaders must be assigned to dataPartition 0. \par
 *          Furthermore dps must follow syntax dependencies as
 *          outlined in document Q15-J-23.
 */
//}}}
static const uint8_t kSyntaxElementToDataPartitionIndex[][SE_MAX_ELEMENTS] = {
  // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    // element number (do not uncomment)
  {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // all elements in one dataPartition no data dping
  {  0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 2, 2, 2, 2, 0, 0, 0, 0 }  // three dps per slice
  };
//}}}
//{{{
struct sSyntaxElement {
  int      type;        // type of syntax element for data part.
  int      value1;      // numerical value of syntax element
  int      value2;      // for blocked symbols, e.g. run/level
  int      len;         // length of code

  int      inf;         // info part of eCavlc code
  uint32_t bitpattern;  // cavlc bitpattern

  int      context;     // cabac context
  int      k;           // cabac context for coeff_count,uv

  // eCavlc mapping to syntaxElement
  void (*mapping) (int, int, int*, int*);

  // eCabac actual coding method of each individual syntax element type
  void (*reading) (sMacroBlock*, sSyntaxElement*, sCabacDecode*);
  };
//}}}

struct sBitStream {
  //{{{
  //! gives CBP value from codeword number, both for intra and inter
  static inline const uint8_t kNCBP[2][48][2] = {
    {  // 0      1        2       3       4       5       6       7       8       9      10      11
      {15, 0},{ 0, 1},{ 7, 2},{11, 4},{13, 8},{14, 3},{ 3, 5},{ 5,10},{10,12},{12,15},{ 1, 7},{ 2,11},
      { 4,13},{ 8,14},{ 6, 6},{ 9, 9},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},
      { 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},
      { 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0},{ 0, 0}
    },
    {
      {47, 0},{31,16},{15, 1},{ 0, 2},{23, 4},{27, 8},{29,32},{30, 3},{ 7, 5},{11,10},{13,12},{14,15},
      {39,47},{43, 7},{45,11},{46,13},{16,14},{ 3, 6},{ 5, 9},{10,31},{12,35},{19,37},{21,42},{26,44},
      {28,33},{35,34},{37,36},{42,40},{44,39},{ 1,43},{ 2,45},{ 4,46},{ 8,17},{17,18},{18,20},{20,24},
      {24,19},{ 6,21},{ 9,26},{22,28},{25,23},{32,27},{33,29},{34,30},{36,22},{40,25},{38,38},{41,41}
    }
  };
  //}}}
  //{{{
  //! for the infoLevRunInter routine
  static inline const uint8_t kNTAB1[4][8][2] = {
    {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{1,1},{1,2},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{2,0},{1,3},{1,4},{1,5},{0,0},{0,0},{0,0},{0,0}},
    {{3,0},{2,1},{2,2},{1,6},{1,7},{1,8},{1,9},{4,0}},
  };
  //}}}
  //{{{
  static inline const uint8_t kLEVRUN1[16] = {
    4,2,2,1,1,1,1,1,1,1,0,0,0,0,0,0,
  };
  //}}}
  //{{{
  static inline const uint8_t kNTAB2[4][8][2] = {
    {{1,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{1,1},{2,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
    {{1,2},{3,0},{4,0},{5,0},{0,0},{0,0},{0,0},{0,0}},
    {{1,3},{1,4},{2,1},{3,1},{6,0},{7,0},{8,0},{9,0}},
  };
  //}}}
  //{{{
  //! for the linfo_levrun__c2x2 routine
  static inline const uint8_t kLEVRUN3[4] = {
    2,1,0,0
  };
  //}}}
  //{{{
  static inline const uint8_t kNTAB3[2][2][2] = {
    {{1,0},{0,0}},
    {{2,0},{1,1}},
  };
  //}}}

  // statics
  static void infoUe (int len, int info, int* value1, int* dummy);
  static void infoSe (int len, int info, int* value1, int* dummy);
  static void infoCbpIntraNormal (int len, int info,int* codedBlockPattern, int* dummy);
  static void infoCbpInterNormal (int len, int info, int* codedBlockPattern, int* dummy);
  static void infoCbpIntraOther (int len, int info, int* codedBlockPattern, int* dummy);
  static void infoCbpInterOther (int len, int info, int* codedBlockPattern, int* dummy);
  static void infoLevelRunInter (int len, int info, int* level, int* irun);
  static void infoLevelRunc2x2 (int len, int info, int* level, int* irun);

  static int vlcStartCode (cSlice* slice, int dummy);
  static int readSyntaxElementVLC (sMacroBlock* mb, sSyntaxElement* se, sDataPartition* dataPartition);

  static int getVlcSymbol (uint8_t buffer[], int totalBitOffset, int* info, int bytecount);
  static int getVlcSymbolIntraMode (uint8_t buffer[], int totalBitOffset, int* info, int bytecount);
  static int getBits (uint8_t buffer[], int totalBitOffset, int* info, int bitCount, int numBits);
  static int showBits (uint8_t buffer[], int totalBitOffset, int bitCount, int numBits);
  static int moreRbspData (uint8_t buffer[], int totalBitOffset, int bytecount);

  // members
  int readSeV (const std::string& label);
  int readUeV (const std::string& label);
  bool readU1 (const std::string& label);
  int readUv (int lenInBits, const std::string& label);
  int readIv (int lenInBits, const std::string& label);

  int readSyntaxElementVLC (sSyntaxElement* se);
  int readSyntaxElementFLC (sSyntaxElement* se);
  int readSyntaxElementIntra4x4PredictionMode (sSyntaxElement* se);

  int readSyntaxElementNumCoeffTrailingOnes (sSyntaxElement* se, char* type);
  int readSyntaxElementNumCoeffTrailingOnesChromaDC (cDecoder264* decoder, sSyntaxElement* se);

  int readSyntaxElementLevelVlc0 (sSyntaxElement* se);
  int readSyntaxElementLevelVlcN (sSyntaxElement* se, int vlc);

  int readSyntaxElementTotalZeros (sSyntaxElement* se);
  int readSyntaxElementTotalZerosChromaDC (cDecoder264* decoder, sSyntaxElement* se);
  int readSyntaxElementRun (sSyntaxElement* se);

  // vars 
  uint8_t* mBuffer; // codebuffer for read bytes
  int32_t  mAllocSize; // allocated buffer size
  int      mOffset; // position in the codebuffer, bit-oriented
  int      mLength; // over codebuffer length, uint8_t oriented
  int      mError;  // error 0=noError

  // - cabac
  int      mReadLen; // position in the codebuffer
  int      mCodeLen; // overall codebuffer length
  };