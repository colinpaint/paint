// basicTypes.h - simple,readable,portable types, structs for now, maybe classes later
#pragma once
//{{{  includes
#include <cstdint>
#include <cstring> // memcpy
#include <cmath>
#include <string>
#include <algorithm>

#include "../common/cLog.h"
#include "fmt/format.h"
//}}}

// float vec, matrix
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
struct cMat4x4 {
  float mat[4][4];

  //{{{
  cMat4x4() {
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
  cMat4x4 (float left, float right, float bottom, float top, float zNear, float zFar) {
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
  void setTranslate (const cVec2& pos) {
  // set translate to pos

    mat[3][0] = pos.x;
    mat[3][1] = pos.y;
    mat[3][2] = 0.f;
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
  void setSize (const cVec2& size) {

    mat[0][0] = size.x;
    mat[1][1] = size.y;
    }
  //}}}

  //{{{
  void translate (const cVec2& offset) {
  // translate by offset

    mat[3][0] += offset.x;
    mat[3][1] += offset.y;
    }
  //}}}
  //{{{
  void size (const cVec2& size) {

    mat[0][0] *= size.x;
    mat[1][1] *= size.y;
    }
  //}}}

  //{{{
  cMat4x4 operator * (const cMat4x4& matR) const {

    cMat4x4 result;

    for (int i = 0; i <= 3; i++)
      for (int j = 0; j <= 3; j++) {
        result.mat[i][j] = 0;
        for (int k = 0; k <= 3; k++)
          result.mat[i][j] += mat[i][k] * matR.mat[k][j];
        }

    return (result);
    }
  //}}}

  //{{{
  cVec2 transform (const cVec2& pos) {
    return { (mat[0][0] * pos.x) + (mat[1][0] * pos.y) + mat[3][0],
             (mat[0][1] * pos.x) + (mat[1][1] * pos.y) + mat[3][1] };
    }
  //}}}
  //{{{
  cVec2 transform (const cVec2& pos, float yflip) {
    return { (mat[0][0] * pos.x) + (mat[1][0] * pos.y) + mat[3][0],
             yflip - ((mat[0][1] * pos.x) + (mat[1][1] * pos.y) + mat[3][1]) };
    }
  //}}}

  //{{{
  void show (const std::string& name) {

    cLog::log (LOGINFO, "mat4x4 " + name);

    for (int j = 0; j <= 3; j++)
      cLog::log (LOGINFO, fmt::format ("  {:7.3f} {:7.3f} {:7.3f} {:7.3f}",
                                       mat[j][0], mat[j][1], mat[j][2], mat[j][3]));
    }
  //}}}
  };
//}}}

// int32_t point, rect
//{{{
struct cPoint {
  int32_t x;
  int32_t y;

  //{{{
  cPoint()  {
    x = 0;
    y = 0;
    }
  //}}}
  //{{{
  cPoint (int32_t x, int32_t y) {
    this->x = x;
    this->y = y;
    }
  //}}}
  //{{{
  cPoint (const cVec2& pos) {
    this->x = (int32_t)pos.x;
    this->y = (int32_t)pos.y;
    }
  //}}}

  //{{{
  cPoint operator - (const cPoint& point) const {
    return cPoint (x - point.x, y - point.y);
    }
  //}}}
  //{{{
  cPoint operator + (const cPoint& point) const {
    return cPoint (x + point.x, y + point.y);
    }
  //}}}
  //{{{
  cPoint operator * (const cPoint& point) const {
    return cPoint (x * point.x, y * point.y);
    }
  //}}}

  //{{{
  const cPoint& operator += (const cPoint& point)  {
    x += point.x;
    y += point.y;
    return *this;
    }
  //}}}
  //{{{
  const cPoint& operator -= (const cPoint& point)  {
    x -= point.x;
    y -= point.y;
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
struct cRect {
  int32_t left;
  int32_t right;
  int32_t top;
  int32_t bottom;

  //{{{
  cRect() {
    left = 0;
    top = 0;
    right = 0;
    bottom = 0;
    }
  //}}}
  //{{{
  cRect (const cPoint& size)  {
    left = 0;
    top = 0;

    right = size.x;
    bottom = size.y;
    }
  //}}}
  //{{{
  cRect (int32_t left, int32_t top, int32_t right, int32_t bottom)  {

    this->left = left;
    this->top = top;

    this->right = right;
    this->bottom = bottom;
    }
  //}}}
  //{{{
  cRect (const cPoint& topLeft, const cPoint& bottomRight)  {

    left = topLeft.x;
    top = topLeft.y;

    right = bottomRight.x;
    bottom = bottomRight.y;
    }
  //}}}
  //{{{
  cRect (const cVec2& size)  {

    left = 0;
    top = 0;

    right = (int32_t)size.x;
    bottom = (int32_t)size.y;
    }
  //}}}
  //{{{
  cRect (const cVec2& topLeft, const cVec2& bottomRight)  {

    left = (int32_t)topLeft.x;
    top = (int32_t)topLeft.y;

    right = (int32_t)bottomRight.x;
    bottom = (int32_t)bottomRight.y;
    }
  //}}}

  //{{{
  cRect operator + (const cPoint& point) const {
    return cRect (left + point.x, top + point.y, right + point.x, bottom + point.y);
    }
  //}}}
  //{{{
  cRect operator + (const cRect& rect) const {
    return cRect (std::min (left, rect.left), std::min (top, rect.top),
                  std::max (right, rect.right), std::max (bottom, rect.bottom));
    }
  //}}}
  //{{{
  const cRect& operator += (const cRect& rect)  {

    left = std::min (left, rect.left);
    top = std::min (top, rect.top),

    right = std::max (right, rect.right);
    bottom = std::max (bottom, rect.bottom);

    return *this;
    }
  //}}}

  int32_t getWidth() const { return right - left; }
  int32_t getHeight() const { return bottom - top; }
  cPoint getSize() const { return cPoint(getWidth(), getHeight()); }
  cPoint getCentre() const { return cPoint(getWidth()/2, getHeight()/2); }

  cPoint getTL() const { return cPoint(left, top); }
  cPoint getTR() const { return cPoint(right, top); }
  cPoint getBL() const { return cPoint(left, bottom); }
  cPoint getBR() const { return cPoint(right, bottom); }

  //{{{
  bool isEmpty() {
    return (getWidth() <= 0) || (getHeight() <= 0);
    }
  //}}}
  //{{{
  bool isInside (const cPoint& pos) {
    return (pos.x >= left) && (pos.x < right) && (pos.y >= top) && (pos.y < bottom);
    }
  //}}}
  };
//}}}

// float color
//{{{
// "color" in imGui so don't use "colour" here
struct cColor {
  float r;
  float g;
  float b;
  float a;

  //{{{
  cColor()  {
    r = 0.f;
    g = 0.f;
    b = 0.f;
    a = 1.f;
    }
  //}}}
  //{{{
  cColor (float r, float g, float b, float a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
    }
  //}}}
  };
//}}}
