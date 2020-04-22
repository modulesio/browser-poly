#include <DelayNode.h>
#include <AudioContext.h>

namespace webaudio {

DelayNode::DelayNode() {}

DelayNode::~DelayNode() {}

Local<Object> DelayNode::Initialize(Isolate *isolate, Local<Value> audioParamCons) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("DelayNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  DelayNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  ctorFn->Set(JS_STR("AudioParam"), audioParamCons);

  return scope.Escape(ctorFn);
}

void DelayNode::InitializePrototype(Local<ObjectTemplate> proto) {
	// nothing
}

NAN_METHOD(DelayNode::New) {
	// Nan::HandleScope scope;

	if (info[0]->IsNumber() && info[1]->IsNumber() &&
		info[2]->IsObject() && JS_OBJ(JS_OBJ(info[2])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))
	) {

		Local<Object> audioContextObj = Local<Object>::Cast(info[2]);
		AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

		double maxDelayTime = TO_DOUBLE(info[0]);
		float sampleRate = TO_FLOAT(info[1]);

		DelayNode *delayNode = new DelayNode();
		Local<Object> delayNodeObj = info.This();
		delayNode->Wrap(delayNodeObj);

		delayNode->context.Reset(audioContextObj);
		delayNode->audioNode = make_shared<lab::DelayNode>(sampleRate, maxDelayTime);

		Local<Function> audioParamConstructor = Local<Function>::Cast(JS_OBJ(delayNodeObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioParam")));
		Local<Value> args[] = {
		  audioContextObj,
		};

		Local<Object> delayTimeAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
		AudioParam *delayTimeAudioParam = ObjectWrap::Unwrap<AudioParam>(delayTimeAudioParamObj);
		delayTimeAudioParam->audioParam = (*(shared_ptr<lab::DelayNode> *)(&delayNode->audioNode))->delayTime();
		delayNodeObj->Set(JS_STR("delayTime"), delayTimeAudioParamObj);

		info.GetReturnValue().Set(delayNodeObj);
	}
	else {
		Nan::ThrowError("invalid arguments");
	}
 }
}
