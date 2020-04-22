#include <WaveShaperNode.h>

namespace webaudio {

WaveShaperNode::WaveShaperNode() {}

WaveShaperNode::~WaveShaperNode() {}

Local<Object> WaveShaperNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("WaveShaperNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  WaveShaperNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

void WaveShaperNode::InitializePrototype(Local<ObjectTemplate> proto) {
	Nan::SetAccessor(proto, JS_STR("type"), CurveGetter, CurveSetter);
	Nan::SetAccessor(proto, JS_STR("type"), OversampleGetter, OversampleSetter);
}

NAN_METHOD(WaveShaperNode::New) {
  // Nan::HandleScope scope;

  if (info[0]->IsObject() && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))) {
    Local<Object> audioContextObj = Local<Object>::Cast(info[0]);

	WaveShaperNode *waveShaperNode = new WaveShaperNode();
    Local<Object> waveShaperNodeObj = info.This();
	waveShaperNode->Wrap(waveShaperNodeObj);

    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
    shared_ptr<lab::WaveShaperNode> labWaveShaperNode = make_shared<lab::WaveShaperNode>();

	waveShaperNode->context.Reset(audioContextObj);
	waveShaperNode->audioNode = labWaveShaperNode;

	info.GetReturnValue().Set(waveShaperNodeObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

NAN_GETTER(WaveShaperNode::OversampleGetter) {
  Local<String> result = Nan::New<String>("none").ToLocalChecked();
  info.GetReturnValue().Set(result);
}

NAN_SETTER(WaveShaperNode::OversampleSetter) {
  Nan::ThrowError("WaveShaperNode::OversampleSetter: not support by underlying library");
}

NAN_GETTER(WaveShaperNode::CurveGetter) {
  // Nan::HandleScope scope;

  WaveShaperNode *waveShaperNode = ObjectWrap::Unwrap<WaveShaperNode>(info.This());
  shared_ptr<lab::WaveShaperNode> labWaveShaperNode = *(shared_ptr<lab::WaveShaperNode> *)(&waveShaperNode->audioNode);

  vector<float> buffer = labWaveShaperNode->curve();
  size_t len = buffer.capacity();

  Local<ArrayBuffer> arrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), len * sizeof(float));
  memcpy((unsigned char *)arrayBuffer->GetContents().Data(), buffer.data(), len);

  Local<Float32Array> result = Float32Array::New(arrayBuffer, 0, len);
  info.GetReturnValue().Set(result);
}

NAN_SETTER(WaveShaperNode::CurveSetter) {
 // Nan::HandleScope scope;

  if (value->IsFloat32Array()) {
	WaveShaperNode *waveShaperNode = ObjectWrap::Unwrap<WaveShaperNode>(info.This());
	shared_ptr<lab::WaveShaperNode> labWaveShaperNode = *(shared_ptr<lab::WaveShaperNode> *)(&waveShaperNode->audioNode);

	Local<Float32Array> arg = Local<Float32Array>::Cast(value);
	Local<ArrayBuffer> arrayBuffer = arg->Buffer();

	vector<float> buffer(arg->Length());
	memcpy(buffer.data(), (unsigned char *)arrayBuffer->GetContents().Data(), arg->ByteLength());

	labWaveShaperNode->setCurve(buffer);

  } else {
    Nan::ThrowError("WaveShaperNode::CurveSetter: invalid arguments");
  }
}
}
