/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <webgl.h>
#include <ml_logging.h>
#include <dlfcn.h>
// #include <string.h>

// forward declarations

class Servo2D;
typedef struct Opaque ServoInstance;
typedef void (*MLLogger)(MLLogLevel lvl, char* msg);
typedef void (*MLHistoryUpdate)(Servo2D* app, bool canGoBack, char* url, bool canGoForward);

extern "C" ServoInstance *(*init_servo)(EGLContext, EGLSurface, EGLDisplay,
                                     Servo2D*, MLLogger, MLHistoryUpdate,
                                     const char* url, int width, int height, float hidpi);
extern "C" void (*heartbeat_servo)(ServoInstance*);
extern "C" void (*trigger_servo)(ServoInstance*, float x, float y, bool down);
extern "C" void (*move_servo)(ServoInstance*, float x, float y);
extern "C" void (*traverse_servo)(ServoInstance*, int delta);
extern "C" void (*navigate_servo)(ServoInstance*, const char* text);
extern "C" void (*discard_servo)(ServoInstance*);

/**
 * Servo2D Landscape Application
 */
class Servo2D {
public:

  /**
   * Constructs the Landscape Application.
   */
  Servo2D();

  /**
   * Destroys the Landscape Application.
   */
  ~Servo2D();

  /**
   * Disallows the copy constructor.
   */
  Servo2D(const Servo2D&) = delete;

  /**
   * Disallows the move constructor.
   */
  Servo2D(Servo2D&&) = delete;

  /**
   * Disallows the copy assignment operator.
   */
  Servo2D& operator=(const Servo2D&) = delete;

  /**
   * Disallows the move assignment operator.
   */
  Servo2D& operator=(Servo2D&&) = delete;

// protected:
  /**
   * Initializes the Landscape Application.
   * @return - 0 on success, error code on failure.
   */
  int init(
    const std::string &url,
    NATIVEwindow *window,
    GLuint tex,
    int width,
    int height,
    std::function<void()> onloadstart,
    std::function<void(const std::string &)> onloadend,
    std::function<void(int, const std::string &, const std::string &)> onloaderror,
    std::function<void(const std::string &, const std::string &, int)> onconsole,
    std::function<void(const std::string &)> onmessage
  );

  /**
   * Deinitializes the Landscape Application.
   * @return - 0 on success, error code on failure.
   */
  int deInit();
  
  int getWidth() const;
  void setWidth(int width);
  int getHeight() const;
  void setHeight(int height);
  
  ServoInstance *getInstance() const;
  void flushTexture() const;
  
  static void init();

private:
  std::string url;
  GLuint tex;
  int width;
  int height;
  std::function<void()> onloadstart;
  std::function<void(const std::string &)> onloadend;
  std::function<void(int, const std::string &, const std::string &)> onloaderror;
  std::function<void(const std::string &, const std::string &, int)> onconsole;
  std::function<void(const std::string &)> onmessage;
  
  GLuint fboOutCache;
  
  ServoInstance *servo_;
  EGLDisplay display;
  EGLSurface surface;
};