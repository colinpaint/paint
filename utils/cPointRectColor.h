// cPointRectColor.h - simple,readable, portable types
#pragma once
//{{{  includes
#include <cstdint>
#include <cmath>
#include <algorithm>
//}}}

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
  // return pos inside rect
    return (pos.x >= left) && (pos.x < right) && (pos.y >= top) && (pos.y < bottom);
    }
  //}}}
  };
//}}}

// "color" in imGui so don't use "colour" here
//{{{
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
