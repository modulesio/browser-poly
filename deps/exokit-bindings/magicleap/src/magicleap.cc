#if defined(LUMIN)

#include <magicleap.h>
#include <ml-math.h>
#include <windowsystem.h>
#include <image-context.h>

#include <uv.h>
#include <exout>

using namespace v8;
using namespace std;

namespace ml {

const char application_name[] = "com.exokit.app";
constexpr int CAMERA_SIZE[] = {960, 540};
constexpr float planeRange = 10.0f;

application_context_t application_context;
MLResult lifecycle_status = MLResult_Pending;

MLLifecycleCallbacks lifecycle_callbacks;
MLInputKeyboardCallbacks keyboardCallbacks;
MLCameraDeviceStatusCallbacks cameraDeviceStatusCallbacks;
MLCameraCaptureCallbacks cameraCaptureCallbacks;

/* Nan::Persistent<Function> eventsCb;
std::vector<Event> events;
uv_async_t eventsAsync;

Nan::Persistent<Function> keyboardEventsCb;
std::vector<KeyboardEvent> keyboardEvents;
uv_async_t keyboardEventsAsync; */

EventHandler *eventHandler = nullptr;
std::mutex eventHandlerMutex;

EventHandler::EventHandler(uv_async_t *async, Local<Function> handlerFn) : async(async), handlerFn(handlerFn) {}

void QueueEvent(std::function<void(std::function<void(int, Local<Value> *)>)> fn) {
  {
    std::lock_guard<std::mutex> lock(eventHandlerMutex);

    eventHandler->fns.push_back(fn);
  }

  uv_async_send(eventHandler->async);
}

void QueueCallback(uv_loop_t *loop, std::function<void()> fn) {
  // XXX
  /* MLCallback *mlCallback = new MLCallback(loop, fn);

  uv_async_send(mlCallback->async.get()); */
}

// std::list<MLPoll *> polls;

/* std::condition_variable  cameraRequestConditionVariable;
std::vector<CameraRequest *> cameraRequests;
uv_async_t cameraAsync;
bool cameraConvertPending = false;
uint8_t cameraRequestRgb[CAMERA_SIZE[0] * CAMERA_SIZE[1] * 3];
uint8_t *cameraRequestJpeg;
size_t cameraRequestSize;
uv_sem_t cameraConvertSem;
uv_async_t cameraConvertAsync; */

MLHandle imageTrackerHandle;
MLImageTrackerSettings imageTrackerSettings;
std::vector<MLImageTracker *> imageTrackers;

/* MLHandle handTracker;
MLHandTrackingData handData;
MLHandTrackingKeyPose lastKeyposeLeft = MLHandTrackingKeyPose_NoHand;
MLHandTrackingKeyPose lastKeyposeRight = MLHandTrackingKeyPose_NoHand;
MLHandTrackingStaticData handStaticData;
std::vector<MLHandTracker *> handTrackers;
bool handPresents[2];
float wristBones[2][4][1 + 3];
float fingerBones[2][5][4][1 + 3]; */

MLHandle floorTracker;
MLPlanesQuery floorRequest;
MLHandle floorRequestHandle;
MLPlane floorResults[MAX_NUM_PLANES];
uint32_t numFloorResults;
bool floorRequestPending = false;
float largestFloorY = 0;
float largestFloorSizeSq = 0;

/* MLHandle meshTracker;
std::vector<MLMesher *> meshers;
MLMeshingExtents meshExtents;
MLHandle meshInfoRequestHandle;
MLMeshingMeshInfo meshInfo;
bool meshInfoRequestPending = false;
std::vector<MLMeshingBlockRequest> meshBlockRequests(128);
uint32_t numMeshBlockRequests = 0;
uint32_t meshBlockRequestIndex = 0;
MLMeshingMeshRequest meshRequest;
std::map<std::string, bool> meshRequestNewMap;
std::map<std::string, bool> meshRequestRemovedMap;
std::map<std::string, bool> meshRequestUnchangedMap;
MLHandle meshRequestHandle;
MLMeshingMesh mesh;
bool meshRequestsPending = false;
bool meshRequestPending = false;

std::map<std::string, MeshBuffer> meshBuffers; */

/* MLHandle planesTracker;
std::vector<MLPlaneTracker *> planeTrackers;
MLPlanesQuery planesRequest;
MLHandle planesRequestHandle;
MLPlane planeResults[MAX_NUM_PLANES];
uint32_t numPlanesResults;
bool planesRequestPending = false; */

bool depthEnabled = false;

bool isPresent() {
  return lifecycle_status == MLResult_Ok;
}

bool isSimulated() {
#ifdef LUMIN
  return false;
#else
  return true;
#endif
}

std::string id2String(const MLCoordinateFrameUID &id) {
  uint64_t id1 = id.data[0];
  uint64_t id2 = id.data[1];
  char idbuf[16*2 + 1];
  sprintf(idbuf, "%016llx%016llx", id1, id2);
  return std::string(idbuf);
}
std::string id2String(const uint64_t &id) {
  char idbuf[16 + 1];
  sprintf(idbuf, "%016llx", id);
  return std::string(idbuf);
}

/* void makePlanesQueryer(MLHandle &planesHandle) {
  if (MLPlanesCreate(&planesHandle) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to create planes handle.", application_name);
  }
}
void beginPlanesQuery(const MLVec3f &position, const MLQuaternionf &rotation, MLHandle &planesHandle, MLHandle &planesQueryHandle, MLPlanesQueryFlags flags) {
  if (!MLHandleIsValid(planesQueryHandle)) {
    MLPlanesQuery query;
    query.flags = flags;
    // query.bounds_center = position;
    // query.bounds_rotation = rotation;
    query.bounds_center.x = 0;
    query.bounds_center.y = 0;
    query.bounds_center.z = 0;
    query.bounds_rotation.x = 0;
    query.bounds_rotation.y = 0;
    query.bounds_rotation.z = 0;
    query.bounds_rotation.w = 1;
    query.bounds_extents.x = 10;
    query.bounds_extents.y = 10;
    query.bounds_extents.z = 10;
    query.min_hole_length = 0.5;
    query.min_plane_area = 0.25;
    query.max_results = MAX_NUM_PLANES;

    if (MLPlanesQueryBegin(planesHandle, &query, &planesQueryHandle) != MLResult_Ok) {
      ML_LOG(Error, "%s: Failed to query planes.", application_name);
    }
  }
}
void endPlanesQuery(MLHandle &planesHandle, MLHandle &planesQueryHandle, MLPlane *planes, uint32_t *numPlanes) {
  if (MLHandleIsValid(planesQueryHandle)) {
    MLResult result = MLPlanesQueryGetResultsWithBoundaries(planesHandle, planesQueryHandle, planes, numPlanes);
    if (result == MLResult_Ok) {
      planesQueryHandle = ML_INVALID_HANDLE;
    } else if (result == MLResult_UnspecifiedFailure) {
      planesQueryHandle = ML_INVALID_HANDLE;

      ML_LOG(Error, "MLPlanesQueryGetResultsWithBoundaries failed: %d", result);
    } else if (result == MLResult_Pending) {
      // nothing, we wait
    } else {
      ML_LOG(Error, "MLPlanesQueryGetResultsWithBoundaries complained: %d", result);
    }
  }
}
void readPlanesQuery(MLPlane *planes, uint32_t numPlanes, int planeType, Local<Float32Array> &planesArray, uint32_t *planesIndex) {
  for (uint32_t i = 0; i < numPlanes; i++) {
    const MLPlane &plane = planes[i];
    uint32_t baseIndex = (*planesIndex) * PLANE_ENTRY_SIZE;
    planesArray->Set(baseIndex + 0, JS_NUM(plane.position.x));
    planesArray->Set(baseIndex + 1, JS_NUM(plane.position.y));
    planesArray->Set(baseIndex + 2, JS_NUM(plane.position.z));
    planesArray->Set(baseIndex + 3, JS_NUM(plane.rotation.x));
    planesArray->Set(baseIndex + 4, JS_NUM(plane.rotation.y));
    planesArray->Set(baseIndex + 5, JS_NUM(plane.rotation.z));
    planesArray->Set(baseIndex + 6, JS_NUM(plane.rotation.w));
    planesArray->Set(baseIndex + 7, JS_NUM(plane.width));
    planesArray->Set(baseIndex + 8, JS_NUM(plane.height));
    planesArray->Set(baseIndex + 9, JS_INT(planeType));

   (*planesIndex)++;
  }
}
inline int gestureCategoryToIndex(MLGestureStaticHandState gesture) {
  if (gesture & MLGestureStaticHandState_NoHand) {
    return 0;
  } else if (gesture & MLGestureStaticHandState_Finger) {
    return 1;
  } else if (gesture & MLGestureStaticHandState_Fist) {
    return 2;
  } else if (gesture & MLGestureStaticHandState_Pinch) {
    return 3;
  } else if (gesture & MLGestureStaticHandState_Thumb) {
    return 4;
  } else if (gesture & MLGestureStaticHandState_L) {
    return 5;
  } else if (gesture & MLGestureStaticHandState_OpenHandBack) {
    return 6;
  } else if (gesture & MLGestureStaticHandState_Ok) {
    return 7;
  } else if (gesture & MLGestureStaticHandState_C) {
    return 8;
  } else {
    return -1;
  }
} */
inline const char *gestureCategoryToDescriptor(MLHandTrackingKeyPose keyPose) {
  switch (keyPose) {
    case MLHandTrackingKeyPose_NoHand:
      return nullptr;
    case MLHandTrackingKeyPose_Finger:
      return "finger";
    case MLHandTrackingKeyPose_Fist:
      return "fist";
    case MLHandTrackingKeyPose_Pinch:
      return "pinch";
    case MLHandTrackingKeyPose_Thumb:
      return "thumb";
    case MLHandTrackingKeyPose_L:
      return "l";
    case MLHandTrackingKeyPose_OpenHandBack:
      return "openHandBack";
    case MLHandTrackingKeyPose_Ok:
      return "ok";
    case MLHandTrackingKeyPose_C:
      return "c";
    default:
      return nullptr;
  }
}
inline Local<Value> gestureCategoryToJsValue(MLHandTrackingKeyPose keyPose) {
  const char *gesture = gestureCategoryToDescriptor(keyPose);
  if (gesture) {
    return JS_STR(gesture);
  } else {
    return Nan::Null();
  }
}

static void onNewInitArg(void* application_context) {
  MLLifecycleInitArgList *args;
  MLLifecycleGetInitArgList(&args);

  // ((struct application_context_t*)application_context)->dummy_value = DummyValue::RUNNING;
  ML_LOG(Info, "%s: On new init arg called %x.", application_name, args);

  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("newInitArg"));

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}

static void onStop(void* application_context) {
  // ((struct application_context_t*)application_context)->dummy_value = DummyValue::STOPPED;
  ML_LOG(Info, "%s: On stop called.", application_name);

  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("stop"));

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}

static void onPause(void* application_context) {
  // ((struct application_context_t*)application_context)->dummy_value = DummyValue::PAUSED;
  ML_LOG(Info, "%s: On pause called.", application_name);

  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("pause"));

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}

static void onResume(void* application_context) {
  // ((struct application_context_t*)application_context)->dummy_value = DummyValue::RUNNING;
  ML_LOG(Info, "%s: On resume called.", application_name);

  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("resume"));

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}

static void onUnloadResources(void* application_context) {
  // ((struct application_context_t*)application_context)->dummy_value = DummyValue::STOPPED;
  ML_LOG(Info, "%s: On unload resources called.", application_name);

  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("unloadResources"));

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}

MLMat4f getWindowTransformMatrix(Local<Object> windowObj, bool inverse = true) {
  Local<Object> localDocumentObj = Local<Object>::Cast(windowObj->Get(JS_STR("document")));
  Local<Value> xrOffsetValue = localDocumentObj->Get(JS_STR("xrOffset"));

  if (xrOffsetValue->IsObject()) {
    Local<Object> xrOffsetObj = Local<Object>::Cast(xrOffsetValue);
    Local<Float32Array> positionFloat32Array = Local<Float32Array>::Cast(xrOffsetObj->Get(JS_STR("position")));
    MLVec3f position;
    memcpy(position.values, (char *)positionFloat32Array->Buffer()->GetContents().Data() + positionFloat32Array->ByteOffset(), sizeof(position.values));
    position.y += largestFloorY;

    Local<Float32Array> orientationFloat32Array = Local<Float32Array>::Cast(xrOffsetObj->Get(JS_STR("orientation")));
    MLQuaternionf orientation;
    memcpy(orientation.values, (char *)orientationFloat32Array->Buffer()->GetContents().Data() + orientationFloat32Array->ByteOffset(), sizeof(orientation.values));

    Local<Float32Array> scaleFloat32Array = Local<Float32Array>::Cast(xrOffsetObj->Get(JS_STR("scale")));
    MLVec3f scale;
    memcpy(scale.values, (char *)scaleFloat32Array->Buffer()->GetContents().Data() + scaleFloat32Array->ByteOffset(), sizeof(scale.values));

    MLMat4f result = composeMatrix(position, orientation, scale);
    if (inverse) {
      result = invertMatrix(result);
    }
    return result;
  } else {
    return makeTranslationMatrix(MLVec3f{0, largestFloorY * (inverse ? -1 : 1), 0});
  }
}

// MLRaycaster

MLHandle MLRaycaster::tracker;
bool MLRaycaster::hasTracker = false;
MLRaycaster::MLRaycaster(const MLVec3f &position, const MLVec3f &direction) {
  if (!hasTracker) {
    MLResult result = MLRaycastCreate(&tracker);
    if (result == MLResult_Ok) {
      hasTracker = true;
    } else {
      ML_LOG(Error, "%s: failed to create raycast handle %x", application_name, result);
    }
  }

  raycastQuery.position = position;
  raycastQuery.direction = direction;
  raycastQuery.up_vector = MLVec3f{0, 1, 0};
  raycastQuery.horizontal_fov_degrees = 30;
  raycastQuery.width = 1;
  raycastQuery.height = 1;
  raycastQuery.collide_with_unobserved = false;

  MLResult result = MLRaycastRequest(tracker, &raycastQuery, &requestHandle);
  if (result != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to request raycast: %x %x", application_name, result);
    Nan::ThrowError("failed to request raycast");
  }
}

MLRaycaster::~MLRaycaster() {}

Local<Function> MLRaycaster::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("MLRaycaster"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "waitGetPoses", WaitGetPoses);
  // Nan::SetMethod(proto, "destroy", Destroy);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

NAN_METHOD(MLRaycaster::New) {
  Local<Float32Array> originFloat32Array = Local<Float32Array>::Cast(info[0]);
  Local<Float32Array> directionFloat32Array = Local<Float32Array>::Cast(info[1]);

  MLVec3f position;
  memcpy(position.values, (char *)originFloat32Array->Buffer()->GetContents().Data() + originFloat32Array->ByteOffset(), sizeof(raycastQuery.position.values));

  MLVec3f direction;
  memcpy(direction.values, (char *)directionFloat32Array->Buffer()->GetContents().Data() + directionFloat32Array->ByteOffset(), sizeof(raycastQuery.direction.values));

  MLRaycaster *mlRaycaster = new MLRaycaster(position, direction);
  Local<Object> mlRaycasterObj = info.This();
  mlRaycaster->Wrap(mlRaycasterObj);
  info.GetReturnValue().Set(mlRaycasterObj);
}

NAN_METHOD(MLRaycaster::WaitGetPoses) {
  MLRaycaster *mlRaycaster = ObjectWrap::Unwrap<MLRaycaster>(info.This());

  MLRaycastResult raycastResult;
  MLResult result = MLRaycastGetResult(mlRaycaster->tracker, mlRaycaster->requestHandle, &raycastResult);
  if (result == MLResult_Ok) {
    if (raycastResult.state == MLRaycastResultState_HitObserved) {
      const MLVec3f position = raycastResult.hitpoint;
      const MLQuaternionf quaternion = getQuaternionFromUnitVectors(MLVec3f{0, 1, 0}, raycastResult.normal);
      const MLVec3f scale = {1, 1, 1};

      MLMat4f hitMatrix = composeMatrix(position, quaternion, scale);

      Local<Object> xrHitResult = Nan::New<Object>();
      Local<Float32Array> hitMatrixArray = Float32Array::New(ArrayBuffer::New(Isolate::GetCurrent(), (void *)hitMatrix.matrix_colmajor, sizeof(hitMatrix.matrix_colmajor)), 0, 16);
      xrHitResult->Set(JS_STR("hitMatrix"), hitMatrixArray);
      Local<Array> result = Nan::New<Array>(1);
      result->Set(0, xrHitResult);
      info.GetReturnValue().Set(result);
    } else {
      Local<Array> result = Nan::New<Array>(0);
      info.GetReturnValue().Set(result);
    }
  } else if (result == MLResult_Pending) {
    info.GetReturnValue().Set(Nan::Null());
  } else {
    ML_LOG(Error, "%s: Raycast request failed! %x", application_name, result);

    Local<Array> result = Nan::New<Array>(0);
    info.GetReturnValue().Set(result);
  }
}

// MLCallback

/* MLCallback::MLCallback(uv_loop_t *loop, std::function<void()> fn) : async(new uv_async_t()), fn(fn) {
  async->data = this;
  uv_async_init(loop, async.get(), RunInAsyncThread);
}

MLCallback::~MLCallback() {
  uv_close((uv_handle_t *)async.release(), [](uv_handle_t *handle) {
    delete handle;
  });
}

void MLCallback::RunInAsyncThread(uv_async_t *handle) {
  Nan::HandleScope scope;

  MLCallback *mlCallback = (MLCallback *)handle->data;
  mlCallback->fn();

  delete mlCallback;
} */

/* // MLPoll

MLPoll::MLPoll(Local<Object> windowObj, std::function<void()> cb) : windowObj(windowObj), cb(cb) {}

MLPoll::~MLPoll() {} */

// MeshBuffer

/* MeshBuffer::MeshBuffer() :
  positions(nullptr),
  numPositions(0),
  normals(nullptr),
  indices(nullptr),
  numIndices(0)
  {}
MeshBuffer::MeshBuffer(MLTransform transform, float *positions, uint32_t numPositions, float *normals, uint16_t *indices, uint16_t numIndices) :
  transform(transform),
  positions(positions),
  numPositions(numPositions),
  normals(normals),
  indices(indices),
  numIndices(numIndices)
  {} */
/* MeshBuffer::MeshBuffer(const MeshBuffer &meshBuffer) {
  positionBuffer = meshBuffer.positionBuffer;
  normalBuffer = meshBuffer.normalBuffer;
  indexBuffer = meshBuffer.indexBuffer;
  positions = meshBuffer.positions;
  numPositions = meshBuffer.numPositions;
  normals = meshBuffer.normals;
  indices = meshBuffer.indices;
  numIndices = meshBuffer.numIndices;
} */

/* float getMaxRange() {
  float range = 1.0f;
  for (auto iter = meshers.begin(); iter != meshers.end(); iter++) {
    MLMesher *mesher = *iter;
    range = std::max(range, mesher->range);
  }
}

MLMeshingLOD getMaxLod() {
  MLMeshingLOD lod = MLMeshingLOD_Minimum;
  for (auto iter = meshers.begin(); iter != meshers.end(); iter++) {
    MLMesher *mesher = *iter;
    lod = std::max(lod, mesher->lod);
  }
} */

// MLMesher

MLMesher::MLMesher(float range, MLMeshingLOD lod) : range(range), lod(lod), meshInfoRequestPending(false), meshRequestsPending(false), meshRequestPending(false) {
  MLMeshingSettings meshingSettings;
  if (MLMeshingInitSettings(&meshingSettings) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to initialize meshing settings", application_name);
  }
  meshingSettings.flags |= MLMeshingFlags_ComputeNormals;
  // meshingSettings.flags |= MLMeshingFlags_Planarize;
  {
    MLResult result = MLMeshingCreateClient(&tracker, &meshingSettings);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "%s: failed to create mesh handle %x", application_name, result);
      Nan::ThrowError("MLContext::Present failed to create mesh handle");
      return;
    }
  }
}

MLMesher::~MLMesher() {}

NAN_METHOD(MLMesher::New) {
  float range = TO_FLOAT(info[0]);
  int lodValue = TO_INT32(info[1]);

  MLMeshingLOD lod;
  switch (lodValue) {
    case 1: {
      lod = MLMeshingLOD_Minimum;
      break;
    }
    case 2: {
      lod = MLMeshingLOD_Medium;
      break;
    }
    case 3: {
      lod = MLMeshingLOD_Maximum;
      break;
    }
    default: {
      lod = MLMeshingLOD_Minimum;
      break;
    }
  }

  MLMesher *mlMesher = new MLMesher(range, lod);
  Local<Object> mlMesherObj = info.This();
  mlMesher->Wrap(mlMesherObj);

  // Nan::SetAccessor(mlMesherObj, JS_STR("onmesh"), OnMeshGetter, OnMeshSetter);

  info.GetReturnValue().Set(mlMesherObj);

  // meshers.push_back(mlMesher);
}

Local<Function> MLMesher::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("MLMesher"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "waitGetPoses", WaitGetPoses);
  Nan::SetMethod(proto, "destroy", Destroy);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

/* NAN_GETTER(MLMesher::OnMeshGetter) {
  // Nan::HandleScope scope;
  MLMesher *mlMesher = ObjectWrap::Unwrap<MLMesher>(info.This());

  Local<Function> cb = Nan::New(mlMesher->cb);
  info.GetReturnValue().Set(cb);
}

NAN_SETTER(MLMesher::OnMeshSetter) {
  // Nan::HandleScope scope;
  MLMesher *mlMesher = ObjectWrap::Unwrap<MLMesher>(info.This());

  if (value->IsFunction()) {
    Local<Function> localCb = Local<Function>::Cast(value);
    mlMesher->cb.Reset(localCb);
  } else {
    Nan::ThrowError("MLMesher::OnMeshSetter: invalid arguments");
  }
} */

NAN_METHOD(MLMesher::WaitGetPoses) {
  MLMesher *mlMesher = ObjectWrap::Unwrap<MLMesher>(info.This());
  MLContext *mlContext = application_context.mlContext;

  // MLMat4f transformMatrix = getWindowTransformMatrix(Nan::New(this->windowObj));

  // request

  if (!mlMesher->meshInfoRequestPending && !mlMesher->meshRequestsPending) {
    MLMeshingExtents meshExtents;
    {
      // std::unique_lock<std::mutex> lock(mlContext->positionMutex);

      meshExtents.center = mlContext->position;
      meshExtents.rotation =  mlContext->rotation;
      // meshExtents.rotation = {0, 0, 0, 1};
    }
    meshExtents.extents.x = mlMesher->range;
    meshExtents.extents.y = mlMesher->range;
    meshExtents.extents.z = mlMesher->range;

    MLResult result = MLMeshingRequestMeshInfo(mlMesher->tracker, &meshExtents, &mlMesher->meshInfoRequestHandle);
    if (result == MLResult_Ok) {
      mlMesher->meshInfoRequestPending = true;
    } else {
      ML_LOG(Error, "%s: Mesh info request failed! %x", application_name, result);
    }
  }

  /* {
    // std::unique_lock<std::mutex> lock(cameraRequestsMutex);

    if (cameraRequests.size() > 0) {
      cameraRequestConditionVariable.notify_one();
    }
  } */

  // responses

  if (mlMesher->meshInfoRequestPending) {
    MLResult result = MLMeshingGetMeshInfoResult(mlMesher->tracker, mlMesher->meshInfoRequestHandle, &mlMesher->meshInfo);
    if (result == MLResult_Ok) {
      for (uint32_t i = 0; i < mlMesher->meshInfo.data_count; i++) {
        MLMeshingBlockInfo &meshInfo = mlMesher->meshInfo.data[i];
        const std::string &id = id2String(meshInfo.id);
        mlMesher->meshInfoMap[id] = &meshInfo;
      }
      /* uint32_t dataCount = meshInfo.data_count;

      meshRequestNewMap.clear();
      meshRequestRemovedMap.clear();
      meshRequestUnchangedMap.clear();
      for (uint32_t i = 0; i < dataCount; i++) {
        const MLMeshingBlockInfo &meshInfo = meshInfo.data[i];
        const MLMeshingMeshState &state = meshInfo.state;
        MLMeshingBlockRequest &meshBlockRequest = meshBlockRequests[i];
        meshBlockRequest.id = meshInfo.id;
        meshBlockRequest.level = mlMesher->lod;

        const std::string &id = id2String(meshInfo.id);
        meshRequestNewMap[id] = (state == MLMeshingMeshState_New);
        meshRequestRemovedMap[id] = (state == MLMeshingMeshState_Deleted);
        meshRequestUnchangedMap[id] = (state == MLMeshingMeshState_Unchanged);
      }
      numMeshBlockRequests = dataCount; */

      mlMesher->meshInfoRequestPending = false;
      mlMesher->meshRequestsPending = true;
      mlMesher->meshRequestPending = false;
      mlMesher->meshBlockRequestIndex = 0;
    } else if (result == MLResult_Pending) {
      // nothing
    } else {
      ML_LOG(Error, "%s: Mesh info get failed! %x", application_name, result);

      mlMesher->meshInfoRequestPending = false;
    }
  }

  if (mlMesher->meshRequestsPending && !mlMesher->meshRequestPending) {
    if (mlMesher->meshBlockRequestIndex < mlMesher->meshInfo.data_count) {
      uint32_t requestsRemaining = mlMesher->meshInfo.data_count - mlMesher->meshBlockRequestIndex;
      uint32_t requestsThisTime = std::min<uint32_t>(requestsRemaining, 16);

      std::vector<MLMeshingBlockRequest> meshBlockRequests(requestsThisTime);
      for (uint32_t i = 0; i < requestsThisTime; i++) {
        const MLMeshingBlockInfo &meshInfo = mlMesher->meshInfo.data[mlMesher->meshBlockRequestIndex + i];
        const MLMeshingMeshState &state = meshInfo.state;
        MLMeshingBlockRequest &meshBlockRequest = meshBlockRequests[i];
        meshBlockRequest.id = meshInfo.id;
        meshBlockRequest.level = mlMesher->lod;

        /* const std::string &id = id2String(meshInfo.id);
        meshRequestNewMap[id] = (state == MLMeshingMeshState_New);
        meshRequestRemovedMap[id] = (state == MLMeshingMeshState_Deleted);
        meshRequestUnchangedMap[id] = (state == MLMeshingMeshState_Unchanged); */
      }

      MLMeshingMeshRequest meshRequest;
      meshRequest.data = meshBlockRequests.data();
      meshRequest.request_count = requestsThisTime;

      mlMesher->meshBlockRequestIndex += requestsThisTime;

      MLResult result = MLMeshingRequestMesh(mlMesher->tracker, &meshRequest, &mlMesher->meshRequestHandle);
      if (result == MLResult_Ok) {
        mlMesher->meshRequestsPending = true;
        mlMesher->meshRequestPending = true;
      } else {
        ML_LOG(Error, "%s: Mesh request failed! %x", application_name, result);

        mlMesher->meshInfoMap.clear();
        MLMeshingFreeResource(mlMesher->tracker, &mlMesher->meshInfoRequestHandle);

        mlMesher->meshRequestsPending = false;
        mlMesher->meshRequestPending = false;
      }
    } else {
      mlMesher->meshInfoMap.clear();
      MLMeshingFreeResource(mlMesher->tracker, &mlMesher->meshInfoRequestHandle);

      mlMesher->meshRequestsPending = false;
      mlMesher->meshRequestPending = false;
    }
  }
  if (mlMesher->meshRequestsPending && mlMesher->meshRequestPending) {
    MLMeshingMesh mesh;
    MLResult result = MLMeshingGetMeshResult(mlMesher->tracker, mlMesher->meshRequestHandle, &mesh);
    if (result == MLResult_Ok) {
      // std::map<std::string, MLMeshingBlockInfo*> blockMeshInfoMap();

      Local<Array> result = Nan::New<Array>();
      int index = 0;
      for (uint32_t i = 0; i < mesh.data_count; i++) {
        MLMeshingBlockMesh &blockMesh = mesh.data[i];
        const std::string &id = id2String(blockMesh.id);
        auto meshInfoIter = mlMesher->meshInfoMap.find(id);
        if (meshInfoIter == mlMesher->meshInfoMap.end()) {
          ML_LOG(Error, "%s: Mesh result for unknown id %s", application_name, id.c_str());
        }
        MLMeshingBlockInfo &meshInfo = *meshInfoIter->second;

        if (meshInfo.state == MLMeshingMeshState_New || (meshInfo.state == MLMeshingMeshState_Updated && blockMesh.index_count > 0) || MLMeshingMeshState_Deleted) {
          Local<Object> obj = Nan::New<Object>();
          obj->Set(JS_STR("id"), JS_STR(id));

          const char *typeString;
          if (meshInfo.state == MLMeshingMeshState_New) {
            typeString = "meshadd";
          } else if (meshInfo.state == MLMeshingMeshState_Updated) {
            typeString = "meshupdate";
          } else if (meshInfo.state == MLMeshingMeshState_Deleted) {
            typeString = "meshremove";
          /* } else if (meshInfo.state == MLMeshingMeshState_Unchanged) {
            typeString = "meshunchanged"; */
          } else {
            typeString = "";
          }
          obj->Set(JS_STR("type"), JS_STR(typeString));

          if (meshInfo.state == MLMeshingMeshState_New || meshInfo.state == MLMeshingMeshState_Updated) {
            /* Local<Float32Array> transformMatrixArray = Float32Array::New(ArrayBuffer::New(Isolate::GetCurrent(), (void *)transformMatrix.matrix_colmajor, 16 * sizeof(float)), 0, 16);
            obj->Set(JS_STR("transformMatrix"), transformMatrixArray); */

            Local<SharedArrayBuffer> positionArrayBuffer = SharedArrayBuffer::New(Isolate::GetCurrent(), blockMesh.vertex_count * 3 * sizeof(float));
            Local<Float32Array> positionArray = Float32Array::New(positionArrayBuffer, 0, blockMesh.vertex_count * 3);
            memcpy(positionArrayBuffer->GetContents().Data(), blockMesh.vertex, blockMesh.vertex_count * 3 * sizeof(float));
            obj->Set(JS_STR("positionArray"), positionArray);
            // obj->Set(JS_STR("positionCount"), JS_INT(blockMesh.vertex_count * 3));

            Local<SharedArrayBuffer> normalArrayBuffer = SharedArrayBuffer::New(Isolate::GetCurrent(), blockMesh.vertex_count * 3 * sizeof(float));
            Local<Float32Array> normalArray = Float32Array::New(normalArrayBuffer, 0, blockMesh.vertex_count * 3);
            memcpy(normalArrayBuffer->GetContents().Data(), blockMesh.normal, blockMesh.vertex_count * 3 * sizeof(float));
            obj->Set(JS_STR("normalArray"), normalArray);
            // obj->Set(JS_STR("normalCount"), JS_INT(blockMesh.vertex_count * 3));

            Local<SharedArrayBuffer> indexArrayBuffer = SharedArrayBuffer::New(Isolate::GetCurrent(), blockMesh.index_count * sizeof(uint16_t));
            Local<Uint16Array> indexArray = Uint16Array::New(indexArrayBuffer, 0, blockMesh.index_count);
            memcpy(indexArrayBuffer->GetContents().Data(), blockMesh.index, blockMesh.index_count * sizeof(uint16_t));
            obj->Set(JS_STR("indexArray"), indexArray);
            // obj->Set(JS_STR("count"), JS_INT(blockMesh.index_count));

            MLVec3f position = mlContext->OffsetFloor(MLVec3f{0, 0, 0}); // {0, 0, 0};
            MLQuaternionf rotation = {0, 0, 0, 1};
            MLVec3f scale = {1, 1, 1};
            MLMat4f transform = composeMatrix(position, rotation, scale);

            Local<ArrayBuffer> transformMatrixArrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), 16 * sizeof(float));
            Local<Float32Array> transformMatrixArray = Float32Array::New(transformMatrixArrayBuffer, 0, 16);
            float *transformMatrixData = (float *)((char *)transformMatrixArrayBuffer->GetContents().Data());
            memcpy(transformMatrixData, transform.matrix_colmajor, sizeof(transform.matrix_colmajor));
            obj->Set(JS_STR("transformMatrix"), transformMatrixArray);
          }

          result->Set(index, obj);
          index++;
        }
      }

      /* // remove outranged mesh buffers
      float range = getMaxRange();
      std::vector<std::string> removedIds;
      for (auto iter = meshBuffers.begin(); iter != meshBuffers.end(); iter++) {
        const std::string &id = iter->first;
        MeshBuffer &meshBuffer = iter->second;
        float distance = distanceTo(mlContext->position, meshBuffer.transform.position);
        if (distance > range) {
          meshRequestRemovedMap[id] = true;
          removedIds.push_back(id);
        }
      }
      for (auto iter = removedIds.begin(); iter != removedIds.end(); iter++) {
        meshBuffers.erase(*iter);
      } */

      info.GetReturnValue().Set(result);

      mlMesher->meshRequestsPending = true;
      mlMesher->meshRequestPending = false;

      MLMeshingFreeResource(mlMesher->tracker, &mlMesher->meshRequestHandle);
    } else if (result == MLResult_Pending) {
      info.GetReturnValue().Set(Nan::Null());
    } else {
      ML_LOG(Error, "%s: Mesh get failed! %x", application_name, result);

      mlMesher->meshRequestsPending = true;
      mlMesher->meshRequestPending = false;

      info.GetReturnValue().Set(Nan::Null());
    }
  }
}

NAN_METHOD(MLMesher::Destroy) {
  MLMesher *mlMesher = ObjectWrap::Unwrap<MLMesher>(info.This());

  if (MLMeshingDestroyClient(&mlMesher->tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to destroy mesh handle", application_name);
  }
}

// MLPlaneTracker

MLPlaneTracker::MLPlaneTracker(float range) : range(range), planesRequestPending(false) {
  if (MLPlanesCreate(&tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to create planes handle", application_name);
  }
}

MLPlaneTracker::~MLPlaneTracker() {}

NAN_METHOD(MLPlaneTracker::New) {
  float range = TO_FLOAT(info[0]);

  MLPlaneTracker *mlPlaneTracker = new MLPlaneTracker(range);
  Local<Object> mlPlaneTrackerObj = info.This();
  mlPlaneTracker->Wrap(mlPlaneTrackerObj);

  // Nan::SetAccessor(mlPlaneTrackerObj, JS_STR("onplanes"), OnPlanesGetter, OnPlanesSetter);

  info.GetReturnValue().Set(mlPlaneTrackerObj);
}

Local<Function> MLPlaneTracker::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("MLPlaneTracker"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "waitGetPoses", WaitGetPoses);
  Nan::SetMethod(proto, "destroy", Destroy);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

/* NAN_GETTER(MLPlaneTracker::OnPlanesGetter) {
  // Nan::HandleScope scope;
  MLPlaneTracker *mlPlaneTracker = ObjectWrap::Unwrap<MLPlaneTracker>(info.This());

  Local<Function> cb = Nan::New(mlPlaneTracker->cb);
  info.GetReturnValue().Set(cb);
}

NAN_SETTER(MLPlaneTracker::OnPlanesSetter) {
  // Nan::HandleScope scope;
  MLPlaneTracker *mlPlaneTracker = ObjectWrap::Unwrap<MLPlaneTracker>(info.This());

  if (value->IsFunction()) {
    Local<Function> localCb = Local<Function>::Cast(value);
    mlPlaneTracker->cb.Reset(localCb);
  } else {
    Nan::ThrowError("MLPlaneTracker::OnPlaneSetter: invalid arguments");
  }
} */

NAN_METHOD(MLPlaneTracker::WaitGetPoses) {
  MLPlaneTracker *mlPlaneTracker = ObjectWrap::Unwrap<MLPlaneTracker>(info.This());
  MLContext *mlContext = application_context.mlContext;

  // MLMat4f transformMatrix = getWindowTransformMatrix(Nan::New(this->windowObj));

  if (!mlPlaneTracker->planesRequestPending) {
    MLPlanesQuery planesRequest;
    {
      // std::unique_lock<std::mutex> lock(mlContext->positionMutex);

      planesRequest.bounds_center = mlContext->position;
      planesRequest.bounds_rotation = mlContext->rotation;
      // planesRequest.bounds_rotation = {0, 0, 0, 1};
    }
    planesRequest.bounds_extents.x = mlPlaneTracker->range;
    planesRequest.bounds_extents.y = mlPlaneTracker->range;
    planesRequest.bounds_extents.z = mlPlaneTracker->range;

    planesRequest.flags = MLPlanesQueryFlag_Arbitrary | MLPlanesQueryFlag_AllOrientations | MLPlanesQueryFlag_Semantic_All | MLPlanesQueryFlag_OrientToGravity;
    // planesRequest.min_hole_length = 0.5;
    planesRequest.min_plane_area = 0.25;
    planesRequest.max_results = MAX_NUM_PLANES;

    MLResult result = MLPlanesQueryBegin(mlPlaneTracker->tracker, &planesRequest, &mlPlaneTracker->planesRequestHandle);
    if (result == MLResult_Ok) {
      mlPlaneTracker->planesRequestPending = true;
    } else {
      ML_LOG(Error, "%s: Planes request failed! %x", application_name, result);
    }
  }

  if (mlPlaneTracker->planesRequestPending) {
    MLPlane planeResults[MAX_NUM_PLANES];
    uint32_t numPlanesResults;
    MLResult result = MLPlanesQueryGetResultsWithBoundaries(mlPlaneTracker->tracker, mlPlaneTracker->planesRequestHandle, planeResults, &numPlanesResults);
    if (result == MLResult_Ok) {
      std::vector<std::string> currentPlaneIds(numPlanesResults);
      for (uint32_t i = 0; i < numPlanesResults; i++) {
        MLPlane &plane = planeResults[i];
        uint64_t planeId = (uint64_t)plane.id;
        currentPlaneIds[i] = id2String((uint64_t)planeId);
      }

      std::set<std::string> addedPlaneIds;
      for (size_t i = 0; i < currentPlaneIds.size(); i++) {
        const std::string &id = currentPlaneIds[i];
        if (std::find(mlPlaneTracker->planeIds.begin(), mlPlaneTracker->planeIds.end(), id) == mlPlaneTracker->planeIds.end()) {
          addedPlaneIds.insert(id);
        }
      }

      std::set<std::string> removedPlaneIds;
      for (size_t i = 0; i < mlPlaneTracker->planeIds.size(); i++) {
        const std::string &id = mlPlaneTracker->planeIds[i];
        if (std::find(currentPlaneIds.begin(), currentPlaneIds.end(), id) == currentPlaneIds.end()) {
          removedPlaneIds.insert(id);
        }
      }

      Local<Array> result = Nan::New<Array>(numPlanesResults);
      for (uint32_t i = 0; i < numPlanesResults; i++) {
        MLPlane &plane = planeResults[i];

        const std::string &id = currentPlaneIds[i];

        Local<Object> obj = Nan::New<Object>();

        obj->Set(JS_STR("id"), JS_STR(id));

        bool added = false;
        bool removed = false;
        bool updated = false;

        const char *typeString;
        if ((added = (addedPlaneIds.find(id) != addedPlaneIds.end()))) {
          typeString = "planeadd";
        } else if ((removed = (removedPlaneIds.find(id) != removedPlaneIds.end()))) {
          typeString = "planeremove";
        } else {
          updated = true;
          typeString = "planeupdate";
        }
        obj->Set(JS_STR("type"), JS_STR(typeString));

        if (added || updated) {
          // uint32_t flags = plane.flags;
          const float &width = plane.width;
          const float &height = plane.height;
          const MLVec3f &position = mlContext->OffsetFloor(plane.position);
          const MLQuaternionf &rotation = plane.rotation;
          const MLVec3f &normal = applyVectorQuaternion(MLVec3f{0, 0, 1}, rotation);

          Local<ArrayBuffer> arrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), sizeof(position.values)+sizeof(normal.values)+sizeof(width)+sizeof(height));
          char *arrayBufferData = (char *)arrayBuffer->GetContents().Data();
          size_t index = 0;

          memcpy(arrayBufferData + index, position.values, sizeof(position.values));
          obj->Set(JS_STR("position"), Float32Array::New(arrayBuffer, index, sizeof(position.values)/sizeof(position.values[0])));
          index += sizeof(position.values);

          memcpy(arrayBufferData + index, normal.values, sizeof(normal.values));
          obj->Set(JS_STR("normal"), Float32Array::New(arrayBuffer, index, sizeof(normal.values)/sizeof(normal.values[0])));
          index += sizeof(normal.values);

          ((float *)(arrayBufferData + index))[0] = width;
          ((float *)(arrayBufferData + index))[1] = height;
          obj->Set(JS_STR("size"), Float32Array::New(arrayBuffer, index, 2));
          index += 2*sizeof(float);
        }

        result->Set(i, obj);
      }

      info.GetReturnValue().Set(result);

      mlPlaneTracker->planeIds = std::move(currentPlaneIds);
      mlPlaneTracker->planesRequestPending = false;
    } else if (result == MLResult_Pending) {
      info.GetReturnValue().Set(Nan::Null());
    } else {
      ML_LOG(Error, "%s: Planes request failed! %x", application_name, result);

      mlPlaneTracker->planesRequestPending = false;

      info.GetReturnValue().Set(Nan::Null());
    }
  }
}

NAN_METHOD(MLPlaneTracker::Destroy) {
  MLPlaneTracker *mlPlaneTracker = ObjectWrap::Unwrap<MLPlaneTracker>(info.This());

  if (MLPlanesDestroy(mlPlaneTracker->tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to destroy planes handle", application_name);
  }
}

// MLHandTracker

/* void setFingerValue(const MLWristState &wristState, MLSnapshot *snapshot, float data[4][1 + 3]);
void setFingerValue(const MLThumbState &thumbState, MLSnapshot *snapshot, float data[4][1 + 3]);
void setFingerValue(const MLFingerState &fingerState, MLSnapshot *snapshot, float data[4][1 + 3]); */
void setFingerValue(const MLKeyPointState &keyPointState, MLSnapshot *snapshot, float *data);
void setFingerValue(float *data);

/* void setFingerValue(const MLWristState &wristState, MLSnapshot *snapshot, float data[4][1 + 3]) {
  setFingerValue(wristState.radial, snapshot, data[0]);
  setFingerValue(wristState.ulnar, snapshot, data[1]);
  setFingerValue(wristState.center, snapshot, data[2]);
  setFingerValue(data[3]);
}
void setFingerValue(const MLThumbState &thumbState, MLSnapshot *snapshot, float data[4][1 + 3]) {
  setFingerValue(thumbState.cmc, snapshot, data[0]);
  setFingerValue(thumbState.mcp, snapshot, data[1]);
  setFingerValue(thumbState.ip, snapshot, data[2]);
  setFingerValue(thumbState.tip, snapshot, data[3]);
}
void setFingerValue(const MLFingerState &fingerState, MLSnapshot *snapshot, float data[4][1 + 3]) {
  setFingerValue(fingerState.mcp, snapshot, data[0]);
  setFingerValue(fingerState.pip, snapshot, data[1]);
  setFingerValue(fingerState.dip, snapshot, data[2]);
  setFingerValue(fingerState.tip, snapshot, data[3]);
} */
void setFingerValue(const MLKeyPointState &keyPointState, MLSnapshot *snapshot, float *data) {
  // uint32_t *uint32Data = (uint32_t *)data;
  // float *floatData = (float *)data;

  if (keyPointState.is_valid) {
    MLTransform transform;
    MLResult result = MLSnapshotGetTransform(snapshot, &keyPointState.frame_id, &transform);
    if (result == MLResult_Ok) {
      // ML_LOG(Info, "%s: ML keypoint ok %f %f %f", application_name, transform.position.x, transform.position.y, transform.position.z);

      // uint32Data[0] = true;
      data[0] = transform.position.x;
      data[1] = transform.position.y;
      data[2] = transform.position.z;
    } else {
      // ML_LOG(Error, "%s: ML failed to get finger transform: %s", application_name, MLSnapshotGetResultString(result));

      setFingerValue(data);
    }
  } else {
    setFingerValue(data);
  }
}
void setFingerValue(float *data) {
  // uint32_t *uint32Data = (uint32_t *)data;
  // float *floatData = (float *)data;

  // uint32Data[0] = false;
  // const MLVec3f &position = MLVec3f{0, 0, 0};
  auto nan = std::numeric_limits<float>::quiet_NaN();
  data[0] = nan;
  data[1] = nan;
  data[2] = nan;
}

MLHandTracker::MLHandTracker() {
  if (MLHandTrackingCreate(&tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to create hand tracker.", application_name);
  }

  MLHandTrackingConfiguration handTrackingConfig;
  handTrackingConfig.handtracking_pipeline_enabled = true;
  handTrackingConfig.keypoints_filter_level = MLKeypointFilterLevel_1;
  handTrackingConfig.pose_filter_level = MLPoseFilterLevel_1;
  for (int i = 0; i < MLHandTrackingKeyPose_Count; i++) {
    handTrackingConfig.keypose_config[i] = true;
  }
  /* handTrackingConfig.keypose_config[MLHandTrackingKeyPose_Ok] = false;
  handTrackingConfig.keypose_config[MLHandTrackingKeyPose_C] = false;
  handTrackingConfig.keypose_config[MLHandTrackingKeyPose_L] = false; */
  /* for (int i = 0; i < MLHandTrackingKeyPose_Count; i++) {
    handTrackingConfig.keypose_config[i] = false;
  }
  handTrackingConfig.keypose_config[MLHandTrackingKeyPose_Pinch] = true; */
  if (MLHandTrackingSetConfiguration(tracker, &handTrackingConfig) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to set hand tracker config.", application_name);
  }
}

MLHandTracker::~MLHandTracker() {}

NAN_METHOD(MLHandTracker::New) {
  MLHandTracker *mlHandTracker = new MLHandTracker();
  Local<Object> mlHandTrackerObj = info.This();
  mlHandTracker->Wrap(mlHandTrackerObj);

  // Nan::SetAccessor(mlHandTrackerObj, JS_STR("onhands"), OnHandsGetter, OnHandsSetter);
  // Nan::SetAccessor(mlHandTrackerObj, JS_STR("ongesture"), OnGestureGetter, OnGestureSetter);

  info.GetReturnValue().Set(mlHandTrackerObj);
}

Local<Function> MLHandTracker::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("MLHandTracker"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "waitGetPoses", WaitGetPoses);
  Nan::SetMethod(proto, "destroy", Destroy);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

/* NAN_GETTER(MLHandTracker::OnHandsGetter) {
  // Nan::HandleScope scope;
  MLHandTracker *mlHandTracker = ObjectWrap::Unwrap<MLHandTracker>(info.This());

  Local<Function> cb = Nan::New(mlHandTracker->cb);
  info.GetReturnValue().Set(cb);
}

NAN_SETTER(MLHandTracker::OnHandsSetter) {
  // Nan::HandleScope scope;
  MLHandTracker *mlHandTracker = ObjectWrap::Unwrap<MLHandTracker>(info.This());

  if (value->IsFunction()) {
    Local<Function> localCb = Local<Function>::Cast(value);
    mlHandTracker->cb.Reset(localCb);
  } else {
    Nan::ThrowError("MLHandTracker::OnHandsSetter: invalid arguments");
  }
}

NAN_GETTER(MLHandTracker::OnGestureGetter) {
  // Nan::HandleScope scope;
  MLHandTracker *mlHandTracker = ObjectWrap::Unwrap<MLHandTracker>(info.This());

  Local<Function> ongesture = Nan::New(mlHandTracker->ongesture);
  info.GetReturnValue().Set(ongesture);
}

NAN_SETTER(MLHandTracker::OnGestureSetter) {
  // Nan::HandleScope scope;
  MLHandTracker *mlHandTracker = ObjectWrap::Unwrap<MLHandTracker>(info.This());

  if (value->IsFunction()) {
    Local<Function> localOngesture = Local<Function>::Cast(value);
    mlHandTracker->ongesture.Reset(localOngesture);
  } else {
    Nan::ThrowError("MLHandTracker::OnGestureSetter: invalid arguments");
  }
} */

NAN_METHOD(MLHandTracker::WaitGetPoses) {
  // MLMat4f transformMatrix = getWindowTransformMatrix(Nan::New(this->windowObj));

  MLHandTracker *mlHandTracker = ObjectWrap::Unwrap<MLHandTracker>(info.This());
  Local<Array> handsArray = Local<Array>::Cast(info[0]);
  MLContext *mlContext = application_context.mlContext;

  MLHandTrackingData handData;
  MLResult result = MLHandTrackingGetData(mlHandTracker->tracker, &handData);
  if (result == MLResult_Ok) {
    MLHandTrackingStaticData handStaticData;
    MLResult result = MLHandTrackingGetStaticData(mlHandTracker->tracker, &handStaticData);
    if (result == MLResult_Ok) {
      MLSnapshot *snapshot = nullptr;
      if (MLPerceptionGetSnapshot(&snapshot) != MLResult_Ok) {
        ML_LOG(Error, "%s: ML failed to get hand snapshot!", application_name);
      }

      for (int i = 0; i < 2; i++) {
        Local<Object> handObj = Local<Object>::Cast(handsArray->Get(i));

        Local<Float32Array> connectedArray = Local<Float32Array>::Cast(handObj->Get(JS_STR("connected")));
        auto &handDataState = i == 0 ? handData.left_hand_state : handData.right_hand_state;
        const bool handVisible = handDataState.hand_confidence >= 0.5;
        connectedArray->Set(0, JS_NUM(handVisible ? 1 : 0));

        if (handVisible) {
          const MLVec3f &position = mlContext->OffsetFloor(MLVec3f{0, 0, 0});
          Local<Float32Array> positionArray = Local<Float32Array>::Cast(handObj->Get(JS_STR("position")));
          float *positionArrayData = (float *)((char *)positionArray->Buffer()->GetContents().Data() + positionArray->ByteOffset());
          positionArrayData[0] = position.x;
          positionArrayData[1] = position.y;
          positionArrayData[2] = position.z;

          Local<Float32Array> orientationArray = Local<Float32Array>::Cast(handObj->Get(JS_STR("orientation")));
          float *orientationArrayData = (float *)((char *)orientationArray->Buffer()->GetContents().Data() + orientationArray->ByteOffset());
          orientationArrayData[0] = 0;
          orientationArrayData[1] = 0;
          orientationArrayData[2] = 0;
          orientationArrayData[3] = 1;

          auto &handStaticDataSide = i == 0 ? handStaticData.left : handStaticData.right;

          // wrist
          {
            auto &handStaticDataSideFinger = handStaticDataSide.wrist;
            Local<Array> wristArray = Local<Array>::Cast(handObj->Get(JS_STR("wrist")));
            {
              Local<Float32Array> wristBoneFloat32Array = Local<Float32Array>::Cast(wristArray->Get(0));
              float *wristBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(wristBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + wristBoneFloat32Array->ByteOffset());
              setFingerValue(handStaticDataSideFinger.radial, snapshot, wristBoneArrayData);
            }
            {
              Local<Float32Array> wristBoneFloat32Array = Local<Float32Array>::Cast(wristArray->Get(1));
              float *wristBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(wristBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + wristBoneFloat32Array->ByteOffset());
              setFingerValue(handStaticDataSideFinger.ulnar, snapshot, wristBoneArrayData);
            }
            {
              Local<Float32Array> wristBoneFloat32Array = Local<Float32Array>::Cast(wristArray->Get(2));
              float *wristBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(wristBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + wristBoneFloat32Array->ByteOffset());
              setFingerValue(handStaticDataSideFinger.center, snapshot, wristBoneArrayData);
            }
            {
              Local<Float32Array> wristBoneFloat32Array = Local<Float32Array>::Cast(wristArray->Get(3));
              float *wristBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(wristBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + wristBoneFloat32Array->ByteOffset());
              setFingerValue(wristBoneArrayData);
            }
          }

          Local<Array> fingersArray = Local<Array>::Cast(handObj->Get(JS_STR("fingers")));
          {
            // thumb
            {
              auto &handStaticDataSideFinger = handStaticDataSide.thumb;
              Local<Array> fingerArray = Local<Array>::Cast(fingersArray->Get(0));
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(0));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.cmc, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(1));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.mcp, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(2));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.ip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(3));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.tip, snapshot, fingerBoneArrayData);
              }
            }

            // index
            {
              auto &handStaticDataSideFinger = handStaticDataSide.index;
              Local<Array> fingerArray = Local<Array>::Cast(fingersArray->Get(1));
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(0));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.mcp, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(1));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.pip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(2));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.dip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(3));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.tip, snapshot, fingerBoneArrayData);
              }
            }

            // middle
            {
              auto &handStaticDataSideFinger = handStaticDataSide.middle;
              Local<Array> fingerArray = Local<Array>::Cast(fingersArray->Get(2));
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(0));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.mcp, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(1));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.pip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(2));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.dip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(3));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.tip, snapshot, fingerBoneArrayData);
              }
            }

            // ring
            {
              auto &handStaticDataSideFinger = handStaticDataSide.ring;
              Local<Array> fingerArray = Local<Array>::Cast(fingersArray->Get(3));
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(0));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.mcp, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(1));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.pip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(2));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.dip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(3));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.tip, snapshot, fingerBoneArrayData);
              }
            }

            // pinky
            {
              auto &handStaticDataSideFinger = handStaticDataSide.pinky;
              Local<Array> fingerArray = Local<Array>::Cast(fingersArray->Get(4));
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(0));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.mcp, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(1));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.pip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(2));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.dip, snapshot, fingerBoneArrayData);
              }
              {
                Local<Float32Array> fingerBoneFloat32Array = Local<Float32Array>::Cast(fingerArray->Get(3));
                float *fingerBoneArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(fingerBoneFloat32Array->Get(JS_STR("buffer")))->GetContents().Data() + fingerBoneFloat32Array->ByteOffset());
                setFingerValue(handStaticDataSideFinger.tip, snapshot, fingerBoneArrayData);
              }
            }
          }

          // MLHandTrackingKeyPose &keypose = handDataState.keypose;
        }
      }

      if (MLPerceptionReleaseSnapshot(snapshot) != MLResult_Ok) {
        ML_LOG(Error, "%s: ML failed to release hand snapshot!", application_name);
      }
    } else {
      ML_LOG(Error, "%s: Hand static data get failed! %x", application_name, result);
    }
  } else {
    ML_LOG(Error, "%s: Hand data get failed! %x", application_name, result);
  }
}

NAN_METHOD(MLHandTracker::Destroy) {
  MLHandTracker *mlHandTracker = ObjectWrap::Unwrap<MLHandTracker>(info.This());

  if (MLHandTrackingDestroy(mlHandTracker->tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to destroy hand tracker.", application_name);
  }
}

// MLEyeTracker

MLEyeTracker::MLEyeTracker() {
  if (MLEyeTrackingCreate(&tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to create eye handle", application_name);
  }
}

MLEyeTracker::~MLEyeTracker() {}

NAN_METHOD(MLEyeTracker::New) {
  MLEyeTracker *mlEyeTracker = new MLEyeTracker();
  Local<Object> mlEyeTrackerObj = info.This();
  mlEyeTracker->Wrap(mlEyeTrackerObj);

  info.GetReturnValue().Set(mlEyeTrackerObj);
}

Local<Function> MLEyeTracker::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("MLEyeTracker"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "waitGetPoses", WaitGetPoses);
  Nan::SetMethod(proto, "destroy", Destroy);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

NAN_METHOD(MLEyeTracker::WaitGetPoses) {
  // Nan::HandleScope scope;

  MLEyeTracker *mlEyeTracker = ObjectWrap::Unwrap<MLEyeTracker>(info.This());
  Local<Object> eyeObj = Local<Object>::Cast(info[0]);
  MLContext *mlContext = application_context.mlContext;

  MLEyeTrackingStaticData eyeStaticData;
  if (MLEyeTrackingGetStaticData(mlEyeTracker->tracker, &eyeStaticData) != MLResult_Ok) {
    ML_LOG(Error, "%s: Eye get static data failed!", application_name);
  }

  MLEyeTrackingState eyeState;
  if (MLEyeTrackingGetState(mlEyeTracker->tracker, &eyeState) != MLResult_Ok) {
    ML_LOG(Error, "%s: Eye get state failed!", application_name);
  }

  MLSnapshot *snapshot;
  if (MLPerceptionGetSnapshot(&snapshot) != MLResult_Ok) {
    ML_LOG(Error, "%s: ML failed to get eye snapshot!", application_name);
  }

  MLTransform transform;
  if (MLSnapshotGetTransform(snapshot, &eyeStaticData.fixation, &transform) != MLResult_Ok) {
    ML_LOG(Error, "%s: ML failed to get eye fixation transform!", application_name);
  }

  const MLVec3f &position = mlContext->OffsetFloor(transform.position);
  Local<Float32Array> positionArray = Local<Float32Array>::Cast(eyeObj->Get(JS_STR("position")));
  float *positionArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(positionArray->Get(JS_STR("buffer")))->GetContents().Data() + positionArray->ByteOffset());
  memcpy(positionArrayData, position.values, sizeof(position.values));

  Local<Float32Array> orientationArray = Local<Float32Array>::Cast(eyeObj->Get(JS_STR("orientation")));
  float *orientationArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(orientationArray->Get(JS_STR("buffer")))->GetContents().Data() + orientationArray->ByteOffset());
  memcpy(orientationArrayData, transform.rotation.values, sizeof(transform.rotation.values));

  Local<Float32Array> axesArray = Local<Float32Array>::Cast(eyeObj->Get(JS_STR("axes")));
  float *axesArrayData = (float *)((char *)Local<SharedArrayBuffer>::Cast(axesArray->Get(JS_STR("buffer")))->GetContents().Data() + axesArray->ByteOffset());
  axesArrayData[0] = eyeState.left_blink ? -1 : 1;
  axesArrayData[1] = eyeState.right_blink ? -1 : 1;

  if (MLPerceptionReleaseSnapshot(snapshot) != MLResult_Ok) {
    ML_LOG(Error, "%s: ML failed to release eye snapshot!", application_name);
  }

  ML_LOG(Error, "MLEyeTracker::WaitGetPoses 9");
}

/* void MLEyeTracker::Update(MLSnapshot *snapshot) {
  if (MLSnapshotGetTransform(snapshot, &eyeStaticData.fixation, &this->transform) == MLResult_Ok) {
    // nothing
  } else {
    ML_LOG(Error, "%s: ML failed to get eye fixation transform!", application_name);
  }

  if (MLSnapshotGetTransform(snapshot, &eyeStaticData.left_center, &this->leftTransform) == MLResult_Ok) {
    // nothing
  } else {
    ML_LOG(Error, "%s: ML failed to get left eye center transform!", application_name);
  }
  this->leftBlink = eyeState.left_blink;

  if (MLSnapshotGetTransform(snapshot, &eyeStaticData.right_center, &this->rightTransform) == MLResult_Ok) {
    // nothing
  } else {
    ML_LOG(Error, "%s: ML failed to get right eye center transform!", application_name);
  }
  this->rightBlink = eyeState.right_blink;

  MLMat4f transformMatrix = getWindowTransformMatrix(Nan::New(this->windowObj));

  if (!isIdentityMatrix(transformMatrix)) {
    {
      MLVec3f &position = this->transform.position;
      MLQuaternionf &rotation = this->transform.rotation;
      MLVec3f scale = {1, 1, 1};
      MLMat4f transform = multiplyMatrices(transformMatrix, composeMatrix(position, rotation, scale));
      decomposeMatrix(transform, position, rotation, scale);
    }
    {
      MLVec3f &position = this->leftTransform.position;
      MLQuaternionf &rotation = this->leftTransform.rotation;
      MLVec3f scale = {1, 1, 1};
      MLMat4f transform = multiplyMatrices(transformMatrix, composeMatrix(position, rotation, scale));
      decomposeMatrix(transform, position, rotation, scale);
    }
    {
      MLVec3f &position = this->rightTransform.position;
      MLQuaternionf &rotation = this->rightTransform.rotation;
      MLVec3f scale = {1, 1, 1};
      MLMat4f transform = multiplyMatrices(transformMatrix, composeMatrix(position, rotation, scale));
      decomposeMatrix(transform, position, rotation, scale);
    }
  }
} */

NAN_METHOD(MLEyeTracker::Destroy) {
  MLEyeTracker *mlEyeTracker = ObjectWrap::Unwrap<MLEyeTracker>(info.This());

  if (MLEyeTrackingDestroy(mlEyeTracker->tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to create eye handle", application_name);
  }
}

// keyboard callbacks

inline uint32_t mlKeycodeToKeycode(MLKeyCode mlKeycode) {
  switch (mlKeycode) {
    case MLKEYCODE_UNKNOWN: return 0;
    case MLKEYCODE_SOFT_LEFT: return 37;
    case MLKEYCODE_SOFT_RIGHT: return 39;
    /* MLKEYCODE_HOME                          = 3,
    MLKEYCODE_BACK                          = 4,
    MLKEYCODE_CALL                          = 5,
    MLKEYCODE_ENDCALL                       = 6, */
    case MLKEYCODE_0: return 48;
    case MLKEYCODE_1: return 49;
    case MLKEYCODE_2: return 50;
    case MLKEYCODE_3: return 51;
    case MLKEYCODE_4: return 52;
    case MLKEYCODE_5: return 53;
    case MLKEYCODE_6: return 54;
    case MLKEYCODE_7: return 55;
    case MLKEYCODE_8: return 56;
    case MLKEYCODE_9: return 57;
    case MLKEYCODE_STAR: return 56;
    case MLKEYCODE_POUND: return 51;
    case MLKEYCODE_DPAD_UP: return 38;
    case MLKEYCODE_DPAD_DOWN: return 40;
    case MLKEYCODE_DPAD_LEFT: return 37;
    case MLKEYCODE_DPAD_RIGHT: return 39;
    // MLKEYCODE_DPAD_CENTER                   = 23,
    /* MLKEYCODE_VOLUME_UP                     = 24,
    MLKEYCODE_VOLUME_DOWN                   = 25,
    MLKEYCODE_POWER                         = 26,
    MLKEYCODE_CAMERA                        = 27,
    MLKEYCODE_CLEAR                         = 28, */
    case MLKEYCODE_A: return 65;
    case MLKEYCODE_B: return 66;
    case MLKEYCODE_C: return 67;
    case MLKEYCODE_D: return 68;
    case MLKEYCODE_E: return 69;
    case MLKEYCODE_F: return 70;
    case MLKEYCODE_G: return 71;
    case MLKEYCODE_H: return 72;
    case MLKEYCODE_I: return 73;
    case MLKEYCODE_J: return 74;
    case MLKEYCODE_K: return 75;
    case MLKEYCODE_L: return 76;
    case MLKEYCODE_M: return 77;
    case MLKEYCODE_N: return 78;
    case MLKEYCODE_O: return 79;
    case MLKEYCODE_P: return 80;
    case MLKEYCODE_Q: return 81;
    case MLKEYCODE_R: return 82;
    case MLKEYCODE_S: return 83;
    case MLKEYCODE_T: return 84;
    case MLKEYCODE_U: return 85;
    case MLKEYCODE_V: return 86;
    case MLKEYCODE_W: return 87;
    case MLKEYCODE_X: return 88;
    case MLKEYCODE_Y: return 89;
    case MLKEYCODE_Z: return 90;
    case MLKEYCODE_COMMA: return 188;
    case MLKEYCODE_PERIOD: return 190;
    case MLKEYCODE_ALT_LEFT: return 18;
    case MLKEYCODE_ALT_RIGHT: return 18;
    case MLKEYCODE_SHIFT_LEFT: return 16;
    case MLKEYCODE_SHIFT_RIGHT: return 16;
    case MLKEYCODE_TAB: return 9;
    case MLKEYCODE_SPACE: return 32;
    // MLKEYCODE_SYM                           = 63,
    // MLKEYCODE_EXPLORER                      = 64,
    // MLKEYCODE_ENVELOPE                      = 65,
    case MLKEYCODE_ENTER: return 13;
    case MLKEYCODE_DEL: return 8;
    case MLKEYCODE_GRAVE: return 192;
    case MLKEYCODE_MINUS: return 189;
    case MLKEYCODE_EQUALS: return 187;
    case MLKEYCODE_LEFT_BRACKET: return 57;
    case MLKEYCODE_RIGHT_BRACKET: return 48;
    case MLKEYCODE_BACKSLASH: return 220;
    case MLKEYCODE_SEMICOLON: return 186;
    case MLKEYCODE_APOSTROPHE: return 222;
    case MLKEYCODE_SLASH: return 191;
    case MLKEYCODE_AT: return 50;
    /* MLKEYCODE_NUM                           = 78,
    MLKEYCODE_HEADSETHOOK                   = 79,
    MLKEYCODE_FOCUS                         = 80,
    MLKEYCODE_PLUS: return 187;
    MLKEYCODE_MENU                          = 82,
    MLKEYCODE_NOTIFICATION                  = 83,
    MLKEYCODE_SEARCH                        = 84,
    MLKEYCODE_MEDIA_PLAY_PAUSE              = 85,
    MLKEYCODE_MEDIA_STOP                    = 86,
    MLKEYCODE_MEDIA_NEXT                    = 87,
    MLKEYCODE_MEDIA_PREVIOUS                = 88,
    MLKEYCODE_MEDIA_REWIND                  = 89,
    MLKEYCODE_MEDIA_FAST_FORWARD            = 90,
    MLKEYCODE_MUTE                          = 91, */
    case MLKEYCODE_PAGE_UP: return 33;
    case MLKEYCODE_PAGE_DOWN: return 34;
    /* MLKEYCODE_PICTSYMBOLS                   = 94,
    MLKEYCODE_SWITCH_CHARSET                = 95,
    MLKEYCODE_BUTTON_A                      = 96,
    MLKEYCODE_BUTTON_B                      = 97,
    MLKEYCODE_BUTTON_C                      = 98,
    MLKEYCODE_BUTTON_X                      = 99,
    MLKEYCODE_BUTTON_Y                      = 100,
    MLKEYCODE_BUTTON_Z                      = 101,
    MLKEYCODE_BUTTON_L1                     = 102,
    MLKEYCODE_BUTTON_R1                     = 103,
    MLKEYCODE_BUTTON_L2                     = 104,
    MLKEYCODE_BUTTON_R2                     = 105,
    MLKEYCODE_BUTTON_THUMBL                 = 106,
    MLKEYCODE_BUTTON_THUMBR                 = 107,
    MLKEYCODE_BUTTON_START                  = 108,
    MLKEYCODE_BUTTON_SELECT                 = 109,
    MLKEYCODE_BUTTON_MODE                   = 110, */
    case MLKEYCODE_ESCAPE: return 27;
    case MLKEYCODE_FORWARD_DEL: return 46;
    case MLKEYCODE_CTRL_LEFT: return 17;
    case MLKEYCODE_CTRL_RIGHT: return 17;
    case MLKEYCODE_CAPS_LOCK: return 20;
    case MLKEYCODE_SCROLL_LOCK: return 145;
    case MLKEYCODE_META_LEFT: return 91;
    case MLKEYCODE_META_RIGHT: return 91;
    // MLKEYCODE_FUNCTION                      = 119,
    // MLKEYCODE_SYSRQ                         = 120,
    case MLKEYCODE_BREAK: return 19;
    case MLKEYCODE_MOVE_HOME: return 36;
    case MLKEYCODE_MOVE_END: return 35;
    case MLKEYCODE_INSERT: return 45;
    /* MLKEYCODE_FORWARD                       = 125,
    MLKEYCODE_MEDIA_PLAY                    = 126,
    MLKEYCODE_MEDIA_PAUSE                   = 127,
    MLKEYCODE_MEDIA_CLOSE                   = 128,
    MLKEYCODE_MEDIA_EJECT                   = 129,
    MLKEYCODE_MEDIA_RECORD                  = 130, */
    case MLKEYCODE_F1: return 112;
    case MLKEYCODE_F2: return 113;
    case MLKEYCODE_F3: return 114;
    case MLKEYCODE_F4: return 115;
    case MLKEYCODE_F5: return 116;
    case MLKEYCODE_F6: return 117;
    case MLKEYCODE_F7: return 118;
    case MLKEYCODE_F8: return 119;
    case MLKEYCODE_F9: return 120;
    case MLKEYCODE_F10: return 121;
    case MLKEYCODE_F11: return 122;
    case MLKEYCODE_F12: return 123;
    /* MLKEYCODE_NUM_LOCK                      = 143,
    MLKEYCODE_NUMPAD_0                      = 144,
    MLKEYCODE_NUMPAD_1                      = 145,
    MLKEYCODE_NUMPAD_2                      = 146,
    MLKEYCODE_NUMPAD_3                      = 147,
    MLKEYCODE_NUMPAD_4                      = 148,
    MLKEYCODE_NUMPAD_5                      = 149,
    MLKEYCODE_NUMPAD_6                      = 150,
    MLKEYCODE_NUMPAD_7                      = 151,
    MLKEYCODE_NUMPAD_8                      = 152,
    MLKEYCODE_NUMPAD_9                      = 153,
    MLKEYCODE_NUMPAD_DIVIDE                 = 154,
    MLKEYCODE_NUMPAD_MULTIPLY               = 155,
    MLKEYCODE_NUMPAD_SUBTRACT               = 156,
    MLKEYCODE_NUMPAD_ADD                    = 157,
    MLKEYCODE_NUMPAD_DOT                    = 158,
    MLKEYCODE_NUMPAD_COMMA                  = 159,
    MLKEYCODE_NUMPAD_ENTER                  = 160,
    MLKEYCODE_NUMPAD_EQUALS                 = 161,
    MLKEYCODE_NUMPAD_LEFT_PAREN             = 162,
    MLKEYCODE_NUMPAD_RIGHT_PAREN            = 163,
    MLKEYCODE_VOLUME_MUTE                   = 164,
    MLKEYCODE_INFO                          = 165,
    MLKEYCODE_CHANNEL_UP                    = 166,
    MLKEYCODE_CHANNEL_DOWN                  = 167,
    MLKEYCODE_ZOOM_IN                       = 168,
    MLKEYCODE_ZOOM_OUT                      = 169,
    MLKEYCODE_TV                            = 170,
    MLKEYCODE_WINDOW                        = 171,
    MLKEYCODE_GUIDE                         = 172,
    MLKEYCODE_DVR                           = 173,
    MLKEYCODE_BOOKMARK                      = 174,
    MLKEYCODE_CAPTIONS                      = 175,
    MLKEYCODE_SETTINGS                      = 176,
    MLKEYCODE_TV_POWER                      = 177,
    MLKEYCODE_TV_INPUT                      = 178,
    MLKEYCODE_STB_POWER                     = 179,
    MLKEYCODE_STB_INPUT                     = 180,
    MLKEYCODE_AVR_POWER                     = 181,
    MLKEYCODE_AVR_INPUT                     = 182,
    MLKEYCODE_PROG_RED                      = 183,
    MLKEYCODE_PROG_GREEN                    = 184,
    MLKEYCODE_PROG_YELLOW                   = 185,
    MLKEYCODE_PROG_BLUE                     = 186,
    MLKEYCODE_APP_SWITCH                    = 187,
    MLKEYCODE_BUTTON_1                      = 188,
    MLKEYCODE_BUTTON_2                      = 189,
    MLKEYCODE_BUTTON_3                      = 190,
    MLKEYCODE_BUTTON_4                      = 191,
    MLKEYCODE_BUTTON_5                      = 192,
    MLKEYCODE_BUTTON_6                      = 193,
    MLKEYCODE_BUTTON_7                      = 194,
    MLKEYCODE_BUTTON_8                      = 195,
    MLKEYCODE_BUTTON_9                      = 196,
    MLKEYCODE_BUTTON_10                     = 197,
    MLKEYCODE_BUTTON_11                     = 198,
    MLKEYCODE_BUTTON_12                     = 199,
    MLKEYCODE_BUTTON_13                     = 200,
    MLKEYCODE_BUTTON_14                     = 201,
    MLKEYCODE_BUTTON_15                     = 202,
    MLKEYCODE_BUTTON_16                     = 203,
    MLKEYCODE_LANGUAGE_SWITCH               = 204,
    MLKEYCODE_MANNER_MODE                   = 205,
    MLKEYCODE_3D_MODE                       = 206,
    MLKEYCODE_CONTACTS                      = 207,
    MLKEYCODE_CALENDAR                      = 208,
    MLKEYCODE_MUSIC                         = 209,
    MLKEYCODE_CALCULATOR                    = 210,
    MLKEYCODE_ZENKAKU_HANKAKU               = 211,
    MLKEYCODE_EISU                          = 212,
    MLKEYCODE_MUHENKAN                      = 213,
    MLKEYCODE_HENKAN                        = 214,
    MLKEYCODE_KATAKANA_HIRAGANA             = 215,
    MLKEYCODE_YEN                           = 216,
    MLKEYCODE_RO                            = 217,
    MLKEYCODE_KANA                          = 218,
    MLKEYCODE_ASSIST                        = 219,
    MLKEYCODE_BRIGHTNESS_DOWN               = 220,
    MLKEYCODE_BRIGHTNESS_UP                 = 221,
    MLKEYCODE_MEDIA_AUDIO_TRACK             = 222,
    MLKEYCODE_SLEEP                         = 223,
    MLKEYCODE_WAKEUP                        = 224,
    MLKEYCODE_PAIRING                       = 225,
    MLKEYCODE_MEDIA_TOP_MENU                = 226,
    MLKEYCODE_11                            = 227,
    MLKEYCODE_12                            = 228,
    MLKEYCODE_LAST_CHANNEL                  = 229,
    MLKEYCODE_TV_DATA_SERVICE               = 230,
    MLKEYCODE_VOICE_ASSIST                  = 231,
    MLKEYCODE_TV_RADIO_SERVICE              = 232,
    MLKEYCODE_TV_TELETEXT                   = 233,
    MLKEYCODE_TV_NUMBER_ENTRY               = 234,
    MLKEYCODE_TV_TERRESTRIAL_ANALOG         = 235,
    MLKEYCODE_TV_TERRESTRIAL_DIGITAL        = 236,
    MLKEYCODE_TV_SATELLITE                  = 237,
    MLKEYCODE_TV_SATELLITE_BS               = 238,
    MLKEYCODE_TV_SATELLITE_CS               = 239,
    MLKEYCODE_TV_SATELLITE_SERVICE          = 240,
    MLKEYCODE_TV_NETWORK                    = 241,
    MLKEYCODE_TV_ANTENNA_CABLE              = 242,
    MLKEYCODE_TV_INPUT_HDMI_1               = 243,
    MLKEYCODE_TV_INPUT_HDMI_2               = 244,
    MLKEYCODE_TV_INPUT_HDMI_3               = 245,
    MLKEYCODE_TV_INPUT_HDMI_4               = 246,
    MLKEYCODE_TV_INPUT_COMPOSITE_1          = 247,
    MLKEYCODE_TV_INPUT_COMPOSITE_2          = 248,
    MLKEYCODE_TV_INPUT_COMPONENT_1          = 249,
    MLKEYCODE_TV_INPUT_COMPONENT_2          = 250,
    MLKEYCODE_TV_INPUT_VGA_1                = 251,
    MLKEYCODE_TV_AUDIO_DESCRIPTION          = 252,
    MLKEYCODE_TV_AUDIO_DESCRIPTION_MIX_UP   = 253,
    MLKEYCODE_TV_AUDIO_DESCRIPTION_MIX_DOWN = 254,
    MLKEYCODE_TV_ZOOM_MODE                  = 255,
    MLKEYCODE_TV_CONTENTS_MENU              = 256,
    MLKEYCODE_TV_MEDIA_CONTEXT_MENU         = 257,
    MLKEYCODE_TV_TIMER_PROGRAMMING          = 258,
    MLKEYCODE_HELP                          = 259,
    MLKEYCODE_NAVIGATE_PREVIOUS             = 260,
    MLKEYCODE_NAVIGATE_NEXT                 = 261,
    MLKEYCODE_NAVIGATE_IN                   = 262,
    MLKEYCODE_NAVIGATE_OUT                  = 263,
    MLKEYCODE_STEM_PRIMARY                  = 264,
    MLKEYCODE_STEM_1                        = 265,
    MLKEYCODE_STEM_2                        = 266,
    MLKEYCODE_STEM_3                        = 267, */
    case MLKEYCODE_DPAD_UP_LEFT: return 38;
    case MLKEYCODE_DPAD_DOWN_LEFT: return 40;
    case MLKEYCODE_DPAD_UP_RIGHT: return 38;
    case MLKEYCODE_DPAD_DOWN_RIGHT: return 40;
    /* MLKEYCODE_MEDIA_SKIP_FORWARD            = 272,
    MLKEYCODE_MEDIA_SKIP_BACKWARD           = 273,
    MLKEYCODE_MEDIA_STEP_FORWARD            = 274,
    MLKEYCODE_MEDIA_STEP_BACKWARD           = 275,
    MLKEYCODE_SOFT_SLEEP                    = 276,
    MLKEYCODE_CUT                           = 277,
    MLKEYCODE_COPY                          = 278,
    MLKEYCODE_PASTE                         = 279,
    MLKEYCODE_COUNT, */
    default: return 0;
  }
}

void setKeyEvent(Local<Object> obj, uint32_t charCode, MLKeyCode mlKeyCode, uint32_t modifier_mask) {
  uint32_t keyCode = mlKeycodeToKeycode(mlKeyCode);
  obj->Set(JS_STR("charCode"), JS_INT(charCode));
  obj->Set(JS_STR("keyCode"), JS_INT(keyCode));
  obj->Set(JS_STR("which"), JS_INT(keyCode));
  obj->Set(JS_STR("shiftKey"), JS_BOOL(modifier_mask & MLKEYMODIFIER_SHIFT));
  obj->Set(JS_STR("altKey"), JS_BOOL(modifier_mask & MLKEYMODIFIER_ALT));
  obj->Set(JS_STR("ctrlKey"), JS_BOOL(modifier_mask & MLKEYMODIFIER_CTRL));
  obj->Set(JS_STR("metaKey"), JS_BOOL(modifier_mask & MLKEYMODIFIER_META));
  // obj->Set(JS_STR("sym"), JS_BOOL(modifier_mask & MLKEYMODIFIER_SYM));
  // obj->Set(JS_STR("function"), JS_BOOL(modifier_mask & MLKEYMODIFIER_FUNCTION));
  // obj->Set(JS_STR("capsLock"), JS_BOOL(modifier_mask & MLKEYMODIFIER_CAPS_LOCK));
  // obj->Set(JS_STR("numLock"), JS_BOOL(modifier_mask & MLKEYMODIFIER_NUM_LOCK));
  // obj->Set(JS_STR("scrollLock"), JS_BOOL(modifier_mask & MLKEYMODIFIER_SCROLL_LOCK));
}

void onChar(uint32_t char_utf32, void *data) {
  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("keypress"));
    setKeyEvent(obj, char_utf32, MLKEYCODE_UNKNOWN, 0);

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}
void onKeyDown(MLKeyCode key_code, uint32_t modifier_mask, void *data) {
  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("keydown"));
    setKeyEvent(obj, 0, key_code, modifier_mask);

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}
void onKeyUp(MLKeyCode key_code, uint32_t modifier_mask, void *data) {
  QueueEvent([=](std::function<void(int, Local<Value> *)> eventHandlerFn) -> void {
    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("type"), JS_STR("keyup"));
    setKeyEvent(obj, 0, key_code, modifier_mask);

    Local<Value> argv[] = {
      obj,
    };
    eventHandlerFn(sizeof(argv)/sizeof(argv[0]), argv);
  });
}

// KeyboardEvent

/* KeyboardEvent::KeyboardEvent(KeyboardEventType type, uint32_t char_utf32) : type(type), char_utf32(char_utf32), key_code(MLKEYCODE_UNKNOWN), modifier_mask(0) {}
KeyboardEvent::KeyboardEvent(KeyboardEventType type, MLKeyCode key_code, uint32_t modifier_mask) : type(type), char_utf32(0), key_code(key_code), modifier_mask(modifier_mask) {} */

// camera device status callbacks

void cameraOnDeviceAvailable(void *data) {
  // XXX
}
void cameraOnDeviceUnavailable(void *data) {
  // XXX
}
void cameraOnDeviceOpened(void *data) {
  // XXX
}
void cameraOnDeviceClosed(void *data) {
  // XXX
}
void cameraOnDeviceDisconnected(void *data) {
  // XXX
}
void cameraOnDeviceError(MLCameraError error, void *data) {
  // XXX
}
void cameraOnPreviewBufferAvailable(MLHandle output, void *data) {
  // uv_async_send(&cameraAsync);
}

// camera capture callbacks

void cameraOnCaptureStarted(const MLCameraResultExtras *extra, void *data) {
  // XXX
}
void cameraOnCaptureFailed(const MLCameraResultExtras *extra, void *data) {
  // XXX
}
void cameraOnCaptureBufferLost(const MLCameraResultExtras *extra, void *data) {
  // XXX
}
void cameraOnCaptureProgressed(MLHandle metadata_handle, const MLCameraResultExtras *extra, void *data) {
  // XXX
}
void cameraOnCaptureCompleted(MLHandle metadata_handle, const MLCameraResultExtras *extra, void *data) {
  // XXX
}
void cameraOnImageBufferAvailable(const MLCameraOutput *output, void *data) {
  /* if (!cameraResponsePending) {
    {
      std::unique_lock<std::mutex> lock(cameraRequestsMutex);
      std::for_each(cameraRequests.begin(), cameraRequests.end(), [&](CameraRequest *c) {
        c->Set(output);
      });
    }
    cameraResponsePending = true;
  } */
}

// CameraRequest

CameraRequest::CameraRequest(uv_loop_t *loop, Local<Function> cbFn) : loop(loop), cbFn(cbFn) {}

void CameraRequest::Set(int width, int height, uint8_t *data, size_t size) {
  this->width = width;
  this->height = height;
  // this->stride = stride;

  Local<ArrayBuffer> arrayBuffer;
  if (size > 0) {
    // exout << "got jpeg " << size << std::endl;

    arrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), size);
    memcpy(arrayBuffer->GetContents().Data(), data, size);
  } else {
    // exout << "failed to get jpeg " << size << std::endl;

    arrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), 4);
  }
  this->data.Reset(arrayBuffer);
}

void CameraRequest::Update() {
  QueueCallback(loop, [this]() -> void {
    Local<Object> asyncObject = Nan::New<Object>();
    AsyncResource asyncResource(Isolate::GetCurrent(), asyncObject, "CameraRequest::Update");

    Local<Function> cbFn = Nan::New(this->cbFn);

    Local<Object> obj = Nan::New<Object>();
    obj->Set(JS_STR("data"), Nan::New(data));
    data.Reset();
    obj->Set(JS_STR("width"), JS_INT(width));
    obj->Set(JS_STR("height"), JS_INT(height));
    // obj->Set(JS_STR("bpp"), JS_INT(bpp));
    // obj->Set(JS_STR("stride"), JS_INT(stride));

    Local<Value> argv[] = {
      obj,
    };
    asyncResource.MakeCallback(cbFn, sizeof(argv)/sizeof(argv[0]), argv);
  });
}

// MLImageTracker

bool imageTrackingEnabled = false;
size_t numImageTrackers = 0;

MLImageTracker::MLImageTracker(Local<Object> windowObj, MLHandle trackerHandle, float size) : windowObj(windowObj), trackerHandle(trackerHandle), size(size), valid(false) {}

MLImageTracker::~MLImageTracker() {}

NAN_METHOD(MLImageTracker::New) {
  if (!imageTrackingEnabled) {
    imageTrackerSettings.enable_image_tracking = true;
    imageTrackerSettings.max_simultaneous_targets = 2;

    MLResult result = MLImageTrackerCreate(&imageTrackerSettings, &imageTrackerHandle);

    if (result == MLResult_Ok) {
      imageTrackingEnabled = true;
    } else {
      ML_LOG(Error, "%s: Failed to connect camera: %x", application_name, result);
      Nan::ThrowError("failed to connect camera");
    }
  }

  Local<Object> windowObj = Local<Object>::Cast(info[0]);
  Local<Object> imageObj = Local<Object>::Cast(info[1]);
  Local<Number> dimensionNumber = Local<Number>::Cast(info[2]);

  MLHandle trackerHandle;
  MLImageTrackerTargetSettings trackerSettings;
  trackerSettings.is_enabled = true;
  trackerSettings.is_stationary = true;
  float longerDimension = TO_FLOAT(dimensionNumber);
  trackerSettings.longer_dimension = longerDimension;
  char name[64];
  sprintf(name, "tracker%u", numImageTrackers++);
  trackerSettings.name = name;

  Image *image = ObjectWrap::Unwrap<Image>(imageObj);
  Local<Uint8ClampedArray> dataArray = Local<Uint8ClampedArray>::Cast(imageObj->Get(JS_STR("data")));
  Local<ArrayBuffer> dataArrayBuffer = dataArray->Buffer();

  uint32_t width = image->GetWidth();
  uint32_t height = image->GetHeight();
  MLResult result = MLImageTrackerAddTargetFromArray(
    imageTrackerHandle,
    &trackerSettings,
    (uint8_t *)dataArrayBuffer->GetContents().Data() + dataArray->ByteOffset(),
    width,
    height,
    MLImageTrackerImageFormat_RGBA,
    &trackerHandle
  );

  if (result == MLResult_Ok) {
    MLImageTracker *mlImageTracker = new MLImageTracker(windowObj, trackerHandle, longerDimension);
    Local<Object> mlImageTrackerObj = info.This();
    mlImageTracker->Wrap(mlImageTrackerObj);

    Nan::SetAccessor(mlImageTrackerObj, JS_STR("ontrack"), OnTrackGetter, OnTrackSetter);

    info.GetReturnValue().Set(mlImageTrackerObj);

    imageTrackers.push_back(mlImageTracker);
  } else {
    Nan::ThrowError("MLImageTracker::New: failed to create tracker");
  }
}

Local<Function> MLImageTracker::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("MLImageTracker"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "destroy", Destroy);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

NAN_GETTER(MLImageTracker::OnTrackGetter) {
  // Nan::HandleScope scope;
  MLImageTracker *mlImageTracker = ObjectWrap::Unwrap<MLImageTracker>(info.This());

  Local<Function> cb = Nan::New(mlImageTracker->cb);
  info.GetReturnValue().Set(cb);
}

NAN_SETTER(MLImageTracker::OnTrackSetter) {
  // Nan::HandleScope scope;
  MLImageTracker *mlImageTracker = ObjectWrap::Unwrap<MLImageTracker>(info.This());

  if (value->IsFunction()) {
    uv_loop_t *loop = windowsystembase::GetEventLoop();
    mlImageTracker->loop = loop;

    Local<Function> localCb = Local<Function>::Cast(value);
    mlImageTracker->cb.Reset(localCb);
  } else {
    Nan::ThrowError("MLImageTracker::OnTrackSetter: invalid arguments");
  }
}

void MLImageTracker::Update(MLSnapshot *snapshot) {
  MLImageTrackerTargetResult trackerTargetResult;
  MLResult result = MLImageTrackerGetTargetResult(
    imageTrackerHandle,
    trackerHandle,
    &trackerTargetResult
  );
  if (result == MLResult_Ok) {
    const bool &lastValid = valid;
    MLImageTrackerTargetStatus &status = trackerTargetResult.status;
    const bool newValid = (status == MLImageTrackerTargetStatus_Tracked || status == MLImageTrackerTargetStatus_Unreliable);

    if (newValid) {
      MLImageTrackerTargetStaticData trackerTargetStaticData;
      MLResult result = MLImageTrackerGetTargetStaticData(
        imageTrackerHandle,
        trackerHandle,
        &trackerTargetStaticData
      );
      if (result == MLResult_Ok) {
        MLTransform transform;
        MLResult result = MLSnapshotGetTransform(snapshot, &trackerTargetStaticData.coord_frame_target, &transform);

        if (result == MLResult_Ok) {
          MLVec3f &position = transform.position;
          MLQuaternionf &rotation = transform.rotation;
          MLVec3f scale = {1, 1, 1};

          MLMat4f transformMatrix = getWindowTransformMatrix(Nan::New(this->windowObj));
          if (!isIdentityMatrix(transformMatrix)) {
            MLMat4f transform = multiplyMatrices(transformMatrix, composeMatrix(position, rotation, scale));
            decomposeMatrix(transform, position, rotation, scale);
          }

          // Local<Object> localWindowObj = Nan::New(this->windowObj);

          QueueCallback(loop, [this, position, rotation]() -> void {
            if (!this->cb.IsEmpty()) {
              Local<Object> asyncObject = Nan::New<Object>();
              AsyncResource asyncResource(Isolate::GetCurrent(), asyncObject, "MLImageTracker::Update");

              Local<Function> cbFn = Nan::New(this->cb);
              Local<Object> objVal = Nan::New<Object>();

              Local<ArrayBuffer> arrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), (3+4)*sizeof(float));
              char *arrayBufferData = (char *)arrayBuffer->GetContents().Data();
              size_t index = 0;

              memcpy(arrayBufferData + index, position.values, sizeof(position.values));
              objVal->Set(JS_STR("position"), Float32Array::New(arrayBuffer, index, sizeof(position.values)/sizeof(position.values[0])));
              index += sizeof(position.values);

              memcpy(arrayBufferData + index, rotation.values, sizeof(rotation.values));
              objVal->Set(JS_STR("rotation"), Float32Array::New(arrayBuffer, index, sizeof(rotation.values)/sizeof(rotation.values[0])));
              index += sizeof(rotation.values);

              objVal->Set(JS_STR("size"), JS_NUM(this->size));

              Local<Value> argv[] = {
                objVal,
              };

              asyncResource.MakeCallback(cbFn, sizeof(argv)/sizeof(argv[0]), argv);
            }
          });

          valid = newValid;
        } else {
          ML_LOG(Error, "%s: ML failed to get eye fixation transform!", application_name);
        }
      } else {
        ML_LOG(Error, "%s: Image tracker get static data failed! %x", application_name, result);
      }
    } else {
      if (lastValid) {
        // Local<Object> localWindowObj = Nan::New(this->windowObj);

        QueueCallback(loop, [this]() -> void {
          if (!this->cb.IsEmpty()) {
            Local<Object> asyncObject = Nan::New<Object>();
            AsyncResource asyncResource(Isolate::GetCurrent(), asyncObject, "MLImageTracker::Update");

            Local<Function> cbFn = Nan::New(cb);
            Local<Value> objVal = Nan::Null();
            Local<Value> argv[] = {
              objVal,
            };

            asyncResource.MakeCallback(cbFn, sizeof(argv)/sizeof(argv[0]), argv);
          }
        });
      }

      valid = newValid;
    }
  } else {
    ML_LOG(Error, "%s: ML failed to get image tracker target result!", application_name);
  }
}

NAN_METHOD(MLImageTracker::Destroy) {
  MLImageTracker *mlImageTracker = ObjectWrap::Unwrap<MLImageTracker>(info.This());

  imageTrackers.erase(std::remove_if(imageTrackers.begin(), imageTrackers.end(), [&](MLImageTracker *i) -> bool {
    if (i == mlImageTracker) {
      MLResult result = MLImageTrackerRemoveTarget(imageTrackerHandle, i->trackerHandle);
      if (result != MLResult_Ok) {
        ML_LOG(Error, "%s: ML failed to remove image tracker target!", application_name);
      }

      delete i;
      return true;
    } else {
      return false;
    }
  }));
}

// MLContext

MLContext::MLContext() : window(nullptr), position{0, 0, 0}, rotation{0, 0, 0, 1}, cameraInTexture(0), /*contentTexture(0),*/ cameraOutTexture(0), cameraFbo(0) {}

MLContext::~MLContext() {}

Handle<Object> MLContext::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "Present", Present);
  Nan::SetMethod(proto, "Exit", Exit);
  Nan::SetMethod(proto, "GetSize", GetSize);
  // Nan::SetMethod(proto, "SetContentTexture", SetContentTexture);
  Nan::SetMethod(proto, "WaitGetPoses", WaitGetPoses);
  // Nan::SetMethod(proto, "PrepareFrame", PrepareFrame);
  Nan::SetMethod(proto, "SubmitFrame", SubmitFrame);
  Nan::SetMethod(proto, "requestHitTest", RequestHitTest);
  Nan::SetMethod(proto, "requestMeshing", RequestMeshing);
  Nan::SetMethod(proto, "requestPlaneTracking", RequestPlaneTracking);
  Nan::SetMethod(proto, "requestHandTracking", RequestHandTracking);
  Nan::SetMethod(proto, "requestEyeTracking", RequestEyeTracking);
  Nan::SetMethod(proto, "requestImageTracking", RequestImageTracking);
  // Nan::SetMethod(proto, "RequestDepthPopulation", RequestDepthPopulation);
  // Nan::SetMethod(proto, "RequestCamera", RequestCamera);
  // Nan::SetMethod(proto, "CancelCamera", CancelCamera);
  Nan::SetMethod(proto, "update", Update);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  Nan::SetMethod(ctorFn, "InitLifecycle", InitLifecycle);
  Nan::SetMethod(ctorFn, "DeinitLifecycle", DeinitLifecycle);
  Nan::SetMethod(ctorFn, "SetEventHandler", SetEventHandler);
  Nan::SetMethod(ctorFn, "IsPresent", IsPresent);
  Nan::SetMethod(ctorFn, "IsSimulated", IsSimulated);
  // Nan::SetMethod(ctorFn, "Poll", Poll);

  return scope.Escape(ctorFn);
}

NAN_METHOD(MLContext::New) {
  Local<Object> mlContextObj = info.This();
  MLContext *mlContext = new MLContext();
  mlContext->Wrap(mlContextObj);

  info.GetReturnValue().Set(mlContextObj);
}

void RunEventsInMainThread(uv_async_t *async) {
  Nan::HandleScope scope;

  std::deque<std::function<void(std::function<void(int argc, Local<Value> *argv)>)>> localFns;
  Local<Function> handlerFn;
  {
    std::lock_guard<std::mutex> lock(eventHandlerMutex);

    localFns = std::move(eventHandler->fns);
    eventHandler->fns.clear();

    handlerFn = Nan::New(eventHandler->handlerFn);
  }
  for (auto iter = localFns.begin(); iter != localFns.end(); iter++) {
    Nan::HandleScope scope;

    (*iter)([&](int argc, Local<Value> *argv) -> void {
      Local<Object> asyncObject = Nan::New<Object>();
      AsyncResource asyncResource(Isolate::GetCurrent(), asyncObject, "mlEvents");

      asyncResource.MakeCallback(handlerFn, argc, argv);
    });
  }
}

/* void RunCameraInMainThread(uv_async_t *handle) {
  Nan::HandleScope scope;

  if (!cameraConvertPending) {
    MLHandle output;
    MLResult result = MLCameraGetPreviewStream(&output);

    if (result == MLResult_Ok) {
      // ANativeWindowBuffer_t *aNativeWindowBuffer = (ANativeWindowBuffer_t *)output;
      //
      MLContext *mlContext = application_context.mlContext;
      NATIVEwindow *window = application_context.window;
      WebGLRenderingContext *gl = application_context.gl;

      EGLImageKHR yuv_img = eglCreateImageKHR(window->display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)(void*)output, nullptr);

      glBindVertexArray(mlContext->cameraVao);
      glBindFramebuffer(GL_FRAMEBUFFER, mlContext->cameraFbo);
      glUseProgram(mlContext->cameraProgram);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_EXTERNAL_OES, mlContext->cameraInTexture);
      glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, yuv_img);
      glUniform1i(mlContext->cameraInTextureLocation, 0);

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, mlContext->contentTexture);
      glUniform1i(mlContext->contentTextureLocation, 1);

      glViewport(0, 0, CAMERA_SIZE[0], CAMERA_SIZE[1]);
      glClear(GL_COLOR_BUFFER_BIT);
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

      eglDestroyImageKHR(window->display, yuv_img);

      glReadPixels(0, 0, CAMERA_SIZE[0], CAMERA_SIZE[1], GL_RGB, GL_UNSIGNED_BYTE, cameraRequestRgb);

      cameraConvertPending = true;
      uv_sem_post(&cameraConvertSem);

      if (gl->HasFramebufferBinding(GL_READ_FRAMEBUFFER)) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->GetFramebufferBinding(GL_READ_FRAMEBUFFER));
      } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->defaultFramebuffer);
      }
      if (gl->HasFramebufferBinding(GL_DRAW_FRAMEBUFFER)) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->GetFramebufferBinding(GL_DRAW_FRAMEBUFFER));
      } else {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->defaultFramebuffer);
      }
      if (gl->HasProgramBinding()) {
        glUseProgram(gl->GetProgramBinding());
      } else {
        glUseProgram(0);
      }
      if (gl->viewportState.valid) {
        glViewport(gl->viewportState.x, gl->viewportState.y, gl->viewportState.w, gl->viewportState.h);
      } else {
        glViewport(0, 0, 1280, 1024);
      }
      if (gl->HasVertexArrayBinding()) {
        glBindVertexArray(gl->GetVertexArrayBinding());
      } else {
        glBindVertexArray(gl->defaultVao);
      }
      if (gl->HasBufferBinding(GL_ARRAY_BUFFER)) {
        glBindBuffer(GL_ARRAY_BUFFER, gl->GetBufferBinding(GL_ARRAY_BUFFER));
      } else {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
      }
      glActiveTexture(GL_TEXTURE0);
      if (gl->HasTextureBinding(GL_TEXTURE0, GL_TEXTURE_2D)) {
        glBindTexture(GL_TEXTURE_2D, gl->GetTextureBinding(GL_TEXTURE0, GL_TEXTURE_2D));
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
      }
      if (gl->HasTextureBinding(GL_TEXTURE0, GL_TEXTURE_EXTERNAL_OES)) {
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, gl->GetTextureBinding(GL_TEXTURE0, GL_TEXTURE_EXTERNAL_OES));
      } else {
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
      }
      glActiveTexture(GL_TEXTURE1);
      if (gl->HasTextureBinding(GL_TEXTURE1, GL_TEXTURE_2D)) {
        glBindTexture(GL_TEXTURE_2D, gl->GetTextureBinding(GL_TEXTURE1, GL_TEXTURE_2D));
      } else {
        glBindTexture(GL_TEXTURE_2D, 0);
      }
      if (gl->HasTextureBinding(GL_TEXTURE1, GL_TEXTURE_EXTERNAL_OES)) {
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, gl->GetTextureBinding(GL_TEXTURE1, GL_TEXTURE_EXTERNAL_OES));
      } else {
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
      }
      glActiveTexture(gl->activeTexture);
    } else {
      ML_LOG(Error, "%s: failed to get camera preview stream %x", application_name, result);
    }
  }
}

void RunCameraConvertInMainThread(uv_async_t *handle) {
  Nan::HandleScope scope;

  std::for_each(cameraRequests.begin(), cameraRequests.end(), [&](CameraRequest *c) {
    c->Set(CAMERA_SIZE[0], CAMERA_SIZE[1], cameraRequestJpeg, cameraRequestSize);
    c->Update();
  });

  if (cameraRequestSize > 0) {
    SjpegFreeBuffer(cameraRequestJpeg);
  }

  cameraConvertPending = false;
} */

const char *meshVsh = "\
#version 330\n\
\n\
in vec3 position;\n\
\n\
uniform mat4 projectionMatrix;\n\
uniform mat4 modelViewMatrix;\n\
\n\
void main() {\n\
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);\n\
}\n\
";
const char *meshFsh = "\
#version 330\n\
\n\
out vec4 fragColor;\n\
\n\
void main() {\n\
  fragColor = vec4(1.0);\n\
}\n\
";
/* const char *cameraVsh = "\
#version 330\n\
\n\
in vec2 point;\n\
in vec2 uv;\n\
out vec2 vUvCamera;\n\
out vec2 vUvContent;\n\
\n\
float cameraAspectRatio = 960.0/540.0;\n\
float renderAspectRatio = 1280.0/960.0;\n\
float xfactor = cameraAspectRatio/renderAspectRatio;\n\
float factor = 1.4;\n\
\n\
void main() {\n\
  vUvCamera = vec2(uv.x, 1.0 - uv.y);\n\
  vUvContent = vec2(\n\
    ((uv.x * 0.5) - 0.25) * xfactor*factor + 0.25,\n\
    (uv.y - 0.5) * factor + 0.5\n\
  );\n\
  gl_Position = vec4(point, 1.0, 1.0);\n\
}\n\
";
const char *cameraFsh = "\
#version 330\n\
#extension GL_OES_EGL_image_external : enable\n\
\n\
in vec2 vUvCamera;\n\
in vec2 vUvContent;\n\
out vec4 fragColor;\n\
\n\
uniform samplerExternalOES cameraInTexture;\n\
uniform sampler2D contentTexture;\n\
\n\
void main() {\n\
  vec4 cameraIn = texture2D(cameraInTexture, vUvCamera);\n\
\n\
  if (vUvContent.x >= 0.0 && vUvContent.x <= 0.5 && vUvContent.y >= 0.0 && vUvContent.y <= 1.0) {\n\
    vec4 content = texture2D(contentTexture, vUvContent);\n\
    fragColor = vec4((cameraIn.rgb * (1.0 - content.a)) + (content.rgb * content.a), 1.0);\n\
  } else {\n\
    fragColor = vec4(cameraIn.rgb, 1.0);\n\
  }\n\
}\n\
"; */
NAN_METHOD(MLContext::InitLifecycle) {
  /* uv_loop_t *loop = windowsystembase::GetEventLoop();
  uv_async_init(loop, &cameraAsync, RunCameraInMainThread);
  uv_async_init(loop, &cameraConvertAsync, RunCameraConvertInMainThread);
  uv_sem_init(&cameraConvertSem, 0); */

  lifecycle_callbacks.on_new_initarg = onNewInitArg;
  lifecycle_callbacks.on_stop = onStop;
  lifecycle_callbacks.on_pause = onPause;
  lifecycle_callbacks.on_resume = onResume;
  lifecycle_callbacks.on_unload_resources = onUnloadResources;
  lifecycle_status = MLLifecycleInit(&lifecycle_callbacks, (void *)(&application_context));
  if (lifecycle_status != MLResult_Ok) {
    Nan::ThrowError("MLContext::InitLifecycle failed to initialize lifecycle");
    return;
  }

  {
    cameraDeviceStatusCallbacks.on_device_available = cameraOnDeviceAvailable;
    cameraDeviceStatusCallbacks.on_device_unavailable = cameraOnDeviceUnavailable;
    cameraDeviceStatusCallbacks.on_device_opened = cameraOnDeviceOpened;
    cameraDeviceStatusCallbacks.on_device_closed = cameraOnDeviceClosed;
    cameraDeviceStatusCallbacks.on_device_disconnected = cameraOnDeviceDisconnected;
    cameraDeviceStatusCallbacks.on_device_error = cameraOnDeviceError;
    cameraDeviceStatusCallbacks.on_preview_buffer_available = cameraOnPreviewBufferAvailable;
    MLResult result = MLCameraSetDeviceStatusCallbacks(&cameraDeviceStatusCallbacks, nullptr);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "%s: failed to set camera device status callbacks %x", application_name, result);
      Nan::ThrowError("MLContext::InitLifecycle failed to set camera device status callbacks");
      return;
    }
  }

  {
    cameraCaptureCallbacks.on_capture_started = cameraOnCaptureStarted;
    cameraCaptureCallbacks.on_capture_failed = cameraOnCaptureFailed;
    cameraCaptureCallbacks.on_capture_buffer_lost = cameraOnCaptureBufferLost;
    cameraCaptureCallbacks.on_capture_progressed = cameraOnCaptureProgressed;
    cameraCaptureCallbacks.on_capture_completed = cameraOnCaptureCompleted;
    cameraCaptureCallbacks.on_image_buffer_available = cameraOnImageBufferAvailable;
    MLResult result = MLCameraSetCaptureCallbacks(&cameraCaptureCallbacks, nullptr);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "%s: failed to set camera device status callbacks %x", application_name, result);
      Nan::ThrowError("MLContext::InitLifecycle failed to set camera device status callbacks");
      return;
    }
  }

  // HACK: prevent exit hang
  std::atexit([]() {
    quick_exit(0);
  });

  application_context.dummy_value = DummyValue::STOPPED;
}

NAN_METHOD(MLContext::DeinitLifecycle) {
  application_context.dummy_value = DummyValue::STOPPED;
}

NAN_METHOD(MLContext::SetEventHandler) {
  if (info[0]->IsFunction()) {
    Local<Function> handlerFn = Local<Function>::Cast(info[0]);

    {
      std::lock_guard<std::mutex> lock(eventHandlerMutex);

      uv_async_t *async = new uv_async_t();
      uv_loop_t *loop = windowsystembase::GetEventLoop();
      uv_async_init(loop, async, RunEventsInMainThread);

      eventHandler = new EventHandler(async, handlerFn);
    }
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_METHOD(MLContext::Present) {
  MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());
  NATIVEwindow *window = (NATIVEwindow *)arrayToPointer(Local<Array>::Cast(info[0]));

  if (lifecycle_status != MLResult_Ok) {
    ML_LOG(Error, "%s: ML Present called before lifecycle initialized.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  application_context.mlContext = mlContext;
  application_context.window = window;

  // initialize perception system

  MLPerceptionSettings perception_settings;
  if (MLPerceptionInitSettings(&perception_settings) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to initialize perception.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }
  if (MLPerceptionStartup(&perception_settings) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to startup perception.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  // initialize graphics subsystem
  MLGraphicsOptions graphics_options = {MLGraphicsFlags_Default, MLSurfaceFormat_RGBA8UNorm, MLSurfaceFormat_D32Float};
  MLHandle opengl_context = reinterpret_cast<MLHandle>(windowsystem::GetGLContext(window));
  {
    MLResult result = MLGraphicsCreateClientGL(&graphics_options, opengl_context, &application_context.mlContext->graphics_client);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "%s: Failed to create graphics clent: %x", application_name, result);
      return;
    }
  }
  {
    MLResult result = MLGraphicsGetRenderTargets(application_context.mlContext->graphics_client, &application_context.mlContext->render_targets_info);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "%s: Failed to get graphics render targets: %x", application_name, result);
      return;
    }
  }

  glGenFramebuffers(1, &application_context.mlContext->dst_framebuffer_id);

  // Now that graphics is connected, the app is ready to go
  if (MLLifecycleSetReadyIndication() != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to indicate lifecycle ready.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  // initialize local graphics stack

  // windowsystem::SetCurrentWindowContext(window);

  /* unsigned int halfWidth = mlContext->render_targets_info.buffers[0].color.width;
  unsigned int width = halfWidth * 2;
  unsigned int height = mlContext->render_targets_info.buffers[0].color.height; */

  /* GLuint fbo;
  GLuint colorTex;
  GLuint depthStencilTex;
  GLuint msFbo;
  GLuint msColorTex;
  GLuint msDepthStencilTex;
  {
    bool ok = windowsystembase::CreateRenderTarget(gl, width, height, 0, 0, 0, 0, &fbo, &colorTex, &depthStencilTex, &msFbo, &msColorTex, &msDepthStencilTex);
    if (!ok) {
      ML_LOG(Error, "%s: Failed to create ML present render context.", application_name);
      info.GetReturnValue().Set(Nan::Null());
      return;
    }
  } */

  {
    // mesh shader

    glGenVertexArrays(1, &mlContext->meshVao);

    // vertex Shader
    mlContext->meshVertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(mlContext->meshVertex, 1, &meshVsh, NULL);
    glCompileShader(mlContext->meshVertex);
    GLint success;
    glGetShaderiv(mlContext->meshVertex, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[4096];
      GLsizei length;
      glGetShaderInfoLog(mlContext->meshVertex, sizeof(infoLog), &length, infoLog);
      infoLog[length] = '\0';
      exout << "ML mesh vertex shader compilation failed:\n" << infoLog << std::endl;
      return;
    };

    // fragment Shader
    mlContext->meshFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(mlContext->meshFragment, 1, &meshFsh, NULL);
    glCompileShader(mlContext->meshFragment);
    glGetShaderiv(mlContext->meshFragment, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[4096];
      GLsizei length;
      glGetShaderInfoLog(mlContext->meshFragment, sizeof(infoLog), &length, infoLog);
      infoLog[length] = '\0';
      exout << "ML mesh fragment shader compilation failed:\n" << infoLog << std::endl;
      return;
    };

    // shader Program
    mlContext->meshProgram = glCreateProgram();
    glAttachShader(mlContext->meshProgram, mlContext->meshVertex);
    glAttachShader(mlContext->meshProgram, mlContext->meshFragment);
    glLinkProgram(mlContext->meshProgram);
    glGetProgramiv(mlContext->meshProgram, GL_LINK_STATUS, &success);
    if (!success) {
      char infoLog[4096];
      GLsizei length;
      glGetShaderInfoLog(mlContext->meshProgram, sizeof(infoLog), &length, infoLog);
      infoLog[length] = '\0';
      exout << "ML mesh program linking failed\n" << infoLog << std::endl;
      return;
    }

    mlContext->positionLocation = glGetAttribLocation(mlContext->meshProgram, "position");
    if (mlContext->positionLocation == -1) {
      exout << "ML mesh program failed to get attrib location for 'position'" << std::endl;
      return;
    }
    mlContext->modelViewMatrixLocation = glGetUniformLocation(mlContext->meshProgram, "modelViewMatrix");
    if (mlContext->modelViewMatrixLocation == -1) {
      exout << "ML meshprogram failed to get uniform location for 'modelViewMatrix'" << std::endl;
      return;
    }
    mlContext->projectionMatrixLocation = glGetUniformLocation(mlContext->meshProgram, "projectionMatrix");
    if (mlContext->projectionMatrixLocation == -1) {
      exout << "ML mesh program failed to get uniform location for 'projectionMatrix'" << std::endl;
      return;
    }

    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(mlContext->meshVertex);
    glDeleteShader(mlContext->meshFragment);
  }

  /* {
    // camera shader

    glGenVertexArrays(1, &mlContext->cameraVao);
    glBindVertexArray(mlContext->cameraVao);

    // vertex Shader
    mlContext->cameraVertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(mlContext->cameraVertex, 1, &cameraVsh, NULL);
    glCompileShader(mlContext->cameraVertex);
    GLint success;
    glGetShaderiv(mlContext->cameraVertex, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[4096];
      GLsizei length;
      glGetShaderInfoLog(mlContext->cameraVertex, sizeof(infoLog), &length, infoLog);
      infoLog[length] = '\0';
      exout << "ML camera vertex shader compilation failed:\n" << infoLog << std::endl;
      return;
    };

    // fragment Shader
    mlContext->cameraFragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(mlContext->cameraFragment, 1, &cameraFsh, NULL);
    glCompileShader(mlContext->cameraFragment);
    glGetShaderiv(mlContext->cameraFragment, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[4096];
      GLsizei length;
      glGetShaderInfoLog(mlContext->cameraFragment, sizeof(infoLog), &length, infoLog);
      infoLog[length] = '\0';
      exout << "ML camera fragment shader compilation failed:\n" << infoLog << std::endl;
      return;
    };

    // shader Program
    mlContext->cameraProgram = glCreateProgram();
    glAttachShader(mlContext->cameraProgram, mlContext->cameraVertex);
    glAttachShader(mlContext->cameraProgram, mlContext->cameraFragment);
    glLinkProgram(mlContext->cameraProgram);
    glGetProgramiv(mlContext->cameraProgram, GL_LINK_STATUS, &success);
    if (!success) {
      char infoLog[4096];
      GLsizei length;
      glGetShaderInfoLog(mlContext->cameraProgram, sizeof(infoLog), &length, infoLog);
      infoLog[length] = '\0';
      exout << "ML camera program linking failed\n" << infoLog << std::endl;
      return;
    }

    mlContext->pointLocation = glGetAttribLocation(mlContext->cameraProgram, "point");
    if (mlContext->pointLocation == -1) {
      exout << "ML camera program failed to get attrib location for 'point'" << std::endl;
      return;
    }
    mlContext->uvLocation = glGetAttribLocation(mlContext->cameraProgram, "uv");
    if (mlContext->uvLocation == -1) {
      exout << "ML camera program failed to get attrib location for 'uv'" << std::endl;
      return;
    }
    mlContext->cameraInTextureLocation = glGetUniformLocation(mlContext->cameraProgram, "cameraInTexture");
    if (mlContext->cameraInTextureLocation == -1) {
      exout << "ML camera program failed to get uniform location for 'cameraInTexture'" << std::endl;
      return;
    }
    mlContext->contentTextureLocation = glGetUniformLocation(mlContext->cameraProgram, "contentTexture");
    if (mlContext->contentTextureLocation == -1) {
      exout << "ML camera program failed to get uniform location for 'contentTexture'" << std::endl;
      return;
    }

    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(mlContext->cameraVertex);
    glDeleteShader(mlContext->cameraFragment);

    glGenBuffers(1, &mlContext->pointBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mlContext->pointBuffer);
    static const GLfloat points[] = {
      -1.0f, -1.0f,
      1.0f, -1.0f,
      -1.0f, 1.0f,
      1.0f, 1.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(points), points, GL_STATIC_DRAW);
    glEnableVertexAttribArray(mlContext->pointLocation);
    glVertexAttribPointer(mlContext->pointLocation, 2, GL_FLOAT, false, 0, 0);

    glGenBuffers(1, &mlContext->uvBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mlContext->uvBuffer);
    static const GLfloat uvs[] = {
      0.0f, 1.0f,
      1.0f, 1.0f,
      0.0f, 0.0f,
      1.0f, 0.0f,
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);
    glEnableVertexAttribArray(mlContext->uvLocation);
    glVertexAttribPointer(mlContext->uvLocation, 2, GL_FLOAT, false, 0, 0);

    glGenTextures(1, &mlContext->cameraInTexture);
    // glBindTexture(GL_TEXTURE_2D, mlContext->cameraInTexture);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenTextures(1, &mlContext->cameraOutTexture);
    // glBindTexture(GL_TEXTURE_2D, mlContext->cameraOutTexture);
    glBindTexture(GL_TEXTURE_2D, mlContext->cameraOutTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, CAMERA_SIZE[0], CAMERA_SIZE[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &mlContext->cameraFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mlContext->cameraFbo);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlContext->cameraOutTexture, 0);

    {
      GLenum result = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
      if (result != GL_FRAMEBUFFER_COMPLETE) {
        ML_LOG(Error, "%s: Failed to create generate camera framebuffer: %x", application_name, result);
      }
    }

    if (gl->HasFramebufferBinding(GL_DRAW_FRAMEBUFFER)) {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->GetFramebufferBinding(GL_DRAW_FRAMEBUFFER));
    } else {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->defaultFramebuffer);
    }
    if (gl->HasVertexArrayBinding()) {
      glBindVertexArray(gl->GetVertexArrayBinding());
    } else {
      glBindVertexArray(gl->defaultVao);
    }
    if (gl->HasBufferBinding(GL_ARRAY_BUFFER)) {
      glBindBuffer(GL_ARRAY_BUFFER, gl->GetBufferBinding(GL_ARRAY_BUFFER));
    } else {
      glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    if (gl->HasTextureBinding(gl->activeTexture, GL_TEXTURE_2D)) {
      glBindTexture(GL_TEXTURE_2D, gl->GetTextureBinding(gl->activeTexture, GL_TEXTURE_2D));
    } else {
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  } */

  // initialize subsystems

  if (MLHeadTrackingCreate(&mlContext->head_tracker) != MLResult_Ok || MLHeadTrackingGetStaticData(mlContext->head_tracker, &mlContext->head_static_data) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to create head tracker.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  MLInputConfiguration inputConfiguration;
  for (int i = 0; i < MLInput_MaxControllers; i++) {
    inputConfiguration.dof[i] = MLInputControllerDof_6;
  }
  if (MLInputCreate(&inputConfiguration, &mlContext->inputTracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to create input tracker.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  {
    keyboardCallbacks.on_char = onChar;
    keyboardCallbacks.on_key_down = onKeyDown;
    keyboardCallbacks.on_key_up = onKeyUp;
    MLResult result = MLInputSetKeyboardCallbacks(mlContext->inputTracker, &keyboardCallbacks, nullptr);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "%s: failed to set keyboard callbacks %x", application_name, result);
      Nan::ThrowError("MLContext::Present failed to set keyboard callbacks");
      return;
    }
  }

  if (MLPlanesCreate(&floorTracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to create floor handle", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  mlContext->TickFloor();

  // uv_loop_t *loop = windowsystembase::GetEventLoop();

  // HACK: force the app to be "running"
  application_context.dummy_value = DummyValue::RUNNING;

  // ML_LOG(Info, "%s: Start loop.", application_name);

  /* Local<Object> result = Nan::New<Object>();
  result->Set(JS_STR("width"), JS_INT(halfWidth));
  result->Set(JS_STR("height"), JS_INT(height));
  result->Set(JS_STR("fbo"), JS_INT(fbo));
  result->Set(JS_STR("colorTex"), JS_INT(colorTex));
  result->Set(JS_STR("depthStencilTex"), JS_INT(depthStencilTex));
  result->Set(JS_STR("msFbo"), JS_INT(msFbo));
  result->Set(JS_STR("msColorTex"), JS_INT(msColorTex));
  result->Set(JS_STR("msDepthStencilTex"), JS_INT(msDepthStencilTex));
  info.GetReturnValue().Set(result); */
}

NAN_METHOD(MLContext::Exit) {
  ML_LOG(Info, "%s: MLContext exit start", application_name);

  MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());

  windowsystem::SetCurrentWindowContext(application_context.window);
  glDeleteFramebuffers(1, &application_context.mlContext->dst_framebuffer_id);

  if (MLGraphicsDestroyClient(&mlContext->graphics_client) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to create graphics clent.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  // destroy perception system
  if (MLHeadTrackingDestroy(mlContext->head_tracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to destroy head tracker.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  if (MLInputDestroy(mlContext->inputTracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to destroy input tracker.", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  if (MLPlanesDestroy(floorTracker) != MLResult_Ok) {
    ML_LOG(Error, "%s: failed to destroy floor handle", application_name);
    info.GetReturnValue().Set(Nan::Null());
    return;
  }

  if (MLPerceptionShutdown() != MLResult_Ok) {
    ML_LOG(Error, "%s: Failed to stop perception.", application_name);
    info.GetReturnValue().Set(JS_BOOL(false));
    return;
  }

  // HACK: force the app to be "stopped"
  application_context.dummy_value = DummyValue::STOPPED;

  ML_LOG(Info, "%s: MLContext exit end", application_name);
}

/* NAN_METHOD(MLContext::WaitGetPoses) {
  if (info[0]->IsObject() && info[1]->IsNumber() && info[2]->IsNumber() && info[3]->IsNumber() && info[4]->IsFloat32Array() && info[5]->IsFloat32Array() && info[6]->IsFloat32Array()) {
    MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());
    WebGLRenderingContext *gl = ObjectWrap::Unwrap<WebGLRenderingContext>(Local<Object>::Cast(info[0]));
    GLuint framebuffer = TO_UINT32(info[1]);
    GLuint width = TO_UINT32(info[2]);
    GLuint height = TO_UINT32(info[3]);

    Local<Float32Array> transformFloat32Array = Local<Float32Array>::Cast(info[4]);
    Local<Float32Array> projectionFloat32Array = Local<Float32Array>::Cast(info[5]);
    Local<Float32Array> controllersFloat32Array = Local<Float32Array>::Cast(info[6]);

    float *transformArray = (float *)((char *)transformFloat32Array->Buffer()->GetContents().Data() + transformFloat32Array->ByteOffset());
    float *projectionArray = (float *)((char *)projectionFloat32Array->Buffer()->GetContents().Data() + projectionFloat32Array->ByteOffset());
    float *controllersArray = (float *)((char *)controllersFloat32Array->Buffer()->GetContents().Data() + controllersFloat32Array->ByteOffset());

    windowsystem::SetCurrentWindowContext(gl->windowHandle);

    MLGraphicsFrameParams frame_params;
    MLResult result = MLGraphicsInitFrameParams(&frame_params);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "MLGraphicsInitFrameParams complained: %d", result);
    }
    frame_params.surface_scale = 1.0f;
    frame_params.projection_type = MLGraphicsProjectionType_Default;
    frame_params.near_clip = 0.1f;
    frame_params.far_clip = 100.0f;
    frame_params.focus_distance = 1.0f;

    result = MLGraphicsBeginFrame(mlContext->graphics_client, &frame_params, &mlContext->frame_handle, &mlContext->virtual_camera_array);

    if (result == MLResult_Ok) {
      mlContext->TickFloor();

      // transform
      for (int i = 0; i < 2; i++) {
        const MLGraphicsVirtualCameraInfo &cameraInfo = mlContext->virtual_camera_array.virtual_cameras[i];
        const MLTransform &transform = cameraInfo.transform;
        const MLVec3f &position = OffsetFloor(transform.position);
        transformArray[i*7 + 0] = position.x;
        transformArray[i*7 + 1] = position.y;
        transformArray[i*7 + 2] = position.z;
        transformArray[i*7 + 3] = transform.rotation.x;
        transformArray[i*7 + 4] = transform.rotation.y;
        transformArray[i*7 + 5] = transform.rotation.z;
        transformArray[i*7 + 6] = transform.rotation.w;

        const MLMat4f &projection = cameraInfo.projection;
        for (int j = 0; j < 16; j++) {
          projectionArray[i*16 + j] = projection.matrix_colmajor[j];
        }
      }

      // position
      {
        // std::unique_lock<std::mutex> lock(mlContext->positionMutex);

        const MLTransform &leftCameraTransform = mlContext->virtual_camera_array.virtual_cameras[0].transform;
        mlContext->position = OffsetFloor(leftCameraTransform.position);
        mlContext->rotation = leftCameraTransform.rotation;
      }

      // controllers
      MLInputControllerState controllerStates[MLInput_MaxControllers];
      result = MLInputGetControllerState(mlContext->inputTracker, controllerStates);
      if (result == MLResult_Ok) {
        for (int i = 0; i < 2 && i < MLInput_MaxControllers; i++) {
          const MLInputControllerState &controllerState = controllerStates[i];
          bool is_connected = controllerState.is_connected;
          const MLVec3f &position = OffsetFloor(controllerState.position);
          const MLQuaternionf &orientation = controllerState.orientation;
          const float trigger = controllerState.trigger_normalized;
          const float bumper = controllerState.button_state[MLInputControllerButton_Bumper] ? 1.0f : 0.0f;
          const float home = controllerState.button_state[MLInputControllerButton_HomeTap] ? 1.0f : 0.0f;
          const bool isTouchActive = controllerState.is_touch_active[0];
          const MLVec3f &touchPosAndForce = controllerState.touch_pos_and_force[0];
          const float touchForceZ = isTouchActive ? (touchPosAndForce.z > 0.5f ? 1.0f : 0.5f) : 0.0f;

          int index;
          controllersArray[index = i * CONTROLLER_ENTRY_SIZE] = is_connected ? 1 : 0;
          controllersArray[++index] = position.x;
          controllersArray[++index] = position.y;
          controllersArray[++index] = position.z;
          controllersArray[++index] = orientation.x;
          controllersArray[++index] = orientation.y;
          controllersArray[++index] = orientation.z;
          controllersArray[++index] = orientation.w;
          controllersArray[++index] = trigger;
          controllersArray[++index] = bumper;
          controllersArray[++index] = home;
          controllersArray[++index] = touchPosAndForce.x;
          controllersArray[++index] = touchPosAndForce.y;
          controllersArray[++index] = touchForceZ;
        }
      } else {
        ML_LOG(Error, "MLInputGetControllerState failed: %s", application_name);
      }

      if (depthEnabled) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        glBindVertexArray(mlContext->meshVao);

        glUseProgram(mlContext->meshProgram);

        for (const auto &iter : meshBuffers) {
          const MeshBuffer &meshBuffer = iter.second;

          if (meshBuffer.numIndices > 0) {
            glBindBuffer(GL_ARRAY_BUFFER, meshBuffer.positionBuffer);
            glEnableVertexAttribArray(mlContext->positionLocation);
            glVertexAttribPointer(mlContext->positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.indexBuffer);

            for (int side = 0; side < 2; side++) {
              const MLGraphicsVirtualCameraInfo &cameraInfo = mlContext->virtual_camera_array.virtual_cameras[side];
              const MLTransform &transform = cameraInfo.transform;
              const MLMat4f &modelView = invertMatrix(composeMatrix(transform.position, transform.rotation));
              glUniformMatrix4fv(mlContext->modelViewMatrixLocation, 1, false, modelView.matrix_colmajor);

              const MLMat4f &projection = cameraInfo.projection;
              glUniformMatrix4fv(mlContext->projectionMatrixLocation, 1, false, projection.matrix_colmajor);

              glViewport(side * width/2, 0, width/2, height);

              glDrawElements(GL_TRIANGLES, meshBuffer.numIndices, GL_UNSIGNED_SHORT, 0);
            }
          }
        }

        if (gl->HasFramebufferBinding(GL_DRAW_FRAMEBUFFER)) {
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->GetFramebufferBinding(GL_DRAW_FRAMEBUFFER));
        } else {
          glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->defaultFramebuffer);
        }
        if (gl->HasProgramBinding()) {
          glUseProgram(gl->GetProgramBinding());
        } else {
          glUseProgram(0);
        }
        if (gl->viewportState.valid) {
          glViewport(gl->viewportState.x, gl->viewportState.y, gl->viewportState.w, gl->viewportState.h);
        } else {
          glViewport(0, 0, 1280, 1024);
        }
        if (gl->HasVertexArrayBinding()) {
          glBindVertexArray(gl->GetVertexArrayBinding());
        } else {
          glBindVertexArray(gl->defaultVao);
        }
        if (gl->HasBufferBinding(GL_ARRAY_BUFFER)) {
          glBindBuffer(GL_ARRAY_BUFFER, gl->GetBufferBinding(GL_ARRAY_BUFFER));
        } else {
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        if (gl->colorMaskState.valid) {
          glColorMask(gl->colorMaskState.r, gl->colorMaskState.g, gl->colorMaskState.b, gl->colorMaskState.a);
        } else {
          glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
      }

      info.GetReturnValue().Set(JS_BOOL(true));
    } else {
      ML_LOG(Error, "MLGraphicsBeginFrame complained: %d", result);

      info.GetReturnValue().Set(JS_BOOL(false));
    }
  } else {
    Nan::ThrowError("MLContext::WaitGetPoses: invalid arguments");
  }
} */

NAN_METHOD(MLContext::WaitGetPoses) {
  if (info[0]->IsFloat32Array() && info[1]->IsFloat32Array() && info[2]->IsFloat32Array()) {
    MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());
    Local<Float32Array> transformFloat32Array = Local<Float32Array>::Cast(info[0]);
    Local<Float32Array> projectionFloat32Array = Local<Float32Array>::Cast(info[1]);
    Local<Float32Array> controllersFloat32Array = Local<Float32Array>::Cast(info[2]);

    float *transformArray = (float *)((char *)transformFloat32Array->Buffer()->GetContents().Data() + transformFloat32Array->ByteOffset());
    float *projectionArray = (float *)((char *)projectionFloat32Array->Buffer()->GetContents().Data() + projectionFloat32Array->ByteOffset());
    float *controllersArray = (float *)((char *)controllersFloat32Array->Buffer()->GetContents().Data() + controllersFloat32Array->ByteOffset());

    // windowsystem::SetCurrentWindowContext(gl->windowHandle);

    MLGraphicsFrameParams frame_params;
    MLResult result = MLGraphicsInitFrameParams(&frame_params);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "MLGraphicsInitFrameParams complained: %d", result);
    }
    frame_params.surface_scale = 1.0f;
    frame_params.projection_type = MLGraphicsProjectionType_Default;
    frame_params.near_clip = 0.1f;
    frame_params.far_clip = 100.0f;
    frame_params.focus_distance = 1.0f;

    result = MLGraphicsBeginFrame(mlContext->graphics_client, &frame_params, &mlContext->frame_handle, &mlContext->virtual_camera_array);

    bool frameOk = (result == MLResult_Ok);
    if (frameOk) {
      /* GLuint fbo;
      glGenFramebuffers(1, &fbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
      for (int i = 0; i < 2; i++) {
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mlContext->virtual_camera_array.color_id, 0, i);
        if (i == 0) {
          glClearColor(1.0, 0.0, 0.0, 1.0);
        } else {
          glClearColor(0.0, 0.0, 1.0, 1.0);
        }
        glClear(GL_COLOR_BUFFER_BIT);
      }
      glDeleteFramebuffers(1, &fbo); */

      mlContext->TickFloor();

      // transform
      for (int i = 0; i < 2; i++) {
        const MLGraphicsVirtualCameraInfo &cameraInfo = mlContext->virtual_camera_array.virtual_cameras[i];
        const MLTransform &transform = cameraInfo.transform;
        const MLVec3f &position = OffsetFloor(transform.position);
        transformArray[i*7 + 0] = position.x;
        transformArray[i*7 + 1] = position.y;
        transformArray[i*7 + 2] = position.z;
        transformArray[i*7 + 3] = transform.rotation.x;
        transformArray[i*7 + 4] = transform.rotation.y;
        transformArray[i*7 + 5] = transform.rotation.z;
        transformArray[i*7 + 6] = transform.rotation.w;

        const MLMat4f &projection = cameraInfo.projection;
        for (int j = 0; j < 16; j++) {
          projectionArray[i*16 + j] = projection.matrix_colmajor[j];
        }
      }

      // position
      {
        // std::unique_lock<std::mutex> lock(mlContext->positionMutex);

        const MLTransform &leftCameraTransform = mlContext->virtual_camera_array.virtual_cameras[0].transform;
        mlContext->position = leftCameraTransform.position;
        mlContext->rotation = leftCameraTransform.rotation;
      }

      // controllers
      MLInputControllerState controllerStates[MLInput_MaxControllers];
      result = MLInputGetControllerState(mlContext->inputTracker, controllerStates);
      if (result == MLResult_Ok) {
        for (int i = 0; i < 2 && i < MLInput_MaxControllers; i++) {
          const MLInputControllerState &controllerState = controllerStates[i];
          bool is_connected = controllerState.is_connected;
          const MLVec3f &position = OffsetFloor(controllerState.position);
          const MLQuaternionf &orientation = controllerState.orientation;
          const float trigger = controllerState.trigger_normalized;
          const float bumper = controllerState.button_state[MLInputControllerButton_Bumper] ? 1.0f : 0.0f;
          const float home = controllerState.button_state[MLInputControllerButton_HomeTap] ? 1.0f : 0.0f;
          const bool isTouchActive = controllerState.is_touch_active[0];
          const MLVec3f &touchPosAndForce = controllerState.touch_pos_and_force[0];
          const float touchForceZ = isTouchActive ? (touchPosAndForce.z > 0.5f ? 1.0f : 0.5f) : 0.0f;

          int index;
          controllersArray[index = i * CONTROLLER_ENTRY_SIZE] = is_connected ? 1 : 0;
          controllersArray[++index] = position.x;
          controllersArray[++index] = position.y;
          controllersArray[++index] = position.z;
          controllersArray[++index] = orientation.x;
          controllersArray[++index] = orientation.y;
          controllersArray[++index] = orientation.z;
          controllersArray[++index] = orientation.w;
          controllersArray[++index] = trigger;
          controllersArray[++index] = bumper;
          controllersArray[++index] = home;
          controllersArray[++index] = touchPosAndForce.x;
          controllersArray[++index] = touchPosAndForce.y;
          controllersArray[++index] = touchForceZ;
        }
      } else {
        ML_LOG(Error, "MLInputGetControllerState failed: %s", application_name);
      }
    } else {
      ML_LOG(Error, "MLGraphicsBeginFrame complained: %d", result);
    }

    info.GetReturnValue().Set(JS_BOOL(frameOk));
  } else {
    Nan::ThrowError("MLContext::WaitGetPoses: invalid arguments");
  }
}

/* NAN_METHOD(MLContext::PrepareFrame) {
  if (info[0]->IsObject() && info[1]->IsNumber() && info[2]->IsNumber() && info[3]->IsNumber()) {
    if (depthEnabled) {
      MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());
      WebGLRenderingContext *gl = ObjectWrap::Unwrap<WebGLRenderingContext>(Local<Object>::Cast(info[0]));
      GLuint framebuffer = TO_UINT32(info[1]);
      GLuint width = TO_UINT32(info[2]);
      GLuint height = TO_UINT32(info[3]);

      windowsystem::SetCurrentWindowContext(gl->windowHandle);

      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glClearColor(0.0, 0.0, 0.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

      glBindVertexArray(mlContext->meshVao);

      glUseProgram(mlContext->meshProgram);

      for (const auto &iter : meshBuffers) {
        const MeshBuffer &meshBuffer = iter.second;

        if (meshBuffer.numIndices > 0) {
          glBindBuffer(GL_ARRAY_BUFFER, meshBuffer.positionBuffer);
          glEnableVertexAttribArray(mlContext->positionLocation);
          glVertexAttribPointer(mlContext->positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

          glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.indexBuffer);

          for (int side = 0; side < 2; side++) {
            const MLGraphicsVirtualCameraInfo &cameraInfo = mlContext->virtual_camera_array.virtual_cameras[side];
            const MLTransform &transform = cameraInfo.transform;
            const MLMat4f &modelView = invertMatrix(composeMatrix(transform.position, transform.rotation));
            glUniformMatrix4fv(mlContext->modelViewMatrixLocation, 1, false, modelView.matrix_colmajor);

            const MLMat4f &projection = cameraInfo.projection;
            glUniformMatrix4fv(mlContext->projectionMatrixLocation, 1, false, projection.matrix_colmajor);

            glViewport(side * width/2, 0, width/2, height);

            glDrawElements(GL_TRIANGLES, meshBuffer.numIndices, GL_UNSIGNED_SHORT, 0);
          }
        }
      }

      if (gl->HasFramebufferBinding(GL_DRAW_FRAMEBUFFER)) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->GetFramebufferBinding(GL_DRAW_FRAMEBUFFER));
      } else {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->defaultFramebuffer);
      }
      if (gl->HasProgramBinding()) {
        glUseProgram(gl->GetProgramBinding());
      } else {
        glUseProgram(0);
      }
      if (gl->viewportState.valid) {
        glViewport(gl->viewportState.x, gl->viewportState.y, gl->viewportState.w, gl->viewportState.h);
      } else {
        glViewport(0, 0, 1280, 1024);
      }
      if (gl->HasVertexArrayBinding()) {
        glBindVertexArray(gl->GetVertexArrayBinding());
      } else {
        glBindVertexArray(gl->defaultVao);
      }
      if (gl->HasBufferBinding(GL_ARRAY_BUFFER)) {
        glBindBuffer(GL_ARRAY_BUFFER, gl->GetBufferBinding(GL_ARRAY_BUFFER));
      } else {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
      }
      if (gl->colorMaskState.valid) {
        glColorMask(gl->colorMaskState.r, gl->colorMaskState.g, gl->colorMaskState.b, gl->colorMaskState.a);
      } else {
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      }
    }

    // info.GetReturnValue().Set(JS_BOOL(true));
  } else {
    Nan::ThrowError("MLContext::PrepareFrame: invalid arguments");
  }
} */

NAN_METHOD(MLContext::SubmitFrame) {
  MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());

  if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber()) {
    // WebGLRenderingContext *gl = ObjectWrap::Unwrap<WebGLRenderingContext>(Local<Object>::Cast(info[0]));
    GLuint fbo = TO_UINT32(info[0]);
    unsigned int width = TO_UINT32(info[1]);
    unsigned int height = TO_UINT32(info[2]);

    const MLRectf &viewport = application_context.mlContext->virtual_camera_array.viewport;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, application_context.mlContext->dst_framebuffer_id);

    for (int i = 0; i < 2; i++) {
      glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, application_context.mlContext->virtual_camera_array.color_id, 0, i);
      // glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, application_context.mlContext->virtual_camera_array.depth_id, 0, i);

      glBlitFramebuffer(i == 0 ? 0 : width/2, 0,
        i == 0 ? width/2 : width, height,
        viewport.x, viewport.y,
        viewport.w, viewport.h,
        GL_COLOR_BUFFER_BIT,
        GL_LINEAR);

      MLGraphicsVirtualCameraInfo &camera = application_context.mlContext->virtual_camera_array.virtual_cameras[i];
      MLResult result = MLGraphicsSignalSyncObjectGL(application_context.mlContext->graphics_client, camera.sync_object);
      if (result != MLResult_Ok) {
        ML_LOG(Error, "MLGraphicsSignalSyncObjectGL complained: %d", result);
      }
    }

    MLResult result = MLGraphicsEndFrame(application_context.mlContext->graphics_client, application_context.mlContext->frame_handle);
    if (result != MLResult_Ok) {
      ML_LOG(Error, "MLGraphicsEndFrame complained: %d", result);
    }

    // glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* if (gl->HasFramebufferBinding(GL_READ_FRAMEBUFFER)) {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->GetFramebufferBinding(GL_READ_FRAMEBUFFER));
    } else {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, gl->defaultFramebuffer);
    }
    if (gl->HasFramebufferBinding(GL_DRAW_FRAMEBUFFER)) {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->GetFramebufferBinding(GL_DRAW_FRAMEBUFFER));
    } else {
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl->defaultFramebuffer);
    } */
  } else {
    Nan::ThrowError("MLContext::SubmitFrame: invalid arguments");
  }
}

NAN_METHOD(MLContext::IsPresent) {
  info.GetReturnValue().Set(JS_BOOL(isPresent()));
}

NAN_METHOD(MLContext::IsSimulated) {
  info.GetReturnValue().Set(JS_BOOL(isSimulated()));
}

NAN_METHOD(MLContext::GetSize) {
  MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());
  unsigned int halfWidth = mlContext->render_targets_info.buffers[0].color.width;
  // unsigned int width = halfWidth * 2;
  unsigned int height = mlContext->render_targets_info.buffers[0].color.height;

  Local<Object> result = Nan::New<Object>();
  result->Set(JS_STR("width"), JS_INT(halfWidth));
  result->Set(JS_STR("height"), JS_INT(height));
  info.GetReturnValue().Set(result);
}

/* NAN_METHOD(MLContext::SetContentTexture) {
  if (info[0]->IsNumber()) {
    MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());
    mlContext->contentTexture = TO_UINT32(info[0]);
  } else {
    Nan::ThrowError("MLContext::SetContentTexture: invalid arguments");
  }
} */

NAN_METHOD(MLContext::RequestMeshing) {
  if (info[0]->IsNumber() && info[1]->IsNumber()) {
    MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(Local<Object>::Cast(info.This()));
    if (mlContext->mlMesherConstructor.IsEmpty()) {
      mlContext->mlMesherConstructor.Reset(MLMesher::Initialize(Isolate::GetCurrent()));
    }
    Local<Function> mlMesherCons = Nan::New(mlContext->mlMesherConstructor);
    Local<Value> argv[] = {
      info[0],
      info[1],
    };
    Local<Object> mlMesherObj = mlMesherCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
    info.GetReturnValue().Set(mlMesherObj);
  } else {
    Nan::ThrowError("MLContext::RequestMeshing: invalid arguments");
  }
}

NAN_METHOD(MLContext::RequestPlaneTracking) {
  if (info[0]->IsNumber()) {
    MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(Local<Object>::Cast(info.This()));
    if (mlContext->mlPlaneTrackerConstructor.IsEmpty()) {
      mlContext->mlPlaneTrackerConstructor.Reset(MLPlaneTracker::Initialize(Isolate::GetCurrent()));
    }
    Local<Function> mlPlaneTrackerCons = Nan::New(mlContext->mlPlaneTrackerConstructor);
    Local<Value> argv[] = {
      info[0],
    };
    Local<Object> mlPlaneTrackerObj = mlPlaneTrackerCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
    info.GetReturnValue().Set(mlPlaneTrackerObj);
  } else {
    Nan::ThrowError("MLContext::RequestPlaneTracking: invalid arguments");
  }
}

NAN_METHOD(MLContext::RequestHitTest) {
  if (
    info[0]->IsFloat32Array() &&
    info[1]->IsFloat32Array()
  ) {
    /* raycastQuery.up_vector = MLVec3f{0, 1, 0};
    raycastQuery.horizontal_fov_degrees = 30;
    raycastQuery.width = 1;
    raycastQuery.height = 1;
    raycastQuery.collide_with_unobserved = false;

    MLMat4f transformMatrix = getWindowTransformMatrix(windowObj, false);
    if (!isIdentityMatrix(transformMatrix)) {
      MLVec3f &position = raycastQuery.position;
      MLVec3f &direction = raycastQuery.direction;
      MLQuaternionf rotation = getQuaternionFromUnitVectors(MLVec3f{0, 0, -1}, direction);
      MLVec3f scale = {1, 1, 1};
      MLMat4f transform = multiplyMatrices(transformMatrix, composeMatrix(position, rotation, scale));
      decomposeMatrix(transform, position, rotation, scale);
      direction = applyVectorQuaternion(MLVec3f{0, 0, -1}, rotation);
    } */

    MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(Local<Object>::Cast(info.This()));
    if (mlContext->mlRaycastTrackerConstructor.IsEmpty()) {
      mlContext->mlRaycastTrackerConstructor.Reset(MLRaycaster::Initialize(Isolate::GetCurrent()));
    }
    Local<Function> mlRaycastTrackerCons = Nan::New(mlContext->mlRaycastTrackerConstructor);

    Local<Value> argv[] = {
      info[0],
      info[1],
    };
    Local<Object> mlRaycastTrackerObj = mlRaycastTrackerCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
    info.GetReturnValue().Set(mlRaycastTrackerObj);

    /*MLHandle requestHandle;
    MLResult result = MLRaycastRequest(raycastTracker, &raycastQuery, &requestHandle);
    if (result == MLResult_Ok) {
      MLRaycaster *raycaster = new MLRaycaster(requestHandle, cb);
      raycasters.push_back(raycaster);
    } else {
      ML_LOG(Error, "%s: Failed to request raycast: %x %x", application_name, result);
      Nan::ThrowError("failed to request raycast");
    } */
  } else {
    Nan::ThrowError("MLContext::RequestHitTest: invalid arguments");
  }
}

NAN_METHOD(MLContext::RequestHandTracking) {
  MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(Local<Object>::Cast(info.This()));
  if (mlContext->mlHandTrackerConstructor.IsEmpty()) {
    mlContext->mlHandTrackerConstructor.Reset(MLHandTracker::Initialize(Isolate::GetCurrent()));
  }
  Local<Function> mlHandTrackerCons = Nan::New(mlContext->mlHandTrackerConstructor);
  /* Local<Value> argv[] = {
    info[0],
  }; */
  Local<Object> mlHandTrackerObj = mlHandTrackerCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), 0, nullptr).ToLocalChecked();
  info.GetReturnValue().Set(mlHandTrackerObj);
}

NAN_METHOD(MLContext::RequestEyeTracking) {
  MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(Local<Object>::Cast(info.This()));
  if (mlContext->mlEyeTrackerConstructor.IsEmpty()) {
    mlContext->mlEyeTrackerConstructor.Reset(MLEyeTracker::Initialize(Isolate::GetCurrent()));
  }

  Local<Function> mlEyeTrackerCons = Nan::New(mlContext->mlEyeTrackerConstructor);
  /* Local<Value> argv[] = {
    info[0],
  }; */
  Local<Object> mlEyeTrackerObj = mlEyeTrackerCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), 0, nullptr).ToLocalChecked();
  info.GetReturnValue().Set(mlEyeTrackerObj);
}

NAN_METHOD(MLContext::RequestImageTracking) {
  if (info[0]->IsObject() && info[1]->IsObject() && info[2]->IsNumber()) {
    Local<Object> windowObj = Local<Object>::Cast(info[0]);
    Local<Object> imageObj = Local<Object>::Cast(info[1]);

    if (
      JS_OBJ(imageObj->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("HTMLImageElement")) &&
      imageObj->Get(JS_STR("image"))->IsObject() &&
      JS_OBJ(imageObj->Get(JS_STR("image")))->Get(JS_STR("data"))->IsUint8ClampedArray()
    ) {
      MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(Local<Object>::Cast(info.This()));
      if (mlContext->mlImageTrackerConstructor.IsEmpty()) {
        mlContext->mlImageTrackerConstructor.Reset(MLImageTracker::Initialize(Isolate::GetCurrent()));
      }
      Local<Function> mlImageTrackerCons = Nan::New(mlContext->mlImageTrackerConstructor);
      Local<Value> argv[] = {
        windowObj,
        imageObj->Get(JS_STR("image")),
        info[2],
      };
      Local<Object> mlImageTrackerObj = mlImageTrackerCons->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();
      info.GetReturnValue().Set(mlImageTrackerObj);
    }
  } else {
    Nan::ThrowError("MLContext::RequestImageTracking: invalid arguments");
  }
}

/* NAN_METHOD(MLContext::RequestDepthPopulation) {
  if (info[0]->IsBoolean()) {
    depthEnabled = TO_BOOL(info[0]);
  } else {
    Nan::ThrowError("MLContext::RequestDepthPopulation: invalid arguments");
  }
} */

/* bool cameraConnected = false;
NAN_METHOD(MLContext::RequestCamera) {
  if (info[0]->IsFunction()) {
    if (!cameraConnected) {
      MLResult result = MLCameraConnect();

      if (result == MLResult_Ok) {
        cameraConnected = true;

        std::thread([]() -> void {
          for (;;) {
            uv_sem_wait(&cameraConvertSem);

            cameraRequestSize = SjpegCompress(cameraRequestRgb, CAMERA_SIZE[0], CAMERA_SIZE[1], 50.0f, &cameraRequestJpeg);

            uv_async_send(&cameraConvertAsync);
          }
        }).detach();
      } else {
        ML_LOG(Error, "%s: Failed to connect camera: %x", application_name, result);
        Nan::ThrowError("failed to connect camera");
        return;
      }
    }

    Local<Function> cbFn = Local<Function>::Cast(info[0]);
    {
      // std::unique_lock<std::mutex> lock(cameraRequestsMutex);

      uv_loop_t *loop = windowsystembase::GetEventLoop();
      CameraRequest *cameraRequest = new CameraRequest(loop, cbFn);
      cameraRequests.push_back(cameraRequest);
    }
  } else {
    Nan::ThrowError("MLContext::RequestCamera: invalid arguments");
    return;
  }
}

NAN_METHOD(MLContext::CancelCamera) {
  if (info[0]->IsFunction()) {
    Local<Function> cbFn = Local<Function>::Cast(info[0]);

    {
      // std::unique_lock<std::mutex> lock(cameraRequestsMutex);

      cameraRequests.erase(std::remove_if(cameraRequests.begin(), cameraRequests.end(), [&](CameraRequest *c) -> bool {
        Local<Function> localCbFn = Nan::New(c->cbFn);
        if (localCbFn->StrictEquals(cbFn)) {
          delete c;
          return true;
        } else {
          return false;
        }
      }), cameraRequests.end());
    }
  } else {
    Nan::ThrowError("MLContext::CancelCamera: invalid arguments");
  }
} */

NAN_METHOD(MLContext::Update) {
  MLContext *mlContext = ObjectWrap::Unwrap<MLContext>(info.This());

  MLSnapshot *snapshot = nullptr;

  // requests

  // XXX
  if (imageTrackers.size() > 0) {
    if (!snapshot) {
      if (MLPerceptionGetSnapshot(&snapshot) != MLResult_Ok) {
        ML_LOG(Error, "%s: ML failed to get image tracking snapshot!", application_name);
      }
    }
    std::for_each(imageTrackers.begin(), imageTrackers.end(), [&](MLImageTracker *i) {
      i->Update(snapshot);
    });
  }

  if (snapshot) {
    if (MLPerceptionReleaseSnapshot(snapshot) != MLResult_Ok) {
      ML_LOG(Error, "%s: ML failed to release eye snapshot!", application_name);
    }
  }
}

/* NAN_METHOD(MLContext::Poll) {
  std::for_each(polls.begin(), polls.end(), [&](MLPoll *poll) {
    poll->cb();
    delete poll;
  });
  polls.clear();
} */

void MLContext::TickFloor() {
  if (floorRequestPending) {
    MLResult result = MLPlanesQueryGetResultsWithBoundaries(floorTracker, floorRequestHandle, floorResults, &numFloorResults);
    if (result == MLResult_Ok) {
      for (uint32_t i = 0; i < numFloorResults; i++) {
        const MLPlane &plane = floorResults[i];
        const MLVec3f &normal = applyVectorQuaternion(MLVec3f{0, 0, 1}, plane.rotation);
        if (normal.y > 0) {
          const float floorSizeSq = plane.width*plane.width + plane.height*plane.height;

          if (floorSizeSq > largestFloorSizeSq) {
            largestFloorY = plane.position.y;
            largestFloorSizeSq = floorSizeSq;
          }
        }
      }

      floorRequestPending = false;
    } else if (result == MLResult_Pending) {
      // nothing
    } else {
      ML_LOG(Error, "%s: Floor request failed! %x", application_name, result);

      floorRequestPending = false;
    }
  }

  if (!floorRequestPending) {
    {
      // std::unique_lock<std::mutex> lock(mlContext->positionMutex);

      floorRequest.bounds_center = this->position;
      floorRequest.bounds_rotation = this->rotation;
      // floorRequest.bounds_rotation = {0, 0, 0, 1};
    }
    floorRequest.bounds_extents.x = planeRange;
    floorRequest.bounds_extents.y = planeRange;
    floorRequest.bounds_extents.z = planeRange;

    floorRequest.flags = MLPlanesQueryFlag_Horizontal | MLPlanesQueryFlag_Semantic_Floor;
    // floorRequest.min_hole_length = 0.5;
    floorRequest.min_plane_area = 0.25;
    floorRequest.max_results = MAX_NUM_PLANES;

    MLResult result = MLPlanesQueryBegin(floorTracker, &floorRequest, &floorRequestHandle);
    if (result == MLResult_Ok) {
      floorRequestPending = true;
    } else {
      ML_LOG(Error, "%s: Floor request failed! %x", application_name, result);
    }
  }
}

MLVec3f MLContext::OffsetFloor(const MLVec3f &position) {
  return MLVec3f{position.x, position.y - largestFloorY, position.z};
}

}

Local<Object> makeMl() {
  Nan::EscapableHandleScope scope;
  return scope.Escape(ml::MLContext::Initialize(Isolate::GetCurrent()));
}

#endif
