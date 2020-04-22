#include <ConvolverNode.h>
#include <AudioBuffer.h>
#include <AudioContext.h>

namespace webaudio {

ConvolverNode::ConvolverNode() {}

ConvolverNode::~ConvolverNode() {}

Local<Object> ConvolverNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("ConvolverNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  ConvolverNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

void ConvolverNode::InitializePrototype(Local<ObjectTemplate> proto) {
  Nan::SetAccessor(proto, JS_STR("buffer"), BufferGetter, BufferSetter);
  Nan::SetAccessor(proto, JS_STR("normalize"), NormalizeGetter, NormalizeSetter);
}

NAN_METHOD(ConvolverNode::New) {
	// Nan::HandleScope scope;

	if (info[0]->IsObject() && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))) {
		Local<Object> audioContextObj = Local<Object>::Cast(info[0]);

		ConvolverNode *convolverNode = new ConvolverNode();
		Local<Object> convolverNodeObj = info.This();
		convolverNode->Wrap(convolverNodeObj);

		convolverNode->context.Reset(audioContextObj);
		convolverNode->audioNode = make_shared<lab::ConvolverNode>();

		info.GetReturnValue().Set(convolverNodeObj);
	}
	else {
		Nan::ThrowError("invalid arguments");
	}
}

NAN_GETTER(ConvolverNode::BufferGetter) {
//	Nan::HandleScope scope;

	ConvolverNode *convolverNode = ObjectWrap::Unwrap<ConvolverNode>(info.This());
	if (!convolverNode->audioBuffer.IsEmpty()) {
		info.GetReturnValue().Set(Nan::New(convolverNode->audioBuffer));
	}
	else {
		info.GetReturnValue().Set(Nan::Null());
	}
}

NAN_SETTER(ConvolverNode::BufferSetter) {
//  Nan::HandleScope scope;

  if (value->IsObject()) {
    Local<Value> constructorName = JS_OBJ(JS_OBJ(value)->Get(JS_STR("constructor")))->Get(JS_STR("name"));

    if (constructorName->StrictEquals(JS_STR("AudioBuffer"))) {
		ConvolverNode *convolverNode = ObjectWrap::Unwrap<ConvolverNode>(info.This());
		shared_ptr<lab::ConvolverNode> labConvolverNode = *(shared_ptr<lab::ConvolverNode> *)(&convolverNode->audioNode);

		Local<Object> audioBufferObj = Local<Object>::Cast(value);
		AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(audioBufferObj);
		shared_ptr<lab::AudioBus> audioBus = audioBuffer->audioBus;

		labConvolverNode->setImpulse(audioBus);
		convolverNode->audioBuffer.Reset(audioBufferObj);

    } else {
      Nan::ThrowError("ConvolverNode::BufferSetter: invalid arguments");
    }
  } else {
    Nan::ThrowError("ConvolverNode::BufferSetter: invalid arguments");
  }
}

NAN_GETTER(ConvolverNode::NormalizeGetter) {
  //Nan::HandleScope scope;

  ConvolverNode *convolverNode = ObjectWrap::Unwrap<ConvolverNode>(info.This());
  shared_ptr<lab::ConvolverNode> labConvolverNode = *(shared_ptr<lab::ConvolverNode> *)(&convolverNode->audioNode);

  bool result = labConvolverNode->normalize();
 
  info.GetReturnValue().Set(JS_BOOL(result));
}

NAN_SETTER(ConvolverNode::NormalizeSetter) {
  //Nan::HandleScope scope;

	if (value->IsBoolean()) {
		ConvolverNode *convolverNode = ObjectWrap::Unwrap<ConvolverNode>(info.This());
	    shared_ptr<lab::ConvolverNode> labConvolverNode = *(shared_ptr<lab::ConvolverNode> *)(&convolverNode->audioNode);

		bool normalize = TO_BOOL(value);
		labConvolverNode->setNormalize(normalize);
	}
	else {
		Nan::ThrowError("ConvolverNode::Normalize: invalid arguments");
	}
}

}
