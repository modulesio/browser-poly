#ifndef _BIQUAD_FILTER_NODE_H_
#define _BIQUAD_FILTER_NODE_H_

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

class BiquadFilterNode : public AudioNode {
public:
  static Local<Object> Initialize(Isolate *isolate, Local<Value> audioParamCons);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);
  static NAN_METHOD(GetFrequencyResponse);
  static NAN_GETTER(TypeGetter);
  static NAN_SETTER(TypeSetter);

  BiquadFilterNode();
  ~BiquadFilterNode();

protected:
};

}

#endif
