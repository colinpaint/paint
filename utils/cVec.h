// cVec.h - simple portable types
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring> // memcpy
#include <cmath>
#include <algorithm>
//}}}

//{{{
struct cVec2 {
  float x;
  float y;

  //{{{
  cVec2()  {
    x = 0;
    y = 0;
    }
  //}}}
  //{{{
  cVec2 (float x, float y) {
    this->x = x;
    this->y = y;
    }
  //}}}

  //{{{
  cVec2 operator - (const cVec2& vec2) const {
    return cVec2(x - vec2.x, y - vec2.y);
    }
  //}}}
  //{{{
  cVec2 operator + (const cVec2& vec2) const {
    return cVec2(x + vec2.x, y + vec2.y);
    }
  //}}}
  //{{{
  cVec2 operator * (const cVec2& vec2) const {
    return cVec2(x * vec2.x, y * vec2.y);
    }
  //}}}
  //{{{
  cVec2 operator * (float scale) const {
    return cVec2(x * scale, y * scale);
    }
  //}}}

  //{{{
  const cVec2& operator += (const cVec2& vec2)  {
    x += vec2.x;
    y += vec2.y;
    return *this;
    }
  //}}}
  //{{{
  const cVec2& operator -= (const cVec2& vec2)  {
    x -= vec2.x;
    y -= vec2.y;
    return *this;
    }
  //}}}
  //{{{
  float magnitude() const {
  // return magnitude of point as vector
    return float(sqrt ((x*x) + (y*y)));
    }
  //}}}
  };
//}}}
//{{{
struct cVec3 {
  float x;
  float y;
  float z;

  //{{{
  cVec3()  {
    x = 0;
    y = 0;
    z = 0;
    }
  //}}}
  //{{{
  cVec3 (float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
    }
  //}}}
  };
//}}}
//{{{
struct cVec4 {
  float x;
  float y;
  float z;
  float w;

  //{{{
  cVec4()  {
    x = 0;
    y = 0;
    z = 0;
    w = 0;
    }
  //}}}
  //{{{
  cVec4 (float x, float y, float z, float w ) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
    }
  //}}}
  };
//}}}

//{{{
struct cMat44 {
  float mat[4][4];

  //{{{
  cMat44() {
    mat[0][0] = 1.f;
    mat[0][1] = 0.f;
    mat[0][2] = 0.f;
    mat[0][3] = 0.f;

    mat[1][0] = 0.f;
    mat[1][1] = 1.f;
    mat[1][2] = 0.f;
    mat[1][3] = 0.f;

    mat[2][0] = 0.f;
    mat[2][1] = 0.f;
    mat[2][2] = 1.f;
    mat[2][3] = 0.f;

    mat[3][0] = 0.f;
    mat[3][1] = 0.f;
    mat[3][2] = 0.f;
    mat[3][3] = 1.f;
    }
  //}}}
  //{{{
  cMat44 (const cMat44& mat) {
    memcpy (this, &mat, sizeof (cMat44));
    }
  //}}}
  };
//}}}
