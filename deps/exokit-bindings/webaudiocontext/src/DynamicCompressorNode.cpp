#include <DynamicsCompressorNode.h>
#include <AudioContext.h>

namespace webaudio {

DynamicsCompressorNode::DynamicsCompressorNode() {}

DynamicsCompressorNode::~DynamicsCompressorNode() {}

Local<Object> DynamicsCompressorNode::Initialize(Isolate *isolate, Local<Value> audioParamCons) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("DynamicsCompressorNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  DynamicsCompressorNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  ctorFn->Set(JS_STR("AudioParam"), audioParamCons);

  return scope.Escape(ctorFn);
}

void DynamicsCompressorNode::InitializePrototype(Local<ObjectTemplate> proto) {
	// nothing
}

NAN_METHOD(DynamicsCompressorNode::New) {
	// Nan::HandleScope scope;

	if (info[0]->IsObject() && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))) {
		Local<Object> audioContextObj = Local<Object>::Cast(info[0]);

		DynamicsCompressorNode *dynamicsCompressorNode = new DynamicsCompressorNode();
		Local<Object> dynamicsCompressorNodeObj = info.This();
		dynamicsCompressorNode->Wrap(dynamicsCompressorNodeObj);

		AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
		shared_ptr<lab::DynamicsCompressorNode> labDynamicsCompressorNode = make_shared<lab::DynamicsCompressorNode>();

		dynamicsCompressorNode->context.Reset(audioContextObj);
		dynamicsCompressorNode->audioNode = labDynamicsCompressorNode;

		Local<Function> audioParamConstructor = Local<Function>::Cast(JS_OBJ(dynamicsCompressorNodeObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioParam")));
		Local<Value> args[] = {
		  audioContextObj,
		};

		Local<Object> thresholdAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
		AudioParam *thresholdAudioParam = ObjectWrap::Unwrap<AudioParam>(thresholdAudioParamObj);
		thresholdAudioParam->audioParam = (*(shared_ptr<lab::DynamicsCompressorNode> *)(&dynamicsCompressorNode->audioNode))->threshold();
		dynamicsCompressorNodeObj->Set(JS_STR("threshold"), thresholdAudioParamObj);

		Local<Object> kneeAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
		AudioParam *kneeAudioParam = ObjectWrap::Unwrap<AudioParam>(kneeAudioParamObj);
		kneeAudioParam->audioParam = (*(shared_ptr<lab::DynamicsCompressorNode> *)(&dynamicsCompressorNode->audioNode))->threshold();
		dynamicsCompressorNodeObj->Set(JS_STR("knee"), kneeAudioParamObj);

		Local<Object> ratioAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
		AudioParam *ratioAudioParam = ObjectWrap::Unwrap<AudioParam>(ratioAudioParamObj);
		ratioAudioParam->audioParam = (*(shared_ptr<lab::DynamicsCompressorNode> *)(&dynamicsCompressorNode->audioNode))->threshold();
		dynamicsCompressorNodeObj->Set(JS_STR("ratio"), ratioAudioParamObj);

		Local<Object> reductionAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
		AudioParam *reductionAudioParam = ObjectWrap::Unwrap<AudioParam>(reductionAudioParamObj);
		reductionAudioParam->audioParam = (*(shared_ptr<lab::DynamicsCompressorNode> *)(&dynamicsCompressorNode->audioNode))->threshold();
		dynamicsCompressorNodeObj->Set(JS_STR("reduction"), reductionAudioParamObj);

		Local<Object> attackAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
		AudioParam *attackAudioParam = ObjectWrap::Unwrap<AudioParam>(attackAudioParamObj);
		attackAudioParam->audioParam = (*(shared_ptr<lab::DynamicsCompressorNode> *)(&dynamicsCompressorNode->audioNode))->threshold();
		dynamicsCompressorNodeObj->Set(JS_STR("attack"), attackAudioParamObj);

		Local<Object> releaseAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
		AudioParam *releaseAudioParam = ObjectWrap::Unwrap<AudioParam>(releaseAudioParamObj);
		releaseAudioParam->audioParam = (*(shared_ptr<lab::DynamicsCompressorNode> *)(&dynamicsCompressorNode->audioNode))->threshold();
		dynamicsCompressorNodeObj->Set(JS_STR("release"), releaseAudioParamObj);

		info.GetReturnValue().Set(dynamicsCompressorNodeObj);
	}
	else {
		Nan::ThrowError("invalid arguments");
	}
}

}
