#ifndef _CHANNEL_MERGER_NODE_H_
#define _CHANNEL_MERGER_NODE_H_

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

class ChannelMergerNode : public AudioNode {
public:
  static Local<Object> Initialize(Isolate *isolate);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  static NAN_METHOD(New);

  ChannelMergerNode(uint32_t numberOfInputs);
  ~ChannelMergerNode();

protected:
};

}

#endif
