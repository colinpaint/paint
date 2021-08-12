// cQuad-OpenGL.cpp - quad vertices
//{{{  includes
#include "cQuad.h"

#include <cstdint>
#include <cmath>

// glad
#include <glad/glad.h>

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

  // vertexArray
  glGenVertexArrays (1, &mVertexArrayObject);
  glBindVertexArray (mVertexArrayObject);

  // vertices
  glGenBuffers (1, &mVertexBufferObject);
  glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray (0);

  glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
  glEnableVertexAttribArray (1);

  const float widthF = static_cast<float>(size.x);
  const float heightF = static_cast<float>(size.y);
  const float kVertices[] = {
    0.f,   heightF,  0.f,1.f, // tl vertex
    widthF,0.f,      1.f,0.f, // br vertex
    0.f,   0.f,      0.f,0.f, // bl vertex
    widthF,heightF,  1.f,1.f, // tr vertex
    };

  glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

  // indices
  mNumIndices = sizeof(kIndices);

  glGenBuffers (1, &mElementArrayBufferObject);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);

  glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
  }
//}}}
//{{{
cQuad::cQuad (cPoint size, const cRect& rect) : mSize(size) {

  // vertexArray
  glGenVertexArrays (1, &mVertexArrayObject);
  glBindVertexArray (mVertexArrayObject);

  // vertices
  glGenBuffers (1, &mVertexBufferObject);
  glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray (0);

  glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
  glEnableVertexAttribArray (1);

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

  glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

  // indices
  mNumIndices = sizeof(kIndices);

  glGenBuffers (1, &mElementArrayBufferObject);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
  }
//}}}
//{{{
cQuad::~cQuad() {

  glDeleteBuffers (1, &mElementArrayBufferObject);
  glDeleteBuffers (1, &mVertexBufferObject);
  glDeleteVertexArrays (1, &mVertexArrayObject);
  }
//}}}

void cQuad::draw() {

  glBindVertexArray (mVertexArrayObject);
  glDrawElements (GL_TRIANGLES, mNumIndices, GL_UNSIGNED_BYTE, 0);
  }
