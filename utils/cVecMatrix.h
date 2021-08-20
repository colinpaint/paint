// cVecMateix.h - simple, minimal, readable, portable types
// - use glm if you want the full monty
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring> // memcpy
#include <cmath>
#include <algorithm>
//}}}

// cVec2
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

// cVec3
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

// cMatrix4x4
struct cMatrix4x4 {
  float mat[4][4];
  //{{{
  cMatrix4x4() {
  // construct identity matrix

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
  cMatrix4x4 (float left, float right, float bottom, float top, float zNear, float zFar) {
  // construct ortho projection matrix

    mat[0][0] = 2.f / (right - left);
    mat[0][1] = 0.f;
    mat[0][2] = 0.f;
    mat[0][3] = 0.f;

    mat[1][0] = 0.f;
    mat[1][1] = 2.f / (top - bottom);
    mat[1][2] = 0.f;
    mat[1][3] = 0.f;

    mat[2][0] = 0.f;
    mat[2][1] = 0.f;
    mat[2][2] = -2.f / (zFar - zNear);
    mat[2][3] = 0.f;

    mat[3][0] = - (right + left) / (right - left);
    mat[3][1] = - (top + bottom) / (top - bottom);
    mat[3][2] = - (zFar + zNear) / (zFar - zNear);
    mat[3][3] = 1.f;
    }
  //}}}

  //{{{
  void setTranslate (const cVec3& pos) {
  // set translate to pos

    mat[3][0] = pos.x;
    mat[3][1] = pos.y;
    mat[3][2] = pos.z;
    }
  //}}}
  //{{{
  void translate (const cVec3& offset) {
  // translate by offset

    mat[3][0] += offset.x;
    mat[3][1] += offset.y;
    mat[3][2] += offset.z;
    }
  //}}}
  };
