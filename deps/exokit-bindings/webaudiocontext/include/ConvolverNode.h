#ifndef _CONVOLVER_NODE_H_
#define _CONVOLVER_NODE_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include <string>
#include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <AudioNode.h>
#include <AudioBuffer.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {

class ConvolverNode : public AudioNode {
public:
  static Local<Object> Initialize(Isolate *isolate);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);
  static NAN_GETTER(BufferGetter);
  static NAN_SETTER(BufferSetter);
  static NAN_GETTER(NormalizeGetter);
  static NAN_SETTER(NormalizeSetter);

  ConvolverNode();
  ~ConvolverNode();

protected:
	Nan::Persistent<Object> audioBuffer;
};

}

#endif
