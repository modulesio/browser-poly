#include <ChannelSplitterNode.h>
#include <AudioContext.h>

namespace webaudio {

ChannelSplitterNode::ChannelSplitterNode(uint32_t numberOfOutputs) {}

ChannelSplitterNode::~ChannelSplitterNode() {}

Local<Object> ChannelSplitterNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("ChannelSplitterNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  ChannelSplitterNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

void ChannelSplitterNode::InitializePrototype(Local<ObjectTemplate> proto) {
	// nothing
}

NAN_METHOD(ChannelSplitterNode::New) {
	// Nan::HandleScope scope;

	if (info[0]->IsNumber() &&
		info[1]->IsObject() && JS_OBJ(JS_OBJ(info[1])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))
		) {
		Local<Object> audioContextObj = Local<Object>::Cast(info[1]);
		AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

		uint32_t numberOfOutputs = TO_UINT32(info[0]);

		ChannelSplitterNode *channelSplitterNode = new ChannelSplitterNode(numberOfOutputs);
		Local<Object> channelSplitterNodeObj = info.This();
		channelSplitterNode->Wrap(channelSplitterNodeObj);

		channelSplitterNode->context.Reset(audioContextObj);
		channelSplitterNode->audioNode = make_shared<lab::ChannelSplitterNode>(numberOfOutputs);

		info.GetReturnValue().Set(channelSplitterNodeObj);
	}
	else {
		Nan::ThrowError("invalid arguments");
	}
 }
}
