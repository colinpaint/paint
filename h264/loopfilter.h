#pragma once
class cDecoder;
struct sPicture;

void deblockPicture (cDecoder264* decoder, sPicture* p) ;
void initDeblock (cDecoder264* decoder, int mbAffFrame);
