// cPaintGpuBrush.cpp
//{{{  includes
#include "cPaintGpuBrush.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// glm
#include <vec2.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>
#include <gtc/matrix_transform.hpp>

#include "../graphics/cPointRect.h"
#include "../graphics/cQuad.h"
#include "../graphics/cShader.h"
#include "../graphics/cFrameBuffer.h"

#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

//{{{
cPaintGpuBrush::cPaintGpuBrush (const string& className, float radius) : cBrush(className, radius) {
  mShader = new cPaintShader();
  setRadius (radius);
  }
//}}}
//{{{
cPaintGpuBrush::~cPaintGpuBrush() {
  delete mShader;
  }
//}}}

void cPaintGpuBrush::paint (glm::vec2 pos, bool first, cFrameBuffer* frameBuffer, cFrameBuffer* frameBuffer1) {

  if (first)
    mPrevPos = pos;

  cRect boundRect = getBoundRect (pos, frameBuffer);
  if (!boundRect.isEmpty()) {
    const float widthF = static_cast<float>(frameBuffer->getSize().x);
    const float heightF = static_cast<float>(frameBuffer->getSize().y);

    // target, boundRect probably ignored for frameBuffer1
    frameBuffer1->setTarget (boundRect);
    frameBuffer1->invalidate();
    frameBuffer1->setBlend();

    // shader
    mShader->use();
    mShader->setModelProject (glm::mat4 (1.f), glm::ortho (0.f,widthF, 0.f,heightF, -1.f,1.f));
    mShader->setStroke (pos, mPrevPos, getRadius(), getColor());

    // source
    frameBuffer->setSource();
    frameBuffer->checkStatus();
    //frameBuffer->reportInfo();

    // draw boundRect to frameBuffer1 target
    cQuad quad (frameBuffer->getSize(), boundRect);
    quad.draw();

    // blit boundRect frameBuffer1 back to frameBuffer
    frameBuffer->blit (frameBuffer1, boundRect.getTL(), boundRect);
    }

  mPrevPos = pos;
  }
