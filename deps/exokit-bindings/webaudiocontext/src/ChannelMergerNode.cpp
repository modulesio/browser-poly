#include <ChannelMergerNode.h>
#include <AudioContext.h>

namespace webaudio {

ChannelMergerNode::ChannelMergerNode(uint32_t numberOfInputs) {}

ChannelMergerNode::~ChannelMergerNode() {}

Local<Object> ChannelMergerNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("ChannelMergerNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  ChannelMergerNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

void ChannelMergerNode::InitializePrototype(Local<ObjectTemplate> proto) {
	// nothing
}

NAN_METHOD(ChannelMergerNode::New) {
	// Nan::HandleScope scope;

	if (info[0]->IsNumber() &&
		info[1]->IsObject() && JS_OBJ(JS_OBJ(info[1])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))
	) {
		Local<Object> audioContextObj = Local<Object>::Cast(info[1]);
		AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

		uint32_t numberOfInputs = TO_UINT32(info[0]);

		ChannelMergerNode *channelMergerNode = new ChannelMergerNode(numberOfInputs);
		Local<Object> channelMergerNodeObj = info.This();
		channelMergerNode->Wrap(channelMergerNodeObj);

		channelMergerNode->context.Reset(audioContextObj);
		channelMergerNode->audioNode = make_shared<lab::ChannelMergerNode>(numberOfInputs);

		info.GetReturnValue().Set(channelMergerNodeObj);
	}
	else {
		Nan::ThrowError("invalid arguments");
	}
 }
}
