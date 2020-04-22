/*
 * webgl.h
 *
 *  Created on: Dec 13, 2011
 *      Author: ngk437
 */

#ifndef _MAGICLEAP_H_
#define _MAGICLEAP_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include <defines.h>
#include <dlfcn.h>
#include <cmath>
#include <vector>
#include <list>
#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <webgl.h>
#include <egl/include/egl.h>
#include <ml_graphics.h>
#include <ml_head_tracking.h>
#include <ml_hand_tracking.h>
#include <ml_perception.h>
#include <ml_snapshot.h>
#include <ml_meshing2.h>
#include <ml_planes.h>
#include <ml_camera.h>
#include <ml_image_tracking.h>
#include <ml_eye_tracking.h>
#include <ml_privilege_ids.h>
#include <ml_privilege_functions.h>
#include <ml_input.h>
#include <ml_raycast.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>

#include "sjpeg.h"

using namespace v8;
using namespace node;

#define MAX_NUM_PLANES (32)
#define PLANE_ENTRY_SIZE (3 + 4 + 2 + 1)
#define CONTROLLER_ENTRY_SIZE (1 + 3 + 4 + 6)
#define CAMERA_REQUEST_PLANE_BUFFER_SIZE (5 * 1024 * 1024)

namespace ml {

// enums

enum DummyValue {
  STOPPED,
  RUNNING,
  PAUSED,
};

/* enum Event {
  NEW_INIT_ARG,
  STOP,
  PAUSE,
  RESUME,
  UNLOAD_RESOURCES,
};

enum KeyboardEventType {
  CHAR,
  KEY_DOWN,
  KEY_UP,
}; */

void RunResInMainThread(uv_async_t *handle);

// classes

class MLContext;

struct application_context_t {
  int dummy_value;
  MLContext *mlContext;
  NATIVEwindow *window;
};

class EventHandler {
public:
  EventHandler(uv_async_t *async, Local<Function> handlerFn);

  uv_async_t *async;
  Nan::Persistent<Function> handlerFn;
  std::deque<std::function<void(std::function<void(int argc, Local<Value> *argv)>)>> fns;
};

/* class KeyboardEvent {
public:
  KeyboardEvent(KeyboardEventType type, uint32_t char_utf32);
  KeyboardEvent(KeyboardEventType type, MLKeyCode key_code, uint32_t modifier_mask);

  KeyboardEventType type;
  uint32_t char_utf32;
  MLKeyCode key_code;
  uint32_t modifier_mask;
}; */

void RunResInMainThread(uv_async_t *handle);

/* class MLCallback {
public:
  MLCallback(uv_loop_t *loop, std::function<void()> fn);
  ~MLCallback();

  static void RunInAsyncThread(uv_async_t *handle);

// protected:
  std::unique_ptr<uv_async_t> async;
  std::function<void()> fn;
}; */

/* class MLPoll {
public:
  Nan::Persistent<Object> windowObj;
  std::function<void()> cb;

  MLPoll(Local<Object> windowObj, std::function<void()> cb);
  ~MLPoll();
}; */

class MLRaycaster : public ObjectWrap {
public:
  static Local<Function> Initialize(Isolate *isolate);

  MLRaycaster(const MLVec3f &position, const MLVec3f &direction);
  ~MLRaycaster();

  static NAN_METHOD(New);
  static NAN_METHOD(WaitGetPoses);

// protected:
  static MLHandle tracker;
  static bool hasTracker;
  MLRaycastQuery raycastQuery;
  MLHandle requestHandle;
  // Nan::Persistent<Function> cb;
};

/* class MeshBuffer {
public:
  MeshBuffer();
  MeshBuffer(MLTransform transform, float *positions, uint32_t numPositions, float *normals, uint16_t *indices, uint16_t numIndices);

  MLTransform transform;
  float *positions;
  uint32_t numPositions;
  float *normals;
  uint16_t *indices;
  uint16_t numIndices;
};

enum class MLUpdateType {
  NEW,
  UNCHANGED,
  UPDATE,
  REMOVE,
}; */

class MLMesher : public ObjectWrap {
public:
  static Local<Function> Initialize(Isolate *isolate);

  MLMesher(float range, MLMeshingLOD lod);
  ~MLMesher();

  static NAN_METHOD(New);
  static NAN_METHOD(WaitGetPoses);
  /* static NAN_GETTER(OnMeshGetter);
  static NAN_SETTER(OnMeshSetter); */
  static NAN_METHOD(Destroy);

// protected:
  /* Nan::Persistent<Object> windowObj;
  uv_loop_t *loop; */
  float range;
  MLMeshingLOD lod;
  MLHandle tracker;
  bool meshInfoRequestPending;
  bool meshRequestsPending;
  bool meshRequestPending;
  uint32_t meshBlockRequestIndex;
  MLHandle meshInfoRequestHandle;
  MLHandle meshRequestHandle;;
  MLMeshingMeshInfo meshInfo;
  MLMeshingMeshRequest meshRequest;
  std::map<std::string, MLMeshingBlockInfo *> meshInfoMap;
  // Nan::Persistent<Function> cb;
};

class MLPlaneTracker : public ObjectWrap {
public:
  static Local<Function> Initialize(Isolate *isolate);

  MLPlaneTracker(float range);
  ~MLPlaneTracker();

  static NAN_METHOD(New);
  static NAN_METHOD(WaitGetPoses);
  static NAN_METHOD(Destroy);

// protected:
  float range;
  MLHandle tracker;
  MLHandle planesRequestHandle;
  bool planesRequestPending;
  std::vector<std::string> planeIds;
  /* Nan::Persistent<Object> windowObj;
  uv_loop_t *loop;
  Nan::Persistent<Function> cb; */
};

class MLHandTracker : public ObjectWrap {
public:
  static Local<Function> Initialize(Isolate *isolate);

  MLHandTracker();
  ~MLHandTracker();

  static NAN_METHOD(New);
  static NAN_METHOD(WaitGetPoses);
  static NAN_METHOD(Destroy);

// protected:
  MLHandle tracker;
  /* Nan::Persistent<Object> windowObj;
  Nan::Persistent<Function> cb;
  uv_loop_t *loop;
  Nan::Persistent<Function> ongesture; */
};

class MLEyeTracker : public ObjectWrap {
public:
  static Local<Function> Initialize(Isolate *isolate);

  MLEyeTracker();
  ~MLEyeTracker();

  static NAN_METHOD(New);
  static NAN_METHOD(WaitGetPoses);
  static NAN_METHOD(Destroy);

// protected:
  MLHandle tracker;
  /* Nan::Persistent<Object> windowObj;
  MLTransform transform;
  MLTransform leftTransform;
  bool leftBlink;
  MLTransform rightTransform;
  bool rightBlink; */
};

class CameraRequest {
public:
  CameraRequest(uv_loop_t *loop, Local<Function> cbFn);
  void Set(int width, int height, uint8_t *data, size_t size);
  void Update();

// protected:
  uv_loop_t *loop;
  Nan::Persistent<Function> cbFn;
  int width;
  int height;
  Nan::Persistent<ArrayBuffer> data;
};

class MLImageTracker : public ObjectWrap {
public:
  static Local<Function> Initialize(Isolate *isolate);

  MLImageTracker(Local<Object> windowObj, MLHandle trackerHandle, float size);
  ~MLImageTracker();

  static NAN_METHOD(New);
  static NAN_GETTER(OnTrackGetter);
  static NAN_SETTER(OnTrackSetter);
  static NAN_METHOD(Destroy);

  void Update(MLSnapshot *snapshot);

// protected:
  Nan::Persistent<Object> windowObj;
  MLHandle trackerHandle;
  float size;
  uv_loop_t *loop;
  Nan::Persistent<Function> cb;
  MLTransform transform;
  bool valid;
};

class MLContext : public ObjectWrap {
public:
  static Local<Object> Initialize(Isolate *isolate);

// protected:
  MLContext();
  ~MLContext();

  static NAN_METHOD(New);
  static NAN_METHOD(InitLifecycle);
  static NAN_METHOD(DeinitLifecycle);
  static NAN_METHOD(SetEventHandler);
  static NAN_METHOD(Present);
  static NAN_METHOD(Exit);
  static NAN_METHOD(WaitGetPoses);
  static NAN_METHOD(RequestGetPoses);
  // static NAN_METHOD(PrepareFrame);
  static NAN_METHOD(SubmitFrame);
  static NAN_METHOD(IsPresent);
  static NAN_METHOD(IsSimulated);
  static NAN_METHOD(GetSize);
  // static NAN_METHOD(SetContentTexture);
  static NAN_METHOD(RequestMeshing);
  static NAN_METHOD(RequestPlaneTracking);
  static NAN_METHOD(RequestHitTest);
  static NAN_METHOD(RequestHandTracking);
  static NAN_METHOD(RequestEyeTracking);
  static NAN_METHOD(RequestImageTracking);
  // static NAN_METHOD(RequestDepthPopulation);
  // static NAN_METHOD(RequestCamera);
  // static NAN_METHOD(CancelCamera);
  static NAN_METHOD(Update);
  // static NAN_METHOD(Poll);

  void TickFloor();
  static MLVec3f OffsetFloor(const MLVec3f &position);

// protected:
  // EGL
  NATIVEwindow *window;

  // tracking
  MLHandle graphics_client;
  MLGraphicsRenderTargetsInfo render_targets_info;
  GLuint dst_framebuffer_id;
  MLHandle frame_handle;
  MLHandle head_tracker;
  MLHeadTrackingStaticData head_static_data;
  MLGraphicsVirtualCameraInfoArray virtual_camera_array;

  // position
  std::mutex positionMutex;
  MLVec3f position;
  MLQuaternionf rotation;

  // input
  MLHandle inputTracker;

  // meshing
  GLuint meshVao;
  GLuint meshVertex;
  GLuint meshFragment;
  GLuint meshProgram;
  GLint positionLocation;
  // GLint normalLocation;
  GLint modelViewMatrixLocation;
  GLint projectionMatrixLocation;

  // camera
  /* GLuint cameraVao;
  GLuint cameraVertex;
  GLuint cameraFragment;
  GLuint cameraProgram;
  GLint pointLocation;
  GLint uvLocation;
  GLint cameraInTextureLocation;
  GLint contentTextureLocation;
  GLuint pointBuffer;
  GLuint uvBuffer; */

  GLuint cameraInTexture;
  // GLuint contentTexture;
  GLuint cameraOutTexture;
  GLuint cameraFbo;

  // occlusion
  // MLHandle occlusionTracker;

  Nan::Persistent<Function> mlMesherConstructor;
  Nan::Persistent<Function> mlPlaneTrackerConstructor;
  Nan::Persistent<Function> mlRaycastTrackerConstructor;
  Nan::Persistent<Function> mlHandTrackerConstructor;
  Nan::Persistent<Function> mlEyeTrackerConstructor;
  Nan::Persistent<Function> mlImageTrackerConstructor;
};

}

Local<Object> makeMl();

#endif
