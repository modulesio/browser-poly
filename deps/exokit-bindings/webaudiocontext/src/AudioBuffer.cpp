#include <AudioBuffer.h>
#include <AudioContext.h>

namespace webaudio {

AudioBuffer::AudioBuffer(uint32_t sampleRate, Local<Array> buffers) : sampleRate(sampleRate), buffers(buffers) {}
AudioBuffer::~AudioBuffer() {}
Local<Object> AudioBuffer::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioBuffer"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetAccessor(proto, JS_STR("sampleRate"), SampleRate);
  Nan::SetAccessor(proto, JS_STR("length"), Length);
  Nan::SetAccessor(proto, JS_STR("duration"), Duration);
  Nan::SetAccessor(proto, JS_STR("numberOfChannels"), NumberOfChannels);
  Nan::SetMethod(proto, "getChannelData", GetChannelData);
  Nan::SetMethod(proto, "copyFromChannel", CopyFromChannel);
  Nan::SetMethod(proto, "copyToChannel", CopyToChannel);
  Nan::SetMethod(proto, "load", Load);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}
NAN_METHOD(AudioBuffer::New) {
  // Nan::HandleScope scope;

  if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber()) {
    uint32_t numOfChannels = TO_UINT32(info[0]);
    uint32_t length = TO_UINT32(info[1]);
    uint32_t sampleRate = TO_UINT32(info[2]);
    Local<Array> buffers;
    if (info[3]->IsArray()) {
      buffers = Local<Array>::Cast(info[3]);
    } else {
      buffers = Nan::New<Array>(numOfChannels);
      for (size_t i = 0; i < numOfChannels; i++) {
        Local<ArrayBuffer> arrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), length * sizeof(float));
        Local<Float32Array> float32Array = Float32Array::New(arrayBuffer, 0, length);
        buffers->Set(i, float32Array);
      }
    }

    AudioBuffer *audioBuffer = new AudioBuffer(sampleRate, buffers);
    Local<Object> audioBufferObj = info.This();
    audioBuffer->Wrap(audioBufferObj);
  } else if (info[0]->IsNumber()) {
    uint32_t sampleRate = TO_UINT32(info[0]);
    Local<Array> buffers = Nan::New<Array>();

    AudioBuffer *audioBuffer = new AudioBuffer(sampleRate, buffers);
    Local<Object> audioBufferObj = info.This();
    audioBuffer->Wrap(audioBufferObj);
  } else {
    Nan::ThrowError("AudioBuffer:New: invalid arguments");
  }
}
NAN_GETTER(AudioBuffer::SampleRate) {
  // Nan::HandleScope scope;

  AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
  info.GetReturnValue().Set(JS_INT(audioBuffer->sampleRate));
}
NAN_GETTER(AudioBuffer::Length) {
  // Nan::HandleScope scope;

  AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
  Local<Array> buffers = Nan::New(audioBuffer->buffers);
  if (buffers->Length() > 0) {
    Local<Float32Array> firstBuffer = Local<Float32Array>::Cast(buffers->Get(0));
    uint32_t numFrames = firstBuffer->Length();

    info.GetReturnValue().Set(JS_INT(numFrames));
  } else {
    info.GetReturnValue().Set(JS_INT(0));
  }
}
NAN_GETTER(AudioBuffer::Duration) {
  // Nan::HandleScope scope;

  AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
  Local<Array> buffers = Nan::New(audioBuffer->buffers);
  if (buffers->Length() > 0) {
    Local<Float32Array> firstBuffer = Local<Float32Array>::Cast(buffers->Get(0));
    double numFrames = firstBuffer->Length();
    double sampleRate = audioBuffer->sampleRate;
    double duration = numFrames / sampleRate;

    info.GetReturnValue().Set(JS_NUM(duration));
  } else {
    info.GetReturnValue().Set(JS_NUM(0));
  }
}
NAN_GETTER(AudioBuffer::NumberOfChannels) {
  // Nan::HandleScope scope;

  AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
  Local<Array> buffers = Nan::New(audioBuffer->buffers);

  info.GetReturnValue().Set(JS_INT(buffers->Length()));
}
NAN_METHOD(AudioBuffer::GetChannelData) {
  // Nan::HandleScope scope;

  if (info[0]->IsNumber()) {
    uint32_t channelIndex = TO_UINT32(info[0]);

    AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
    Local<Array> buffers = Nan::New(audioBuffer->buffers);
    Local<Value> channelData = buffers->Get(channelIndex);

    info.GetReturnValue().Set(channelData);
  } else {
    Nan::ThrowError("AudioBuffer:GetChannelData: invalid arguments");
  }
}
NAN_METHOD(AudioBuffer::CopyFromChannel) {
  // Nan::HandleScope scope;

  if (info[0]->IsFloat32Array() && info[1]->IsNumber()) {
    Local<Float32Array> destinationFloat32Array = Local<Float32Array>::Cast(info[0]);
    uint32_t channelIndex = TO_UINT32(info[1]);

    AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
    Local<Array> buffers = Nan::New(audioBuffer->buffers);
    Local<Value> channelData = buffers->Get(channelIndex);

    if (TO_BOOL(channelData)) {
      Local<Float32Array> channelDataFloat32Array = Local<Float32Array>::Cast(channelData);
      uint32_t offset = std::min<uint32_t>(info[2]->IsNumber() ? TO_UINT32(info[2]) : 0, channelDataFloat32Array->Length());
      size_t copyLength = std::min<size_t>(channelDataFloat32Array->Length() - offset, destinationFloat32Array->Length());
      for (size_t i = 0; i < copyLength; i++) {
        destinationFloat32Array->Set(i, channelDataFloat32Array->Get(offset + i));
      }
    } else {
      Nan::ThrowError("AudioBuffer:CopyFromChannel: invalid channel index");
    }
  } else {
    Nan::ThrowError("AudioBuffer:CopyFromChannel: invalid arguments");
  }
}
NAN_METHOD(AudioBuffer::CopyToChannel) {
  // Nan::HandleScope scope;

  if (info[0]->IsFloat32Array() && info[1]->IsNumber()) {
    Local<Float32Array> sourceFloat32Array = Local<Float32Array>::Cast(info[0]);
    uint32_t channelIndex = TO_UINT32(info[1]);

    AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());
    Local<Array> buffers = Nan::New(audioBuffer->buffers);
    Local<Value> channelData = buffers->Get(channelIndex);

    if (TO_BOOL(channelData)) {
      Local<Float32Array> channelDataFloat32Array = Local<Float32Array>::Cast(channelData);
      uint32_t offset = std::min<uint32_t>(info[2]->IsNumber() ? TO_UINT32(info[2]) : 0, channelDataFloat32Array->Length());
      size_t copyLength = std::min<size_t>(channelDataFloat32Array->Length() - offset, sourceFloat32Array->Length());
      for (size_t i = 0; i < copyLength; i++) {
        channelDataFloat32Array->Set(offset + i, sourceFloat32Array->Get(i));
      }
    } else {
      Nan::ThrowError("AudioBuffer:CopyToChannel: invalid channel index");
    }
  } else {
    Nan::ThrowError("AudioBuffer:CopyToChannel: invalid arguments");
  }
}
NAN_METHOD(AudioBuffer::Load) {
  if (info[1]->IsFunction()) {
    if (info[0]->IsArrayBuffer()) {
      AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());

      Local<ArrayBuffer> arrayBuffer = Local<ArrayBuffer>::Cast(info[0]);
      Local<Function> cbFn = Local<Function>::Cast(info[1]);

      audioBuffer->Load(arrayBuffer, 0, arrayBuffer->ByteLength(), cbFn);
    } else if (info[0]->IsTypedArray()) {
      AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(info.This());

      Local<ArrayBufferView> arrayBufferView = Local<ArrayBufferView>::Cast(info[0]);
      Local<ArrayBuffer> arrayBuffer = arrayBufferView->Buffer();
      Local<Function> cbFn = Local<Function>::Cast(info[1]);

      audioBuffer->Load(arrayBuffer, arrayBufferView->ByteOffset(), arrayBufferView->ByteLength(), cbFn);
    } else {
      Local<Object> asyncObject = Nan::New<Object>();
      AsyncResource asyncResource(Isolate::GetCurrent(), asyncObject, "AudioBuffer::Load");

      Local<Function> cbFn = Local<Function>::Cast(info[1]);
      Local<Value> argv[] = {
        JS_STR("invalid buffer"),
      };
      asyncResource.MakeCallback(cbFn, sizeof(argv)/sizeof(argv[0]), argv);
    }
  } else {
    Nan::ThrowError("invalid arguments");
  }
}

void AudioBuffer::Load(Local<ArrayBuffer> arrayBuffer, size_t byteOffset, size_t byteLength, Local<Function> cbFn) {
  if (this->cbFn.IsEmpty()) {
    this->cbFn.Reset(cbFn);
    this->error = "";

    WebAudioAsync *webAudioAsync = getWebAudioAsync();
    std::vector<unsigned char> buffer(byteLength);
    memcpy(buffer.data(), (unsigned char *)arrayBuffer->GetContents().Data() + byteOffset, byteLength);
    
    threadpool::ThreadPool *threadPool = threadpool::getWindowThreadPool();
    threadPool->queueWork([this, webAudioAsync, buffer{std::move(buffer)}]() mutable -> void {
      this->audioBus = lab::MakeBusFromMemory(buffer, false, &this->error);

      webAudioAsync->QueueOnMainThread(std::bind(ProcessLoadInMainThread, this));
    });
  } else {
    Local<String> arg0 = Nan::New<String>("already loading").ToLocalChecked();
    Local<Value> argv[] = {
      arg0,
    };
    cbFn->Call(Isolate::GetCurrent()->GetCurrentContext(), Nan::Null(), sizeof(argv)/sizeof(argv[0]), argv);
  }
}

void AudioBuffer::ProcessLoadInMainThread(AudioBuffer *audioBuffer) {
  Nan::HandleScope scope;
  
  uint32_t numChannels = audioBuffer->audioBus->numberOfChannels();
  uint32_t numFrames = audioBuffer->audioBus->channel(0)->length();
  Local<Array> buffers = Nan::New<Array>(numChannels);
  for (size_t i = 0; i < numChannels; i++) {
    lab::AudioChannel *audioChannel = audioBuffer->audioBus->channel(i);
    const float *source = audioChannel->data();

    Local<ArrayBuffer> sourceArrayBuffer = ArrayBuffer::New(Isolate::GetCurrent(), numFrames * sizeof(float));
    memcpy(sourceArrayBuffer->GetContents().Data(), source, sourceArrayBuffer->ByteLength());
    Local<Float32Array> sourceFloat32Array = Float32Array::New(sourceArrayBuffer, 0, numFrames);
    buffers->Set(i, sourceFloat32Array);
  }
  audioBuffer->buffers.Reset(buffers);

  Local<Object> asyncObject = Nan::New<Object>();
  AsyncResource asyncResource(Isolate::GetCurrent(), asyncObject, "AudioBuffer::ProcessLoadInMainThread");

  Local<Function> cbFn = Nan::New(audioBuffer->cbFn);
  Local<String> arg0 = Nan::New<String>(audioBuffer->error).ToLocalChecked();
  Local<Value> argv[] = {
    arg0,
  };
  asyncResource.MakeCallback(cbFn, sizeof(argv)/sizeof(argv[0]), argv);

  audioBuffer->cbFn.Reset();
  audioBuffer->audioBus.reset();
  audioBuffer->error = "";
}

AudioBufferSourceNode::AudioBufferSourceNode() {
  WebAudioAsync *webaudioAsync = getWebAudioAsync();
  audioNode.reset(new lab::FinishableSourceNode([this, webaudioAsync](lab::ContextRenderLock &r){
    webaudioAsync->QueueOnMainThread(r, std::bind(ProcessInMainThread, this));
  }));
}
AudioBufferSourceNode::~AudioBufferSourceNode() {}
Local<Object> AudioBufferSourceNode::Initialize(Isolate *isolate, Local<Value> audioParamCons) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioBufferSourceNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);
  AudioBufferSourceNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();
  
  ctorFn->Set(JS_STR("AudioParam"), audioParamCons);

  return scope.Escape(ctorFn);
}
void AudioBufferSourceNode::InitializePrototype(Local<ObjectTemplate> proto) {
  Nan::SetAccessor(proto, JS_STR("buffer"), BufferGetter, BufferSetter);
  Nan::SetAccessor(proto, JS_STR("onended"), OnEndedGetter, OnEndedSetter);
  Nan::SetMethod(proto, "start", Start);
  Nan::SetMethod(proto, "stop", Stop);
}
NAN_METHOD(AudioBufferSourceNode::New) {
  // Nan::HandleScope scope;

  if (info[0]->IsObject() && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioContext"))) {
    Local<Object> audioContextObj = Local<Object>::Cast(info[0]);
    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

    AudioBufferSourceNode *audioBufferSourceNode = new AudioBufferSourceNode();
    Local<Object> audioBufferSourceNodeObj = info.This();
    audioBufferSourceNode->Wrap(audioBufferSourceNodeObj);
    audioBufferSourceNode->context.Reset(audioContextObj);
    
    Local<Function> audioParamConstructor = Local<Function>::Cast(JS_OBJ(audioBufferSourceNodeObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioParam")));
    Local<Value> args[] = {
      audioContextObj,
    };
    Local<Object> playbackRateAudioParamObj = audioParamConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(args)/sizeof(args[0]), args).ToLocalChecked();
    AudioParam *playbackRateAudioParam = ObjectWrap::Unwrap<AudioParam>(playbackRateAudioParamObj);
    playbackRateAudioParam->audioParam = (*(shared_ptr<lab::FinishableSourceNode> *)(&audioBufferSourceNode->audioNode))->playbackRate();
    audioBufferSourceNodeObj->Set(JS_STR("playbackRate"), playbackRateAudioParamObj);

    info.GetReturnValue().Set(audioBufferSourceNodeObj);
  } else {
    Nan::ThrowError("AudioBufferSourceNode:New: invalid arguments");
  }
}
NAN_METHOD(AudioBufferSourceNode::Start) {
  // Nan::HandleScope scope;

  double when = info[0]->IsNumber() ? TO_DOUBLE(info[0]) : 0;

  AudioBufferSourceNode *audioBufferSourceNode = ObjectWrap::Unwrap<AudioBufferSourceNode>(info.This());
  ((lab::FinishableSourceNode *)audioBufferSourceNode->audioNode.get())->start(when);
}
NAN_METHOD(AudioBufferSourceNode::Stop) {
  // Nan::HandleScope scope;

	double when = info[0]->IsNumber() ? TO_DOUBLE(info[0]) : 0;

  AudioBufferSourceNode *audioBufferSourceNode = ObjectWrap::Unwrap<AudioBufferSourceNode>(info.This());
  ((lab::FinishableSourceNode *)audioBufferSourceNode->audioNode.get())->stop(when);
}
NAN_GETTER(AudioBufferSourceNode::BufferGetter) {
  // Nan::HandleScope scope;

  AudioBufferSourceNode *audioBufferSourceNode = ObjectWrap::Unwrap<AudioBufferSourceNode>(info.This());
  Local<Object> buffer = Nan::New(audioBufferSourceNode->buffer);
  info.GetReturnValue().Set(buffer);
}
NAN_SETTER(AudioBufferSourceNode::BufferSetter) {
  // Nan::HandleScope scope;

  AudioBufferSourceNode *audioBufferSourceNode = ObjectWrap::Unwrap<AudioBufferSourceNode>(info.This());
  Local<Object> audioContextObj = Nan::New(audioBufferSourceNode->context);
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
  lab::FinishableSourceNode *audioNode = (lab::FinishableSourceNode *)audioBufferSourceNode->audioNode.get();

  if (value->IsObject() && JS_OBJ(JS_OBJ(value)->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("AudioBuffer"))) {
    Local<Object> audioBufferObj = Local<Object>::Cast(value);
    audioBufferSourceNode->buffer.Reset(audioBufferObj);

    AudioBuffer *audioBuffer = ObjectWrap::Unwrap<AudioBuffer>(audioBufferObj);
    Local<Array> buffers = Nan::New(audioBuffer->buffers);
    size_t numChannels = buffers->Length();
    size_t numFrames = numChannels > 0 ? Local<Float32Array>::Cast(buffers->Get(0))->Length() : 0;

    unique_ptr<float *[]> frames(new float*[numChannels]);
    for (size_t i = 0; i < numChannels; i++) {
      Local<Float32Array> bufferFramesFloat32Array = Local<Float32Array>::Cast(buffers->Get(i));
      size_t numBufferFrames = bufferFramesFloat32Array->Length();
      Local<ArrayBuffer> bufferFramesArrayBuffer = bufferFramesFloat32Array->Buffer();
      frames[i] = (float *)((unsigned char *)bufferFramesArrayBuffer->GetContents().Data() + bufferFramesFloat32Array->ByteOffset());
    }

    shared_ptr<lab::AudioBus> audioBus(lab::MakeBusFromRawBuffer(audioContext->audioContext->sampleRate(), numChannels, numFrames, frames.get(), false).release());

    {
      lab::ContextRenderLock lock(audioContext->audioContext.get(), "AudioBufferSourceNode::buffer");
      audioNode->reset(lock);
      audioNode->setBus(lock, audioBus);
    }
  } else {
    audioBufferSourceNode->buffer.Reset();

    {
      lab::ContextRenderLock lock(audioContext->audioContext.get(), "AudioBufferSourceNode::buffer");
      audioNode->reset(lock);
      audioNode->setBus(lock, nullptr);
    }
  }
}
NAN_GETTER(AudioBufferSourceNode::OnEndedGetter) {
  // Nan::HandleScope scope;

  AudioBufferSourceNode *audioBufferSourceNode = ObjectWrap::Unwrap<AudioBufferSourceNode>(info.This());
  Local<Function> onended = Nan::New(audioBufferSourceNode->onended);
  info.GetReturnValue().Set(onended);
}
NAN_SETTER(AudioBufferSourceNode::OnEndedSetter) {
  // Nan::HandleScope scope;

  AudioBufferSourceNode *audioBufferSourceNode = ObjectWrap::Unwrap<AudioBufferSourceNode>(info.This());

  if (value->IsFunction()) {
    Local<Function> onended = Local<Function>::Cast(value);
    audioBufferSourceNode->onended.Reset(onended);
  } else {
    audioBufferSourceNode->onended.Reset();
  }
}
void AudioBufferSourceNode::ProcessInMainThread(AudioBufferSourceNode *self) {
  Nan::HandleScope scope;

  if (!self->onended.IsEmpty()) {
    Local<Function> onended = Nan::New(self->onended);
    onended->Call(Isolate::GetCurrent()->GetCurrentContext(), Nan::Null(), 0, nullptr);
  }
}

}
