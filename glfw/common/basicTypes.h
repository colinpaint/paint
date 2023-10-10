// basicTypes.h
#pragma once
#include <cstdint>
#include <string>
#include <algorithm>  // std::min, std::max
#include <functional> // function<> callbacks

#define _USE_MATH_DEFINES
#include <math.h> // sqrt, M_PI, hate the name
constexpr float kPi = M_PI;

//{{{
class cColor {
public:
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

  //{{{
  bool operator == (const cColor& color)  {
    return (r == color.r) && (g == color.g) && (b == color.b) && (a == color.a);
    }
  //}}}
  };
//}}}
//{{{  const colors
const cColor kDimBgnd     {0.1f,  0.1f,  0.f,   0.7f};
const cColor kClearBgnd   {0.0f,  0.0f,  0.0f,  0.7f};

const cColor kBlack       {0.f,   0.f,   0.f,   1.0f};
const cColor kDimGray     {0.1f,  0.1f,  0.1f,  1.0f};
const cColor kDarkGray    {0.3f,  0.3f,  0.3f,  1.0f};
const cColor kGray        {0.5f,  0.5f,  0.5f,  1.0f};
const cColor kLightGray   {0.7f,  0.7f,  0.7f,  1.0f};
const cColor kWhite       {1.f,   1.f,   1.f,   1.0f};

const cColor kTextGray    {0.05f, 0.05f, 0.05f, 1.0f};
const cColor kBoxGray     { 0.8f, 0.8f,  0.8f,  1.0f};

const cColor kDarkBlue    {0.2f,  0.2f,  0.8f,  1.0f};
const cColor kLightBlue   {0.5f,  0.5f,  0.8f,  1.0f};

const cColor kRed         {0.8f,  0.1f,  0.1f,  1.0f};
const cColor kYellow      {0.8f,  0.8f,  0.0f,  1.0f};
const cColor kOrange      {0.8f,  0.6f,  0.0f,  1.0f};

const cColor kGreen       {0.2f,  0.8f,  0.2f,  1.0f};
const cColor kDarkGreen   {0.1f,  0.8f,  0.1f,  1.0f};
const cColor kGreenYellow {0.7f,  0.7f,  0.0f,  1.0f};

const cColor kFullRed     {1.0f,  0.0f,  0.0f,  1.0f};
const cColor kFullGreen   {0.0f,  1.0f,  0.0f,  1.0f};
const cColor kFullBlue    {0.0f,  0.0f,  1.0f,  1.0f};
//}}}

//{{{  class cPoint
class cPoint {
public:
  float x;
  float y;

  //{{{
  cPoint()  {
    x = 0;
    y = 0;
    }
  //}}}
  //{{{
  cPoint (float value) {
    this->x = value;
    this->y = value;
    }
  //}}}
  //{{{
  cPoint (double value) {
    this->x = (float)value;
    this->y = (float)value;
    }
  //}}}
  //{{{
  cPoint (float x, float y) {
    this->x = x;
    this->y = y;
    }
  //}}}
  //{{{
  cPoint (double x, double y) {
    this->x = (float)x;
    this->y = (float)y;
    }
  //}}}
  //{{{
  cPoint (int x, int y) {
    this->x = (float)x;
    this->y = (float)y;
    }
  //}}}

  // int gets
  int32_t getXInt32() const { return (int32_t)x; }
  int32_t getYInt32() const { return (int32_t)y; }

  // operators
  //{{{
  cPoint operator - () const {
    return cPoint (-x, -y);
    }
  //}}}
  //{{{
  cPoint operator - (cPoint point) const {
    return cPoint (x - point.x, y - point.y);
    }
  //}}}
  //{{{
  cPoint operator + (cPoint point) const {
    return cPoint (x + point.x, y + point.y);
    }
  //}}}
  //{{{
  cPoint operator * (float f) const {
    return cPoint (x * f, y * f);
    }
  //}}}
  //{{{
  cPoint operator * (double f) const {
    return cPoint (float(x * f), float(y * f));
    }
  //}}}
  //{{{
  cPoint operator * (cPoint point) const {
    return cPoint (x * point.x, y * point.y);
    }
  //}}}
  //{{{
  cPoint operator / (float f) const {
    return cPoint (x / f, y / f);
    }
  //}}}
  //{{{
  cPoint operator / (double f) const {
    return cPoint (float(x / f), float(y / f));
    }
  //}}}

  //{{{
  const cPoint& operator += (cPoint point) {
    x += point.x;
    y += point.y;
    return *this;
    }
  //}}}
  //{{{
  const cPoint& operator -= (cPoint point) {
    x -= point.x;
    y -= point.y;
    return *this;
    }
  //}}}

  //{{{
  bool operator == (cPoint point) {
    return (x == point.x) && (y == point.y);
    }
  //}}}
  //{{{
  bool operator != (cPoint point) {
    return (x != point.x) || (y != point.y);
    }
  //}}}
  //{{{
  bool inside (cPoint pos) const {
  // return pos inside rect formed by us as size
    return pos.x >= 0 && pos.x < x && pos.y >= 0 && pos.y < y;
    }
  //}}}

  //{{{
  float magnitude() const {
  // return magnitude of point as vector
    return float(sqrt ((x*x) + (y*y)));
    }
  //}}}
  //{{{
  cPoint perp() {
    float mag = magnitude();
    return cPoint (-y / mag, x / mag);
    }
  //}}}
  };
//}}}
//{{{  class cRect
class cRect {
  public:
    float left;
    float top;
    float right;
    float bottom;

  //{{{
  cRect()  {
    left = 0;
    bottom = 0;
    right = 0;
    bottom = 0;
    }
  //}}}
  //{{{
  cRect (float sizeX, float sizeY)  {
    left = 0;
    top = 0;
    right = sizeX;
    bottom = sizeY;
    }
  //}}}
  //{{{
  cRect (float l, float t, float r, float b)  {
    left = l;
    top = t;
    right = r;
    bottom = b;
    }
  //}}}

  //{{{
  cRect (cPoint size)  {
    left = 0;
    top = 0;
    right = size.x;
    bottom = size.y;
    }
  //}}}
  //{{{
  cRect (cPoint topLeft, cPoint bottomRight)  {
    left = topLeft.x;
    top = topLeft.y;
    right = bottomRight.x;
    bottom = bottomRight.y;
    }
  //}}}

  //{{{
  bool empty() const {
    return area() == 0;
    }
  //}}}
  //{{{
  float area() const {
    return getWidth() * getHeight();
    }
  //}}}
  float getWidth() const { return right - left; }
  float getHeight() const { return bottom - top; }

  cPoint getTL() const { return cPoint(left, top); }
  cPoint getTR() const { return cPoint(right, top); }
  cPoint getBL() const { return cPoint(left, bottom); }
  cPoint getBR() const { return cPoint(right, bottom); }

  cPoint getSize() const { return cPoint(right-left, bottom-top); }
  cPoint getCentre() const { return cPoint(getCentreX(), getCentreY()); }
  float getCentreX() const { return (left + right)/2.f; }
  float getCentreY() const { return (top + bottom)/2.f; }

  // int access
  int32_t getTopInt32() const { return (int32_t)top; }
  int32_t getLeftInt32() const { return (int32_t)left; }
  int32_t getRightInt32() const { return (int32_t)right; }
  int32_t getBottomInt32() const { return (int32_t)bottom; }
  int32_t getWidthInt32() const { return int32_t(right - left); }
  int32_t getHeightInt32() const { return int32_t(bottom - top); }
  uint32_t getWidthUint32() const { return uint32_t(right - left); }
  uint32_t getHeightUint32() const { return uint32_t(bottom - top); }

  //{{{
  cRect operator + (cPoint point) const {
    return cRect (left + point.x, top + point.y, right + point.x, bottom + point.y);
    }
  //}}}
  //{{{
  cRect operator / (float scale) const {
    return cRect (left / scale, top / scale, right / scale, bottom / scale);
    }
  //}}}
  //{{{
  cRect operator * (float scale) const {
    return cRect (left * scale, top * scale, right * scale, bottom * scale);
    }
  //}}}
  //{{{
  cRect operator |= (const cPoint& point) {

    if (empty()) {
      left = point.x;
      top = point.y;
      right = point.x + 1;
      bottom = point.y + 1;
      }
    else {
      if (left > point.x)
        left = point.x;
      if (top > point.y)
        top = point.y;
      if (right <= point.x)
        right = point.x + 1;
      if (bottom <= point.y)
        bottom = point.y + 1;
      }

    return *this;
    }
  //}}}

  //{{{
  bool operator == (const cRect& rect)  {
    return (left == rect.left) && (right == rect.right) &&
           (top == rect.top)   && (bottom == rect.bottom);
    }
  //}}}
  //{{{
  bool operator != (const cRect& rect)  {
    return (left != rect.left) || (right != rect.right) ||
           (top != rect.top)   || (bottom != rect.bottom);
    }
  //}}}

  //{{{
  bool inside (cPoint pos) const {
  // return pos inside rect
    return (pos.x >= left) && (pos.x < right) && (pos.y >= top) && (pos.y < bottom);
    }
  //}}}

  //{{{
  void addHorizontal (float width) {
    left += width;
    right += width;
    }
  //}}}
  //{{{
  void addVertical (float width) {
    top += width;
    bottom += width;
    }
  //}}}
  //{{{
  void addBorder (float width) {

    left += width;
    top += width;
    right -= 2*width;
    bottom -= 2*width;
    }
  //}}}
  };
//}}}
//{{{  class cClipRect
class cClipRect {
public:
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
  int32_t srcLeft;
  int32_t srcTop;
  bool empty;

  //{{{
  cClipRect (const cRect& rect, int32_t width, int32_t height)
      : left ((int32_t)rect.left), top((int32_t)rect.top),
        right((int32_t)rect.right), bottom((int32_t)rect.bottom),
        srcLeft(0), srcTop(0), empty(false) {

    if (left >= width) {
      empty = true;
      return;
      }
    if (left < 0) {
      srcLeft = -left;
      left = 0;
      }

    if (right < 0) {
      empty = true;
      return;
      }
    if (right > width)
      right = width;

    if (top >= height) {
      empty = true;
      return;
      }
    if (top < 0) {
      srcTop = -top;
      top = 0;
      }

    if (bottom < 0) {
      empty = true;
      return;
      }
    if (bottom > height)
      bottom = height;
    };
  //}}}
  //{{{
  cClipRect (const cRect& rect, const cRect& clip)
      : left ((int32_t)rect.left), top((int32_t)rect.top),
        right((int32_t)rect.right), bottom((int32_t)rect.bottom),
        srcLeft(0), srcTop(0), empty(false) {

    if (left >= clip.right) {
      empty = true;
      return;
      }
    if (left < clip.left) {
      srcLeft = (int32_t)clip.left - left;
      left = (int32_t)clip.left;
      }

    if (right < clip.left) {
      empty = true;
      return;
      }
    if (right > clip.right)
      right = (int32_t)clip.right;

    if (top >= clip.bottom) {
      empty = true;
      return;
      }
    if (top < clip.top) {
      srcTop = (int32_t)clip.top - top;
      top = (int32_t)clip.top;
      }

    if (bottom < clip.top) {
      empty = true;
      return;
      }
    if (bottom > clip.bottom)
      bottom = (int32_t)clip.bottom;
    };
  //}}}

  int32_t getWidth() const { return right - left; }
  int32_t getHeight() const { return bottom - top; }
  };
//}}}

//{{{
class cMatrix3x2 {
public:
  cMatrix3x2() : m00(1),m01(0),m02(0), m10(0),m11(1),m12(0) {}
  cMatrix3x2 (const cMatrix3x2& matrix)
    : m00(matrix.m00), m01(matrix.m01), m02(matrix.m02), m10(matrix.m10), m11(matrix.m11), m12(matrix.m12) {}
  cMatrix3x2 (float value00, float value01, float value02, float value10, float value11, float value12)
    : m00(value00),m01(value01),m02(value02), m10(value10),m11(value11),m12(value12) {}

  //{{{
  static cMatrix3x2 createTranslate (const cPoint& point) {
    return cMatrix3x2 (1,0,point.x, 0,1,point.y);
    }
  //}}}
  //{{{
  static cMatrix3x2 createTranslate (float x, float y) {
    return cMatrix3x2 (1,0,x, 0,1,y);
    }
  //}}}
  //{{{
  static cMatrix3x2 createRotate (float angle) {
    float c = (float)cos (angle * kPi*2.f);
    float s = (float)sin (angle * kPi*2.f);
    return cMatrix3x2 (c,-s,0, s,c,0);
    }
  //}}}
  //{{{
  static cMatrix3x2 createScale (const cPoint& point) {
    return cMatrix3x2 (point.x,0,0, 0,point.y,0);
    }
  //}}}
  //{{{
  static cMatrix3x2 createScale (float x, float y) {
    return cMatrix3x2 (x,0,0, 0,y,0 );
    }
  //}}}
  //{{{
  static cMatrix3x2 createScale (float scale) {
    return cMatrix3x2 (scale,0,0, 0,scale,0);
    }
  //}}}

  //{{{
  cMatrix3x2& operator = (const cMatrix3x2& m) {
    m00 = m.m00;
    m01 = m.m01;
    m10 = m.m10;
    m11 = m.m11;
    m02 = m.m02;
    m12 = m.m12;
    return *this;
    }
  //}}}
  //{{{
  cMatrix3x2 operator * (const cMatrix3x2& m) {
    return cMatrix3x2 ((m00 * m.m00) + (m01 * m.m10),
                       (m00 * m.m01) + (m01 * m.m11),
                       (m00 * m.m02) + (m01 * m.m12) + m02,
                       (m10 * m.m00) + (m11 * m.m10),
                       (m10 * m.m01) + (m11 * m.m11),
                       (m10 * m.m02) + (m11 * m.m12) + m12);
    }
  //}}}

  //{{{
  cPoint operator * (const cPoint& p) {
    return cPoint ((m00 * p.x) + (m10 * p.y) + m02,
                   (m01 * p.x) + (m11 * p.y) + m12);
    }
  //}}}

  //{{{
  void translate (const cPoint& point) {
    (*this) = (*this) * createTranslate (point);
    }
  //}}}
  //{{{
  void translate (float x, float y) {
    (*this) = (*this) * createTranslate (x, y);
    }
  //}}}
  //{{{
  void rotate (float angle) {
    (*this) = (*this) * createRotate (angle);
    }
  //}}}
  //{{{
  void scale (const cPoint& point) {
    (*this) = (*this) * createScale (point);
    }
  //}}}
  //{{{
  void scale (float x, float y) {
    (*this) = (*this) * createScale (x, y);
    }
  //}}}
  //{{{
  void scale (float s) {
    (*this) = (*this) * createScale (s);
    }
  //}}}

  float m00, m01, m02;
  float m10, m11, m12;
  };
//}}}
