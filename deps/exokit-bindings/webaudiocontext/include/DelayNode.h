#ifndef _DELAY_NODE_H_
#define _DELAY_NODE_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <AudioNode.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {

class DelayNode : public AudioNode {
public:
  static Local<Object> Initialize(Isolate *isolate, Local<Value> audioParamCons);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);

  DelayNode();
  ~DelayNode();

protected:
};

}

#endif
