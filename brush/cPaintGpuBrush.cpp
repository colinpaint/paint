// cPaintGpuBrush.cpp
//{{{  includes
// glm
#include <gtc/matrix_transform.hpp>

#include "cBrush.h"
#include "../graphics/cGraphics.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

class cPaintGpuBrush : public cBrush {
public:
  //{{{
  cPaintGpuBrush (const string& className, float radius, cGraphics& graphics)
      : cBrush(className, radius), mGraphics(graphics) {

    mShader = graphics.createPaintShader();
    setRadius (radius);
    }
  //}}}
  //{{{
  ~cPaintGpuBrush() {
    delete mShader;
    }
  //}}}

  //{{{
  void paint (glm::vec2 pos, bool first, cFrameBuffer* frameBuffer, cFrameBuffer* frameBuffer1) {

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
      cQuad* quad = mGraphics.createQuad (frameBuffer->getSize(), boundRect);
      quad->draw();
      delete quad;

      // blit boundRect frameBuffer1 back to frameBuffer
      frameBuffer->blit (frameBuffer1, boundRect.getTL(), boundRect);
      }

    mPrevPos = pos;
    }
  //}}}

private:
  //{{{
  static cBrush* createBrush (const std::string& className, float radius, cGraphics& graphics) {
    return new cPaintGpuBrush (className, radius, graphics);
    }
  //}}}

  // register brush with its static manager
  inline static const bool mRegistered = registerClass ("paintGpu", &createBrush);

  cPaintShader* mShader = nullptr;
  cGraphics& mGraphics;
  };
