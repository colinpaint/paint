// main.cpp - paintbox main - use glm, imGui, stb
// - brushes,ui use static var init idiom to register to their man(agers)
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <vector>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "platform/cPlatform.h"
#include "graphics/cGraphics.h"
#include "canvas/cLayer.h"
#include "canvas/cCanvas.h"
#include "ui/cUI.h"
#include "log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  void windowResized (cGraphics* graphics, int width, int height) {
  // platform windowResized callback, !!! should be lambda !!!
    graphics->windowResized (width, height);
    }
  }

int main (int numArgs, char* args[]) {
  //{{{  args to params
  vector <string> params;
  for (int i = 1; i < numArgs; i++)
    params.push_back (args[i]);
  //}}}
  eLogLevel logLevel = LOGINFO;
    //{{{  parse params
    for (auto it = params.begin(); it < params.end(); ++it) {
      if (*it == "log1") { logLevel = LOGINFO1; params.erase (it); }
      else if (*it == "log2") { logLevel = LOGINFO2; params.erase (it); }
      else if (*it == "log3") { logLevel = LOGINFO3; params.erase (it); }
      }
    //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, "paintbox");

  // start platform singleton, !!! does anybody else need a platform pointer, keystrokes use imGui !!!
  cPlatform& platform = cPlatform::getInstance();
  if (!platform.init (cPoint(1200, 800), false))
    exit (EXIT_FAILURE);

  // start graphics singleton, !!! maybe possible to send graphics pointer through cUIMan::draw !!!
  cGraphics& graphics = cGraphics::create();
  if (!graphics.init (platform.getDevice(), platform.getDeviceContext(), platform.getSwapChain()))
    exit (EXIT_FAILURE);
  platform.setSizeCallback (&graphics, windowResized);

  // start canvas
  cCanvas canvas (params.empty() ? "../piccies/tv.jpg" : params[0], graphics);
  if (params.size() > 1)
    canvas.newLayer (params[1]);

  // main UI loop
  while (platform.pollEvents()) {
    platform.newFrame();
    cUI::draw (canvas, graphics);
    platform.present();
    }

  graphics.shutdown();
  platform.shutdown();

  return EXIT_SUCCESS;
  }
