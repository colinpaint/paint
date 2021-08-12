// cShader-DirectX11.cpp - simple shader wrapper - NOT YET!!!
//{{{  includes
#include "cShader.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// glad
#include <glad/glad.h>

// glm
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>
#include <gtc/type_ptr.hpp>

#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  #ifdef OPENGL_2
    //{{{
    const string kQuadVertShader =
      "#version 120\n"

      "layout (location = 0) in vec2 inPos;"
      "layout (location = 1) in vec2 inTextureCoord;"
      "out vec2 textureCoord;"

      "uniform mat4 uModel;"
      "uniform mat4 uProject;"

      "void main() {"
      "  textureCoord = inTextureCoord;"
      "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
      "  }";
    //}}}
    //{{{
    const string kCanvasFragShader =
      "#version 120\n"

      "uniform sampler2D uSampler;"

      "in vec2 textureCoord;"
      "out vec4 outColor;"

      "void main() {"
      "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
      //"  outColor /= outColor.w;"
      "  }";
    //}}}
    //{{{
    const string kLayerFragShader =
      "#version 120\n"

      "uniform sampler2D uSampler;"
      "uniform float uHue;"
      "uniform float uVal;"
      "uniform float uSat;"

      "in vec2 textureCoord;"
      "out vec4 outColor;"

      //{{{
      "vec3 rgbToHsv (float r, float g, float b) {"
      "  float max_val = max(r, max(g, b));"
      "  float min_val = min(r, min(g, b));"
      "  float h;" // hue in degrees

      "  if (max_val == min_val) {" // Simple default case. Do NOT increase saturation if this is the case!
      "    h = 0.0; }"
      "  else if (max_val == r) {"
      "    h = 60.0 * (0.0 + (g - b) / (max_val - min_val)); }"
      "  else if (max_val == g) {"
      "    h = 60.0 * (2.0 + (b - r)/ (max_val - min_val)); }"
      "  else if (max_val == b) {"
      "    h = 60.0 * (4.0 + (r - g) / (max_val - min_val)); }"
      "  if (h < 0.0) {"
      "    h += 360.0; }"

      "  float s = max_val == 0.0 ? 0.0 : (max_val - min_val) / max_val;"
      "  float v = max_val;"
      "  return vec3 (h, s, v);"
      "  }"
      //}}}
      //{{{
      "vec3 hsvToRgb (float h, float s, float v) {"
      "  float r, g, b;"
      "  float c = v * s;"
      "  float h_ = mod(h / 60.0, 6);" // For convenience, change to multiples of 60
      "  float x = c * (1.0 - abs(mod(h_, 2) - 1));"
      "  float r_, g_, b_;"

      "  if (0.0 <= h_ && h_ < 1.0) {"
      "    r_ = c, g_ = x, b_ = 0.0; }"
      "  else if (1.0 <= h_ && h_ < 2.0) {"
      "    r_ = x, g_ = c, b_ = 0.0; }"
      "  else if (2.0 <= h_ && h_ < 3.0) {"
      "    r_ = 0.0, g_ = c, b_ = x; }"
      "  else if (3.0 <= h_ && h_ < 4.0) {"
      "    r_ = 0.0, g_ = x, b_ = c; }"
      "  else if (4.0 <= h_ && h_ < 5.0) {"
      "    r_ = x, g_ = 0.0, b_ = c; }"
      "  else if (5.0 <= h_ && h_ < 6.0) {"
      "    r_ = c, g_ = 0.0, b_ = x; }"
      "  else {"
      "    r_ = 0.0, g_ = 0.0, b_ = 0.0; }"

      "  float m = v - c;"
      "  r = r_ + m;"
      "  g = g_ + m;"
      "  b = b_ + m;"

      "  return vec3 (r, g, b);"
      "  }"
      //}}}

      "void main() {"
      "  outColor = texture (uSampler, textureCoord);"

      "  if (uHue != 0.0 || uVal != 0.0 || uSat != 0.0) {"
      "    vec3 hsv = rgbToHsv (outColor.x, outColor.y, outColor.z);"
      "    hsv.x += uHue;"
      "    if ((outColor.x != outColor.y) || (outColor.y != outColor.z)) {"
             // not grayscale
      "      hsv.y = uSat <= 0.0 ? "
      "      hsv.y * (1.0 + uSat) : hsv.y + (1.0 - hsv.y) * uSat;"
      "      }"
      "    hsv.z = uVal <= 0.0 ? hsv.z * (1.0 + uVal) : hsv.z + (1.0 - hsv.z) * uVal;"
      "    vec3 rgb = hsvToRgb (hsv.x, hsv.y, hsv.z);"
      "    outColor.xyz = rgb;"
      "    }"

      //"  if (uPreMultiply)"
      //"    outColor.xyz *= outColor.w;"
      "  }";
    //}}}
    //{{{
    const string kPaintFragShader =
      "#version 120\n"

      "uniform sampler2D uSampler;"
      "uniform vec2 uPos;"
      "uniform vec2 uPrevPos;"
      "uniform float uRadius;"
      "uniform vec4 uColor;"

      "in vec2 textureCoord;"
      "out vec4 outColor;"

      "float distToLine (vec2 v, vec2 w, vec2 p) {"
      "  float l2 = pow (distance(w, v), 2.);"
      "  if (l2 == 0.0)"
      "    return distance (p, v);"
      "  float t = clamp (dot (p - v, w - v) / l2, 0., 1.);"
      "  vec2 j = v + t * (w - v);"
      "  return distance (p, j);"
      "  }"

      "void main() {"
      "  float dist = distToLine (uPrevPos.xy, uPos.xy, textureCoord * textureSize (uSampler, 0)) - uRadius;"
      "  outColor = mix (uColor, texture (uSampler, textureCoord), clamp (dist, 0.0, 1.0));"
      "  }";
    //}}}
  #else
    //{{{
    const string kQuadVertShader =
      "#version 330 core\n"
      "layout (location = 0) in vec2 inPos;"
      "layout (location = 1) in vec2 inTextureCoord;"
      "out vec2 textureCoord;"

      "uniform mat4 uModel;"
      "uniform mat4 uProject;"

      "void main() {"
      "  textureCoord = inTextureCoord;"
      "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
      "  }";
    //}}}
    //{{{
    const string kCanvasFragShader =
      "#version 330 core\n"
      "uniform sampler2D uSampler;"

      "in vec2 textureCoord;"
      "out vec4 outColor;"

      "void main() {"
      "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
      //"  outColor /= outColor.w;"
      "  }";
    //}}}
    //{{{
    const string kLayerFragShader =
      "#version 330 core\n"
      "uniform sampler2D uSampler;"
      "uniform float uHue;"
      "uniform float uVal;"
      "uniform float uSat;"

      "in vec2 textureCoord;"
      "out vec4 outColor;"

      //{{{
      "vec3 rgbToHsv (float r, float g, float b) {"
      "  float max_val = max(r, max(g, b));"
      "  float min_val = min(r, min(g, b));"
      "  float h;" // hue in degrees

      "  if (max_val == min_val) {" // Simple default case. Do NOT increase saturation if this is the case!
      "    h = 0.0; }"
      "  else if (max_val == r) {"
      "    h = 60.0 * (0.0 + (g - b) / (max_val - min_val)); }"
      "  else if (max_val == g) {"
      "    h = 60.0 * (2.0 + (b - r)/ (max_val - min_val)); }"
      "  else if (max_val == b) {"
      "    h = 60.0 * (4.0 + (r - g) / (max_val - min_val)); }"
      "  if (h < 0.0) {"
      "    h += 360.0; }"

      "  float s = max_val == 0.0 ? 0.0 : (max_val - min_val) / max_val;"
      "  float v = max_val;"
      "  return vec3 (h, s, v);"
      "  }"
      //}}}
      //{{{
      "vec3 hsvToRgb (float h, float s, float v) {"
      "  float r, g, b;"
      "  float c = v * s;"
      "  float h_ = mod(h / 60.0, 6);" // For convenience, change to multiples of 60
      "  float x = c * (1.0 - abs(mod(h_, 2) - 1));"
      "  float r_, g_, b_;"

      "  if (0.0 <= h_ && h_ < 1.0) {"
      "    r_ = c, g_ = x, b_ = 0.0; }"
      "  else if (1.0 <= h_ && h_ < 2.0) {"
      "    r_ = x, g_ = c, b_ = 0.0; }"
      "  else if (2.0 <= h_ && h_ < 3.0) {"
      "    r_ = 0.0, g_ = c, b_ = x; }"
      "  else if (3.0 <= h_ && h_ < 4.0) {"
      "    r_ = 0.0, g_ = x, b_ = c; }"
      "  else if (4.0 <= h_ && h_ < 5.0) {"
      "    r_ = x, g_ = 0.0, b_ = c; }"
      "  else if (5.0 <= h_ && h_ < 6.0) {"
      "    r_ = c, g_ = 0.0, b_ = x; }"
      "  else {"
      "    r_ = 0.0, g_ = 0.0, b_ = 0.0; }"

      "  float m = v - c;"
      "  r = r_ + m;"
      "  g = g_ + m;"
      "  b = b_ + m;"

      "  return vec3 (r, g, b);"
      "  }"
      //}}}

      "void main() {"
      "  outColor = texture (uSampler, textureCoord);"

      "  if (uHue != 0.0 || uVal != 0.0 || uSat != 0.0) {"
      "    vec3 hsv = rgbToHsv (outColor.x, outColor.y, outColor.z);"
      "    hsv.x += uHue;"
      "    if ((outColor.x != outColor.y) || (outColor.y != outColor.z)) {"
             // not grayscale
      "      hsv.y = uSat <= 0.0 ? "
      "      hsv.y * (1.0 + uSat) : hsv.y + (1.0 - hsv.y) * uSat;"
      "      }"
      "    hsv.z = uVal <= 0.0 ? hsv.z * (1.0 + uVal) : hsv.z + (1.0 - hsv.z) * uVal;"
      "    vec3 rgb = hsvToRgb (hsv.x, hsv.y, hsv.z);"
      "    outColor.xyz = rgb;"
      "    }"

      //"  if (uPreMultiply)"
      //"    outColor.xyz *= outColor.w;"
      "  }";
    //}}}
    //{{{
    const string kPaintFragShader =
      "#version 330 core\n"

      "uniform sampler2D uSampler;"
      "uniform vec2 uPos;"
      "uniform vec2 uPrevPos;"
      "uniform float uRadius;"
      "uniform vec4 uColor;"

      "in vec2 textureCoord;"
      "out vec4 outColor;"

      "float distToLine (vec2 v, vec2 w, vec2 p) {"
      "  float l2 = pow (distance(w, v), 2.);"
      "  if (l2 == 0.0)"
      "    return distance (p, v);"
      "  float t = clamp (dot (p - v, w - v) / l2, 0., 1.);"
      "  vec2 j = v + t * (w - v);"
      "  return distance (p, j);"
      "  }"

      "void main() {"
      "  float dist = distToLine (uPrevPos.xy, uPos.xy, textureCoord * textureSize (uSampler, 0)) - uRadius;"
      "  outColor = mix (uColor, texture (uSampler, textureCoord), clamp (dist, 0.0, 1.0));"
      "  }";
    //}}}
  #endif

  //{{{
  const string kDrawListVertShader120 =
    "#version 100\n"

    "uniform mat4 ProjMtx;"

    "attribute vec2 Position;"
    "attribute vec2 UV;"
    "attribute vec4 Color;"

    "varying vec2 Frag_UV;"
    "varying vec4 Frag_Color;"

    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }";
  //}}}
  //{{{
  const string kDrawListVertShader130 =
    "#version 130\n"

    "uniform mat4 ProjMtx;"

    "in vec2 Position;"
    "in vec2 UV;"
    "in vec4 Color;"

    "out vec2 Frag_UV;"
    "out vec4 Frag_Color;"

    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }";
  //}}}
  //{{{
  const string kDrawListVertShader300es =
    "#version 300 es\n"

    "precision mediump float;"

    "layout (location = 0) in vec2 Position;"
    "layout (location = 1) in vec2 UV;"
    "layout (location = 2) in vec4 Color;"

    "uniform mat4 ProjMtx;"

    "out vec2 Frag_UV;"
    "out vec4 Frag_Color;"

    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }";
  //}}}
  //{{{
  const string kDrawListVertShader410core =
    "#version 410 core\n"

    "layout (location = 0) in vec2 Position;"
    "layout (location = 1) in vec2 UV;"
    "layout (location = 2) in vec4 Color;"

    "uniform mat4 ProjMtx;"

    "out vec2 Frag_UV;"
    "out vec4 Frag_Color;"

    "void main() {"
    "  Frag_UV = UV;"
    "  Frag_Color = Color;"
    "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
    "  }";
  //}}}

  //{{{
  const string kDrawListFragShader120 =
    "#version 100\n"

    "#ifdef GL_ES\n"
    "  precision mediump float;"
    "#endif\n"

    "uniform sampler2D Texture;"

    "varying vec2 Frag_UV;"
    "varying vec4 Frag_Color;"

    "void main() {"
    "  gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);"
    "  }";
  //}}}
  //{{{
  const string kDrawListFragShader130 =
    "#version 130\n"

    "uniform sampler2D Texture;"

    "in vec2 Frag_UV;"
    "in vec4 Frag_Color;"

    "out vec4 Out_Color;"

    "void main() {"
    "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
    "  }";
  //}}}
  //{{{
  const string kDrawListFragShader300es =
    "#version 300 es\n"

    "precision mediump float;"

    "uniform sampler2D Texture;"

    "in vec2 Frag_UV;"
    "in vec4 Frag_Color;"

    "layout (location = 0) out vec4 Out_Color;"

    "void main() {"
    "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
    "  }";
  //}}}
  //{{{
  const string kDrawListFragShader410core =
    "#version 410 core\n"

    "in vec2 Frag_UV;"
    "in vec4 Frag_Color;"

    "uniform sampler2D Texture;"

    "layout (location = 0) out vec4 Out_Color;"

    "void main() {"
    "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
    "  }";
  //}}}
  }

// cShader
//{{{
void cShader::use() {

  glUseProgram (mId);
  }
//}}}

//{{{
cShader::cShader (const string& vertShaderString, const string& fragShaderString) {

  // compile vertShader
  const GLuint vertShader = glCreateShader (GL_VERTEX_SHADER);
  const GLchar* vertShaderStr = vertShaderString.c_str();
  glShaderSource (vertShader, 1, &vertShaderStr, 0);
  glCompileShader (vertShader);
  GLint success;
  glGetShaderiv (vertShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    //{{{  error, exit
    char errMessage[512];
    glGetProgramInfoLog (vertShader, 512, NULL, errMessage);
    cLog::log (LOGERROR, format ("vertShader failed {}", errMessage));

    exit (EXIT_FAILURE);
    }
    //}}}

  // compile fragShader
  const GLuint fragShader = glCreateShader (GL_FRAGMENT_SHADER);
  const GLchar* fragShaderStr = fragShaderString.c_str();
  glShaderSource (fragShader, 1, &fragShaderStr, 0);
  glCompileShader (fragShader);
  glGetShaderiv (fragShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    //{{{  error, exit
    char errMessage[512];
    glGetProgramInfoLog (fragShader, 512, NULL, errMessage);
    cLog::log (LOGERROR, format ("fragShader failed {}", errMessage));
    exit (EXIT_FAILURE);
    }
    //}}}

  // create shader program
  mId = glCreateProgram();
  glAttachShader (mId, vertShader);
  glAttachShader (mId, fragShader);
  glLinkProgram (mId);
  glGetProgramiv (mId, GL_LINK_STATUS, &success);
  if (!success) {
    //{{{  error, exit
    char errMessage[512];
    glGetProgramInfoLog (mId, 512, NULL, errMessage);
    cLog::log (LOGERROR, format ("shaderProgram failed {} ",  errMessage));

    exit (EXIT_FAILURE);
    }
    //}}}

  glDeleteShader (vertShader);
  glDeleteShader (fragShader);
  }
//}}}
//{{{
cShader::~cShader() {
  glDeleteProgram (mId);
  }
//}}}

//{{{
void cShader::setBool (const string &name, bool value) {
  glUniform1i (glGetUniformLocation(mId, name.c_str()), value);
  }
//}}}
//{{{
void cShader::setInt (const string &name, int value) {
  glUniform1i (glGetUniformLocation (mId, name.c_str()), value);
  }
//}}}
//{{{
void cShader::setFloat (const string &name, float value) {
  glUniform1f (glGetUniformLocation (mId, name.c_str()), value);
  }
//}}}
//{{{
void cShader::setVec2 (const string &name, glm::vec2 value) {
  glUniform2fv (glGetUniformLocation (mId, name.c_str()), 1, glm::value_ptr (value));
  }
//}}}
//{{{
void cShader::setVec3 (const string &name, glm::vec3 value) {
  glUniform3fv (glGetUniformLocation (mId, name.c_str()), 1, glm::value_ptr (value));
  }
//}}}
//{{{
void cShader::setVec4 (const string &name, glm::vec4 value) {
  glUniform4fv (glGetUniformLocation (mId, name.c_str()), 1, glm::value_ptr (value));
  }
//}}}
//{{{
void cShader::setMat4 (const string &name, glm::mat4 value) {
  glUniformMatrix4fv (glGetUniformLocation (mId, name.c_str()), 1, GL_FALSE, glm::value_ptr (value));
  }
//}}}

// cQuadShader
cQuadShader::cQuadShader (const string& fragShaderString) : cShader (kQuadVertShader, fragShaderString) {}
//{{{
void cQuadShader::setModelProject (glm::mat4 model, glm::mat4 project) {
  setMat4 ("uModel", model);
  setMat4 ("uProject", project);
  }
//}}}

// cCanvasShader
cCanvasShader::cCanvasShader() : cQuadShader (kCanvasFragShader) {}

// cLayerShader
cLayerShader::cLayerShader() : cQuadShader (kLayerFragShader) {}
//{{{
void cLayerShader::setHueSatVal (float hue, float sat, float val) {

  setFloat ("uHue", 0.f);
  setFloat ("uSat", 0.f);
  setFloat ("uVal", 0.f);
  }
//}}}

// cPaintShader
cPaintShader::cPaintShader() : cQuadShader (kPaintFragShader) {}
//{{{
void cPaintShader::setStroke (glm::vec2 pos, glm::vec2 prevPos, float radius, glm::vec4 color) {

  setVec2 ("uPos", pos);
  setVec2 ("uPrevPos", prevPos);
  setFloat ("uRadius", radius);
  setVec4 ("uColor", color);
  }
//}}}

// cDrawListShader
//{{{  version
//if (gGlslVersion == 120) {
  //vertex_shader = vertexShader120;
  //fragment_shader = fragmentShader120;
  //}
//else if (gGlslVersion == 300) {
  //vertex_shader = vertexShader300es;
  //fragment_shader = fragmentShader300es;
  //}
//else if (gGlslVersion >= 410) {
  //vertex_shader = vertexShader410core;
  //fragment_shader = fragmentShader410core;
  //}
//else {
  //vertex_shader = vertexShader130;
  //fragment_shader = fragmentShader130;
  //}
//}}}
//{{{
cDrawListShader::cDrawListShader (uint32_t glslVersion)
    : cShader (kDrawListVertShader130, kDrawListFragShader130) {

  // store uniform locations
  mAttribLocationTexture = glGetUniformLocation (getId(), "Texture");
  mAttribLocationProjMtx = glGetUniformLocation (getId(), "ProjMtx");

  mAttribLocationVtxPos = glGetAttribLocation (getId(), "Position");
  mAttribLocationVtxUV = glGetAttribLocation (getId(), "UV");
  mAttribLocationVtxColor = glGetAttribLocation (getId(), "Color");
  }
//}}}
//{{{
void cDrawListShader::setMatrix (float* matrix) {
  glUniformMatrix4fv (mAttribLocationProjMtx, 1, GL_FALSE, matrix);
  };
//}}}
