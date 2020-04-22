#include <BiquadFilterNode.h>

namespace webaudio {

enum ModelType {
  ModelLowPass = 0,
  ModelHighPass = 1,
  ModelBandPass = 2,
  ModelLowShelf = 3,
  ModelHighShelf = 4,
  ModelPeaking = 5,
  ModelNotch = 6,
  ModelAllPass = 7
};

BiquadFilterNode::BiquadFilterNode() {}

BiquadFilterNode::~BiquadFilterNode() {}

Local<Object> BiquadFilterNode::Initialize(Isolate *isolate, Local<Value> audioParamCons) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("BiquadFilterNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  BiquadFilterNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  ctorFn->Set(JS_STR("AudioParam"), audioParamCons);

  return scope.Escape(ctorFn);
}

void BiquadFilterNode::InitializePrototype(Local<ObjectTemplate> proto) {
  Nan::SetAccessor(proto, JS_STR("type"), TypeGetter, TypeSetter);
  Nan::SetMethod(proto, "getFrequencyResponse", GetFrequencyResponse);
}

NAN_METHOD(BiquadFilterNode::New) {
  // Nan::HandleScope scope;

  if (info[0]->IsObject() && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))) {
    Local<Object> audioContextObj = Local<Object>::Cast(info[0]);

	BiquadFilterNode *biquadFilterNode = new BiquadFilterNode();
    Local<Object> biquadFilterNodeObj = info.This();
	biquadFilterNode->Wrap(biquadFilterNodeObj);

    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
    shared_ptr<lab::BiquadFilterNode> labBiquadFilterNode = make_shared<lab::BiquadFilterNode>();

	biquadFilterNode->context.Reset(audioContextObj);
	biquadFilterNode->audioNode = labBiquadFilterNode;

	Local<Function> audioParamConstructor = Local<Function>::Cast(JS_OBJ(biquadFilterNodeObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioParam")));
	Local<Value> args[] = {
	  audioContextObj,
	};

	Local<Object> frequencyAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
	AudioParam *frequencyAudioParam = ObjectWrap::Unwrap<AudioParam>(frequencyAudioParamObj);
	frequencyAudioParam->audioParam = (*(shared_ptr<lab::BiquadFilterNode> *)(&biquadFilterNode->audioNode))->frequency();
	biquadFilterNodeObj->Set(JS_STR("frequency"), frequencyAudioParamObj);

	Local<Object> detuneAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
	AudioParam *detuneAudioParam = ObjectWrap::Unwrap<AudioParam>(detuneAudioParamObj);
	detuneAudioParam->audioParam = (*(shared_ptr<lab::BiquadFilterNode> *)(&biquadFilterNode->audioNode))->detune();
	biquadFilterNodeObj->Set(JS_STR("detune"), detuneAudioParamObj);

	Local<Object> QAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
	AudioParam *QAudioParam = ObjectWrap::Unwrap<AudioParam>(QAudioParamObj);
	QAudioParam->audioParam = (*(shared_ptr<lab::BiquadFilterNode> *)(&biquadFilterNode->audioNode))->q();
	biquadFilterNodeObj->Set(JS_STR("Q"), QAudioParamObj);

	Local<Object> gainAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args) / sizeof(args[0]), args).ToLocalChecked();
	AudioParam *gainAudioParam = ObjectWrap::Unwrap<AudioParam>(gainAudioParamObj);
	gainAudioParam->audioParam = (*(shared_ptr<lab::BiquadFilterNode> *)(&biquadFilterNode->audioNode))->gain();
	biquadFilterNodeObj->Set(JS_STR("gain"), gainAudioParamObj);

	info.GetReturnValue().Set(biquadFilterNodeObj);
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

// https://developer.mozilla.org/en-US/docs/Web/API/BiquadFilterNode/getFrequencyResponse
NAN_METHOD(BiquadFilterNode::GetFrequencyResponse) {
	// Nan::HandleScope scope;

	if (info[0]->IsFloat32Array() && info[1]->IsFloat32Array() && info[2]->IsFloat32Array()) {
		BiquadFilterNode *biquadFilterNode = ObjectWrap::Unwrap<BiquadFilterNode>(info.This());
		shared_ptr<lab::BiquadFilterNode> labBiquadFilterNode = *(shared_ptr<lab::BiquadFilterNode> *)(&biquadFilterNode->audioNode);
		Local<Object> audioContextObj = Nan::New(biquadFilterNode->context);
		AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

		Local<Float32Array> frequencyHzArg   = Local<Float32Array>::Cast(info[0]);
		Local<Float32Array> magResponseArg   = Local<Float32Array>::Cast(info[1]);
		Local<Float32Array> phaseResponseArg = Local<Float32Array>::Cast(info[2]);

		vector<float> freqBuffer (frequencyHzArg->Length());
		vector<float> magRBuffer (magResponseArg->Length());
		vector<float> phaseBuffer(phaseResponseArg->Length());

		{
		   lab::ContextRenderLock lock(audioContext->audioContext.get(), "BiquadFilterNode::GetFrequencyResponse");
	       labBiquadFilterNode->getFrequencyResponse(lock, freqBuffer, magRBuffer, phaseBuffer);
		}
		Local<ArrayBuffer> freqArrayBuffer = frequencyHzArg->Buffer();
		memcpy((unsigned char *)freqArrayBuffer ->GetContents().Data() + frequencyHzArg ->ByteOffset(), freqBuffer .data(), frequencyHzArg  ->Length());

		Local<ArrayBuffer> magRArrayBuffer = magResponseArg->Buffer();
		memcpy((unsigned char *)magRArrayBuffer ->GetContents().Data() + magResponseArg ->ByteOffset(), magRBuffer .data(), magResponseArg  ->Length());

		Local<ArrayBuffer> phaseArrayBuffer = phaseResponseArg->Buffer();
		memcpy((unsigned char *)phaseArrayBuffer->GetContents().Data() + phaseResponseArg->ByteOffset(), phaseBuffer.data(), phaseResponseArg->Length());

	}
	else {
		Nan::ThrowError("BiquadFilter:GetFrequencyResponse: invalid arguments");
	}
}

NAN_GETTER(BiquadFilterNode::TypeGetter) {
 // Nan::HandleScope scope;

  BiquadFilterNode *biquadFilterNode = ObjectWrap::Unwrap<BiquadFilterNode>(info.This());
  shared_ptr<lab::BiquadFilterNode> labBiquadFilterNode = *(shared_ptr<lab::BiquadFilterNode> *)(&biquadFilterNode->audioNode);

  Local<String> result;
  ModelType typeModel = (ModelType)labBiquadFilterNode->type();
  switch (typeModel) {
    case ModelType::ModelLowPass: {
      result = Nan::New<String>("lowpass").ToLocalChecked();
      break;
    }
    case ModelType::ModelHighPass: {
      result = Nan::New<String>("highpass").ToLocalChecked();
      break;
    }
	case ModelType::ModelBandPass: {
		result = Nan::New<String>("bandpass").ToLocalChecked();
		break;
	}
	case ModelType::ModelLowShelf: {
		result = Nan::New<String>("lowshelf").ToLocalChecked();
		break;
	}
	case ModelType::ModelHighShelf: {
		result = Nan::New<String>("highself").ToLocalChecked();
		break;
	}
	case ModelType::ModelPeaking: {
		result = Nan::New<String>("peaking").ToLocalChecked();
		break;
	}
	case ModelType::ModelNotch: {
		result = Nan::New<String>("notch").ToLocalChecked();
		break;
	}
	case ModelType::ModelAllPass: {
		result = Nan::New<String>("allpass").ToLocalChecked();
		break;
	}
  }

  info.GetReturnValue().Set(result);
}

NAN_SETTER(BiquadFilterNode::TypeSetter) {
///  Nan::HandleScope scope;

  if (value->IsString()) {
	BiquadFilterNode *biquadFilterNode = ObjectWrap::Unwrap<BiquadFilterNode>(info.This());
    shared_ptr<lab::BiquadFilterNode> labBiquadFilterNode = *(shared_ptr<lab::BiquadFilterNode> *)(&biquadFilterNode->audioNode);

    Nan::Utf8String valueUtf8(Local<String>::Cast(value));
    string valueString(*valueUtf8, valueUtf8.length());

    ModelType typeModel;
    if (valueString == "lowpass") {
	  typeModel = ModelType::ModelLowPass;
    } else if (valueString == "highpass") {
		typeModel = ModelType::ModelHighPass;
    } else if (valueString == "bandpass") {
		typeModel = ModelType::ModelBandPass;
	} else if (valueString == "lowshelf") {
		typeModel = ModelType::ModelLowShelf;
	} else if (valueString == "highself") {
		typeModel = ModelType::ModelHighShelf;
	} else if (valueString == "peaking") {
		typeModel = ModelType::ModelPeaking;
	} else if (valueString == "notch") {
		typeModel = ModelType::ModelNotch;
	} else if (valueString == "allpass") {
		typeModel = ModelType::ModelAllPass;
	} else {
		Nan::ThrowError("value: not a valid type");
    }

	labBiquadFilterNode->setType(typeModel);
  } else {
    Nan::ThrowError("value: invalid arguments");
  }
}
}
