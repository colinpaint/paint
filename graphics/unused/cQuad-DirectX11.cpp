// cQuad-DirectX11.cpp - quad vertices -NOT YET
//{{{  includes
#include "cQuad.h"

#include <cstdint>
#include <cmath>

// glm
#include <gtc/matrix_transform.hpp>
#include <gtx/string_cast.hpp>
//}}}

namespace {
  //{{{
  const uint8_t kIndices[] = {
    0, 1, 2, // 0   0-3
    0, 3, 1  // |\   \|
    };       // 2-1   1
  //}}}
  }

//{{{
cQuad::cQuad (cPoint size) : mSize(size) {

  // vertices
  //glGenBuffers (1, &mVertexBufferObject);
  //glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

  const float widthF = static_cast<float>(size.x);
  const float heightF = static_cast<float>(size.y);
  const float kVertices[] = {
    0.f,   heightF,  0.f,1.f, // tl vertex
    widthF,0.f,      1.f,0.f, // br vertex
    0.f,   0.f,      0.f,0.f, // bl vertex
    widthF,heightF,  1.f,1.f, // tr vertex
    };

  // indices
  mNumIndices = sizeof(kIndices);
  }
//}}}
//{{{
cQuad::cQuad (cPoint size, const cRect& rect) : mSize(size) {

  // vertexArray

  const float subLeftF = static_cast<float>(rect.left);
  const float subRightF = static_cast<float>(rect.right);
  const float subBottomF = static_cast<float>(rect.bottom);
  const float subTopF = static_cast<float>(rect.top);

  const float subLeftTexF = subLeftF / size.x;
  const float subRightTexF = subRightF / size.x;
  const float subBottomTexF = subBottomF / size.y;
  const float subTopTexF = subTopF / size.y;

  const float kVertices[] = {
    subLeftF, subTopF,     subLeftTexF, subTopTexF,    // tl
    subRightF,subBottomF,  subRightTexF,subBottomTexF, // br
    subLeftF, subBottomF,  subLeftTexF, subBottomTexF, // bl
    subRightF,subTopF,     subRightTexF,subTopTexF     // tr
    };


  // indices
  mNumIndices = sizeof(kIndices);
  }
//}}}
//{{{
cQuad::~cQuad() {
  }
//}}}

void cQuad::draw() {
  }
