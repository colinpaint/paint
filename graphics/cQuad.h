// cQuad.h - quad vertices
#pragma once
//{{{  includes
#include <cstdint>

#include "cPointRect.h"
//}}}

class cQuad {
public:
  cQuad (cPoint size);
  cQuad (cPoint size, const cRect& rect);
  ~cQuad();

  void draw();

private:
  const cPoint mSize;

  uint32_t mVertexArrayObject = 0;
  uint32_t mVertexBufferObject = 0;
  uint32_t mElementArrayBufferObject = 0;
  uint32_t mNumIndices = 0;
  };
