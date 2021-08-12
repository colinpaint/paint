// cShader.h - simple shader wrapper
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// glm
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>
//}}}

// base classes
//{{{
class cShader {
public:
  cShader (const std::string& vertShaderString, const std::string& fragShaderString);
  virtual ~cShader();

  uint32_t getId() { return mId; }

  void use();

protected:
  void setBool (const std::string &name, bool value);

  void setInt (const std::string &name, int value);
  void setFloat (const std::string &name, float value);

  void setVec2 (const std::string &name, glm::vec2 value);
  void setVec3 (const std::string &name, glm::vec3 value);
  void setVec4 (const std::string &name, glm::vec4 value);

  void setMat4 (const std::string &name, glm::mat4 value);

private:
  uint32_t mId = 0;
  };
//}}}
//{{{
class cQuadShader : public cShader {
public:
  cQuadShader (const std::string& fragShaderString);
  virtual ~cQuadShader() = default;

  void setModelProject (const glm::mat4& model, const glm::mat4& project);
  };
//}}}

// real classes
//{{{
class cCanvasShader : public cQuadShader {
public:
  cCanvasShader();
  virtual ~cCanvasShader() = default;
  };
//}}}
//{{{
class cLayerShader : public cQuadShader {
public:
  cLayerShader();
  virtual ~cLayerShader() = default;

  void setHueSatVal (float hue, float sat, float val);
  };
//}}}
//{{{
class cPaintShader : public cQuadShader {
public:
  cPaintShader();
  virtual ~cPaintShader() = default;

  void setStroke (const glm::vec2& pos, const glm::vec2& prevPos, float radius, const glm::vec4& color);
  };
//}}}

//{{{
class cDrawListShader : public cShader {
public:
  cDrawListShader (uint32_t glslVersion);
  virtual ~cDrawListShader() = default;

  int32_t getAttribLocationVtxPos() { return mAttribLocationVtxPos; }
  int32_t getAttribLocationVtxUV() { return mAttribLocationVtxUV; }
  int32_t getAttribLocationVtxColor() { return mAttribLocationVtxColor; }

  void setMatrix (float* matrix);

private:
  int32_t mAttribLocationTexture = 0;
  int32_t mAttribLocationProjMtx = 0;

  int32_t mAttribLocationVtxPos = 0;
  int32_t mAttribLocationVtxUV = 0;
  int32_t mAttribLocationVtxColor = 0;
  };
//}}}
