// main.cpp - paintbox main - use glm, imGui, stb
// - brushes,ui use static var init idiom to register to their man(agers)
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "platform/cPlatform.h"
#include "graphics/cGraphics.h"
#include "brush/cBrush.h"
#include "canvas/cLayer.h"
#include "canvas/cCanvas.h"
#include "ui/cUI.h"
#include "log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

int main (int numArgs, char* args[]) {

  // default params
  eLogLevel logLevel = LOGINFO;
  string platformString = "glfw";
  string graphicsString = "opengl";
  //{{{  args to params
  vector <string> params;
  for (int i = 1; i < numArgs; i++)
    params.push_back (args[i]);
  //}}}
  //{{{  parse params, removing recognised options
  for (auto it = params.begin(); it < params.end();) {
    if (*it == "log1") { logLevel = LOGINFO1; params.erase (it); }
    else if (*it == "log2") { logLevel = LOGINFO2; params.erase (it); }
    else if (*it == "log3") { logLevel = LOGINFO3; params.erase (it); }
    else if (*it == "win32") { platformString = *it; params.erase (it); }
    else if (*it == "directx") { graphicsString = *it; params.erase (it); }
    else ++it;
    };
  //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, format ("paintbox {} {}", platformString, graphicsString));

  cUI::listClasses();
  cBrush::listClasses();
  cPlatform::listClasses();
  cGraphics::listClasses();

  // create and start platform
  cPlatform& platform = cPlatform::createByName (platformString);
  if (!platform.init (cPoint(1200, 800), false))
    exit (EXIT_FAILURE);

  // create and start graphics
  cGraphics& graphics = cGraphics::createByName (graphicsString);
  if (!graphics.init (platform.getDevice(), platform.getDeviceContext(), platform.getSwapChain()))
    exit (EXIT_FAILURE);

  // create canvas
  cCanvas canvas (params.empty() ? "../piccies/tv.jpg" : params[0], graphics);
  if (params.size() > 1)
    canvas.newLayer (params[1]);

  // set resizeCallback lambda
  platform.setResizeCallback (
    [&](int width, int height) noexcept {
      graphics.windowResize (width, height);
      platform.newFrame();
      cUI::draw (canvas, graphics);
      platform.present();
      }
    );

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
