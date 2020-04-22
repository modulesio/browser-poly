#ifndef _WAVE_SHAPER_NODE_H_
#define _WAVE_SHAPER_NODE_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <AudioNode.h>
#include <AudioParam.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {

class WaveShaperNode : public AudioNode {
public:
  static Local<Object> Initialize(Isolate *isolate);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);
  static NAN_GETTER(CurveGetter);
  static NAN_SETTER(CurveSetter);
  // these are not supported by LabSound, but present to actively notify as such
  static NAN_GETTER(OversampleGetter);
  static NAN_SETTER(OversampleSetter);

  WaveShaperNode();
  ~WaveShaperNode();

protected:
};

}

#endif
