#include <AudioContext.h>

namespace webaudio {
  
WebAudioAsync *getWebAudioAsync() {
  if (!_webAudioAsync) {
    _webAudioAsync.reset(new WebAudioAsync());
  }
  return _webAudioAsync.get();
}

lab::AudioContext *getDefaultAudioContext(float sampleRate) {
  if (!_defaultAudioContext) {
    _defaultAudioContext = lab::MakeRealtimeAudioContext(sampleRate);

    atexit([]() {
      _defaultAudioContext.reset();
    });
  }
  return _defaultAudioContext.get();
}

AudioContext::AudioContext(float sampleRate) {
  audioContext = lab::MakeRealtimeAudioContext(sampleRate);
}

AudioContext::~AudioContext() {}

Local<Object> AudioContext::Initialize(
	Isolate *isolate, 
	Local<Value> audioListenerCons, 
	Local<Value> audioSourceNodeCons, 
	Local<Value> audioDestinationNodeCons, 
	Local<Value> gainNodeCons, 
	Local<Value> analyserNodeCons, 
	Local<Value> biquadFilterNodeCons, 
	Local<Value> channelMergerNodeCons,
	Local<Value> channelSplitterNodeCons,
	Local<Value> convolverNodeCons,
	Local<Value> delayNodeCons,
	Local<Value> dynamicsCompressNodeCons, 
	Local<Value> pannerNodeCons, 
	Local<Value> audioBufferCons, 
	Local<Value> audioBufferSourceNodeCons, 
	Local<Value> audioProcessingEventCons, 
	Local<Value> stereoPannerNodeCons, 
	Local<Value> oscillatorNodeCons, 
	Local<Value> scriptProcessorNodeCons,
	Local<Value> waveShaperNodeCons,
	Local<Value> mediaStreamTrackCons, 
	Local<Value> microphoneMediaStreamCons) {
#if defined(ANDROID) || defined(LUMIN)
  lab::SetGenericFunctions(
    adgCreate,
    adgDestroy,
    adgStart,
    adgStop,
    adgStartRecording,
    adgStopRecording
  );
#endif

  /* atexit([]{
    uv_close((uv_handle_t *)&threadAsync, nullptr);
    uv_sem_destroy(&threadSemaphore);
  }); */
  
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioContext"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  Nan::SetMethod(proto, "createMediaElementSource", CreateMediaElementSource);
  Nan::SetMethod(proto, "createMediaStreamSource", CreateMediaStreamSource);
  Nan::SetMethod(proto, "createMediaStreamDestination", CreateMediaStreamDestination);
  Nan::SetMethod(proto, "createMediaStreamTrackSource", CreateMediaStreamTrackSource);
  Nan::SetMethod(proto, "createGain", CreateGain);
  Nan::SetMethod(proto, "createAnalyser", CreateAnalyser);
  Nan::SetMethod(proto, "createBiquadFilter", CreateBiquadFilter);
  Nan::SetMethod(proto, "createChannelMerger", CreateChannelMerger);
  Nan::SetMethod(proto, "createChannelSplitter", CreateChannelSplitter);
  Nan::SetMethod(proto, "createConvolver", CreateConvolver);
  Nan::SetMethod(proto, "createDelay", CreateDelay);
  Nan::SetMethod(proto, "createDynamicsCompressor", CreateDynamicsCompressor);
  Nan::SetMethod(proto, "createPanner", CreatePanner);
  Nan::SetMethod(proto, "createStereoPanner", CreateStereoPanner);
  Nan::SetMethod(proto, "createOscillator", CreateOscillator);
  Nan::SetMethod(proto, "createBuffer", CreateBuffer);
  Nan::SetMethod(proto, "createEmptyBuffer", CreateEmptyBuffer);
  Nan::SetMethod(proto, "createBufferSource", CreateBufferSource);
  Nan::SetMethod(proto, "createScriptProcessor", CreateScriptProcessor);
  Nan::SetMethod(proto, "createWaveShaper", CreateWaveShaper);
  Nan::SetMethod(proto, "suspend", Suspend);
  Nan::SetMethod(proto, "resume", Resume);
  Nan::SetMethod(proto, "close", Close);
  Nan::SetAccessor(proto, JS_STR("currentTime"), CurrentTimeGetter);
  Nan::SetAccessor(proto, JS_STR("sampleRate"), SampleRateGetter);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  ctorFn->Set(JS_STR("AudioListener"), audioListenerCons);
  ctorFn->Set(JS_STR("AudioSourceNode"), audioSourceNodeCons);
  ctorFn->Set(JS_STR("AudioDestinationNode"), audioDestinationNodeCons);
  ctorFn->Set(JS_STR("GainNode"), gainNodeCons);
  ctorFn->Set(JS_STR("AnalyserNode"), analyserNodeCons);
  ctorFn->Set(JS_STR("BiquadFilterNode"), biquadFilterNodeCons);
  ctorFn->Set(JS_STR("ChannelMergerNode"), channelMergerNodeCons);
  ctorFn->Set(JS_STR("ChannelSplitterNode"), channelSplitterNodeCons);
  ctorFn->Set(JS_STR("ConvolverNode"), convolverNodeCons);
  ctorFn->Set(JS_STR("DelayNode"), delayNodeCons);
  ctorFn->Set(JS_STR("DynamicsCompressorNode"), dynamicsCompressNodeCons);
  ctorFn->Set(JS_STR("PannerNode"), pannerNodeCons);
  ctorFn->Set(JS_STR("StereoPannerNode"), stereoPannerNodeCons);
  ctorFn->Set(JS_STR("OscillatorNode"), oscillatorNodeCons);
  ctorFn->Set(JS_STR("AudioBuffer"), audioBufferCons);
  ctorFn->Set(JS_STR("AudioBufferSourceNode"), audioBufferSourceNodeCons);
  ctorFn->Set(JS_STR("AudioProcessingEvent"), audioProcessingEventCons);
  ctorFn->Set(JS_STR("ScriptProcessorNode"), scriptProcessorNodeCons);
  ctorFn->Set(JS_STR("WaveShaperNode"), waveShaperNodeCons);
  ctorFn->Set(JS_STR("MediaStreamTrack"), mediaStreamTrackCons);
  ctorFn->Set(JS_STR("MicrophoneMediaStream"), microphoneMediaStreamCons);
  ctorFn->Set(JS_STR("Destroy"), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(Destroy)).ToLocalChecked());

  return scope.Escape(ctorFn);
}

Local<Object> AudioContext::CreateMediaElementSource(Local<Function> audioSourceNodeConstructor, Local<Object> mediaElement, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    mediaElement,
    audioContextObj,
  };
  Local<Object> audioSourceNodeObj = audioSourceNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return audioSourceNodeObj;
}

Local<Object> AudioContext::CreateMediaStreamSource(Local<Function> audioSourceNodeConstructor, Local<Object> mediaStream, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    mediaStream,
    audioContextObj,
  };
  Local<Object> audioSourceNodeObj = audioSourceNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return audioSourceNodeObj;
}

void AudioContext::CreateMediaStreamDestination() {
  Nan::ThrowError("AudioContext::CreateMediaStreamDestination: not implemented"); // TODO
}

void AudioContext::CreateMediaStreamTrackSource() {
  Nan::ThrowError("AudioContext::CreateMediaStreamTrackSource: not implemented"); // TODO
}

Local<Object> AudioContext::CreateGain(Local<Function> gainNodeConstructor, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    audioContextObj,
  };
  Local<Object> gainNodeObj = gainNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return gainNodeObj;
}

Local<Object> AudioContext::CreateAnalyser(Local<Function> analyserNodeConstructor, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	  audioContextObj,
	};
	Local<Object> analyserNodeObj = analyserNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return analyserNodeObj;
}

Local<Object> AudioContext::CreateBiquadFilter(Local<Function> biquadFilterNodeConstructor, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	  audioContextObj,
	};
	Local<Object> biquadFilterNodeObj = biquadFilterNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return biquadFilterNodeObj;
}

Local<Object> AudioContext::CreateChannelMerger(Local<Function> channelMergerNodeConstructor, uint32_t numberOfInputs, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	 JS_INT(numberOfInputs),
	  audioContextObj,
	};
	Local<Object> channelMergerNodeObj = channelMergerNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return channelMergerNodeObj;
}

Local<Object> AudioContext::CreateChannelSplitter(Local<Function> channelSplitterNodeConstructor, uint32_t numberOfOutputs, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	 JS_INT(numberOfOutputs),
	  audioContextObj,
	};
	Local<Object> channelSplitterNodeObj = channelSplitterNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return channelSplitterNodeObj;
}

Local<Object> AudioContext::CreateConvolver(Local<Function> convolverNodeConstructor, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	  audioContextObj,
	};
	Local<Object> convolverNodeObj = convolverNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return convolverNodeObj;
}

Local<Object> AudioContext::CreateDelay(Local<Function> delayNodeConstructor, double maxDelayTime, float sampleRate, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	 JS_NUM(maxDelayTime),
	 JS_NUM(sampleRate),
	  audioContextObj,
	};
	Local<Object> delayNodeObj = delayNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return delayNodeObj;
}

Local<Object> AudioContext::CreateDynamicsCompressor(Local<Function> dynamicsCompressorNodeConstructor, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	  audioContextObj,
	};
	Local<Object> dynamicsCompressorNodeObj = dynamicsCompressorNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return dynamicsCompressorNodeObj;
}

Local<Object> AudioContext::CreatePanner(Local<Function> pannerNodeConstructor, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    audioContextObj,
  };
  Local<Object> pannerNodeObj = pannerNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return pannerNodeObj;
}

Local<Object> AudioContext::CreateStereoPanner(Local<Function> stereoPannerNodeConstructor, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    audioContextObj,
  };
  Local<Object> stereoPannerNodeObj = stereoPannerNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return stereoPannerNodeObj;
}

Local<Object> AudioContext::CreateOscillator(Local<Function> oscillatorNodeConstructor, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    audioContextObj,
  };
  Local<Object> oscillatorNodeObj = oscillatorNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return oscillatorNodeObj;
}

Local<Object> AudioContext::CreateBuffer(Local<Function> audioBufferConstructor, uint32_t numOfChannels, uint32_t length, uint32_t sampleRate) {
  Local<Value> argv[] = {
    JS_INT(numOfChannels),
    JS_INT(length),
    JS_INT(sampleRate),
  };
  Local<Object> audioBufferObj = audioBufferConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return audioBufferObj;
}

Local<Object> AudioContext::CreateEmptyBuffer(Local<Function> audioBufferConstructor, uint32_t sampleRate) {
  Local<Value> argv[] = {
    JS_INT(sampleRate),
  };
  Local<Object> audioBufferObj = audioBufferConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return audioBufferObj;
}

Local<Object> AudioContext::CreateBufferSource(Local<Function> audioBufferSourceNodeConstructor, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    audioContextObj,
  };
  Local<Object> audioBufferObj = audioBufferSourceNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return audioBufferObj;
}

Local<Object> AudioContext::CreateScriptProcessor(Local<Function> scriptProcessorNodeConstructor, uint32_t bufferSize, uint32_t numberOfInputChannels, uint32_t numberOfOutputChannels, Local<Object> audioContextObj) {
  Local<Value> argv[] = {
    JS_INT(bufferSize),
    JS_INT(numberOfInputChannels),
    JS_INT(numberOfOutputChannels),
    audioContextObj,
  };
  Local<Object> scriptProcessorNodeObj = scriptProcessorNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv)/sizeof(argv[0]), argv).ToLocalChecked();

  return scriptProcessorNodeObj;
}

Local<Object> AudioContext::CreateWaveShaper(Local<Function> waveShaperNodeConstructor, Local<Object> audioContextObj) {
	Local<Value> argv[] = {
	  audioContextObj,
	};
	Local<Object> waveShaperNodeObj = waveShaperNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv) / sizeof(argv[0]), argv).ToLocalChecked();

	return waveShaperNodeObj;
}

void AudioContext::Suspend() {
  // Nan::HandleScope scope;

  audioContext->suspend();
}

void AudioContext::Resume() {
  // Nan::HandleScope scope;

  audioContext->resume();
}

void AudioContext::Close() {
  // Nan::HandleScope scope;

  audioContext.reset();
}

NAN_METHOD(AudioContext::New) {
  Local<Object> options = info[0]->IsObject() ? Local<Object>::Cast(info[0]) : Nan::New<Object>();
  Local<Value> sampleRateValue = options->Get(JS_STR("sampleRate"));
  float sampleRate = sampleRateValue->IsNumber() ? TO_DOUBLE(sampleRateValue) : lab::DefaultSampleRate;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = new AudioContext(sampleRate);
  audioContext->Wrap(audioContextObj);

  Local<Function> audioDestinationNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioDestinationNode")));
  Local<Value> argv1[] = {
    audioContextObj,
  };
  Local<Object> audioDestinationNodeObj = audioDestinationNodeConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv1)/sizeof(argv1[0]), argv1).ToLocalChecked();
  audioContextObj->Set(JS_STR("destination"), audioDestinationNodeObj);

  Local<Function> audioListenerConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioListener")));
  Local<Value> argv2[] = {
    audioContextObj,
  };
  Local<Object> audioListenerObj = audioListenerConstructor->NewInstance(Isolate::GetCurrent()->GetCurrentContext(), sizeof(argv2)/sizeof(argv2[0]), argv2).ToLocalChecked();
  audioContextObj->Set(JS_STR("listener"), audioListenerObj);

  info.GetReturnValue().Set(audioContextObj);
}

NAN_METHOD(AudioContext::CreateMediaElementSource) {
  // Nan::HandleScope scope;

  if (info[0]->IsObject() && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("HTMLAudioElement"))) {
    Local<Object> htmlAudioElement = Local<Object>::Cast(info[0]);

    Local<Object> audioContextObj = info.This();
    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

    Local<Function> audioSourceNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioSourceNode")));
    Local<Object> audioNodeObj = audioContext->CreateMediaElementSource(audioSourceNodeConstructor, htmlAudioElement, audioContextObj);

    info.GetReturnValue().Set(audioNodeObj);
  } else {
    Nan::ThrowError("AudioContext::CreateMediaElementSource: invalid arguments");
  }
}

NAN_METHOD(AudioContext::CreateMediaStreamSource) {
  // Nan::HandleScope scope;

  if (info[0]->IsObject() && JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"))->StrictEquals(JS_STR("MicrophoneMediaStream"))) {
    Local<Object> microphoneMediaStream = Local<Object>::Cast(info[0]);

    Local<Object> audioContextObj = info.This();
    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

    Local<Function> audioSourceNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioSourceNode")));
    Local<Object> audioNodeObj = audioContext->CreateMediaStreamSource(audioSourceNodeConstructor, microphoneMediaStream, audioContextObj);

    info.GetReturnValue().Set(audioNodeObj);
  } else {
    Nan::ThrowError("AudioContext::CreateMediaStreamSource: invalid arguments");
  }
}

NAN_METHOD(AudioContext::CreateMediaStreamDestination) {
  // Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->CreateMediaStreamDestination();
}

NAN_METHOD(AudioContext::CreateMediaStreamTrackSource) {
  // Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->CreateMediaStreamTrackSource();
}

NAN_METHOD(AudioContext::CreateGain) {
  // Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> gainNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("GainNode")));
  Local<Object> gainNodeObj = audioContext->CreateGain(gainNodeConstructor, audioContextObj);

  info.GetReturnValue().Set(gainNodeObj);
}

NAN_METHOD(AudioContext::CreateAnalyser) {
	// Nan::HandleScope scope;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	Local<Function> analyserNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AnalyserNode")));
	Local<Object> analyserNodeObj = audioContext->CreateAnalyser(analyserNodeConstructor, audioContextObj);

	info.GetReturnValue().Set(analyserNodeObj);
}

NAN_METHOD(AudioContext::CreateBiquadFilter) {
	// Nan::HandleScope scope;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	Local<Function> biquadFilterNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("BiquadFilterNode")));
	Local<Object> biquadFilterNodeObj = audioContext->CreateBiquadFilter(biquadFilterNodeConstructor, audioContextObj);

	info.GetReturnValue().Set(biquadFilterNodeObj);
}

NAN_METHOD(AudioContext::CreateChannelMerger) {
	// Nan::HandleScope scope;

	uint32_t numberOfInputs = info[0]->IsNumber() ? TO_UINT32(info[0]) : 6;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	Local<Function> channelMergerNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("ChannelMergerNode")));
	Local<Object> channelMergerNodeObj = audioContext->CreateChannelMerger(channelMergerNodeConstructor, numberOfInputs, audioContextObj);

	info.GetReturnValue().Set(channelMergerNodeObj);
}

NAN_METHOD(AudioContext::CreateChannelSplitter) {
	// Nan::HandleScope scope;

	uint32_t numberOfOutputs = info[0]->IsNumber() ? TO_UINT32(info[0]) : 6;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	Local<Function> channelSplitterNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("ChannelSplitterNode")));
	Local<Object> channelSplitterNodeObj = audioContext->CreateChannelSplitter(channelSplitterNodeConstructor, numberOfOutputs, audioContextObj);

	info.GetReturnValue().Set(channelSplitterNodeObj);
}

NAN_METHOD(AudioContext::CreateConvolver) {
	// Nan::HandleScope scope;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	Local<Function> convolverNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("ConvolverNode")));
	Local<Object> convolverNodeObj = audioContext->CreateConvolver(convolverNodeConstructor, audioContextObj);

	info.GetReturnValue().Set(convolverNodeObj);
}

NAN_METHOD(AudioContext::CreateDelay) {
	// Nan::HandleScope scope;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	shared_ptr<lab::AudioContext> labAudioContext = *(shared_ptr<lab::AudioContext> *)(&audioContext->audioContext);

	// lab sound has 2 arguments whereas web audio has one.  Also, the arg in both
	// is the second arg in lab sound.  Switching order, so it is first.
	double maxDelayTime = info[0]->IsNumber() ? TO_DOUBLE(info[0]) : 1;

	// This one uses the context's sample rate, when not specified
	float sampleRate = info[1]->IsNumber() ? TO_FLOAT(info[1]) : labAudioContext->sampleRate();

	Local<Function> delayNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("DelayNode")));
	Local<Object> delayNodeObj = audioContext->CreateDelay(delayNodeConstructor, maxDelayTime, sampleRate, audioContextObj);

	info.GetReturnValue().Set(delayNodeObj);
}

NAN_METHOD(AudioContext::CreateDynamicsCompressor) {
	// Nan::HandleScope scope;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	Local<Function> dynamicsCompressorNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("DynamicsCompressorNode")));
	Local<Object> dynamicsCompressorNodeObj = audioContext->CreateDynamicsCompressor(dynamicsCompressorNodeConstructor, audioContextObj);

	info.GetReturnValue().Set(dynamicsCompressorNodeObj);
}

NAN_METHOD(AudioContext::CreatePanner) {
  // Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> pannerNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("PannerNode")));
  Local<Object> pannerNodeObj = audioContext->CreatePanner(pannerNodeConstructor, audioContextObj);

  info.GetReturnValue().Set(pannerNodeObj);
}

NAN_METHOD(AudioContext::CreateStereoPanner) {
  // Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> stereoPannerNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("StereoPannerNode")));
  Local<Object> stereoPannerNodeObj = audioContext->CreateStereoPanner(stereoPannerNodeConstructor, audioContextObj);

  info.GetReturnValue().Set(stereoPannerNodeObj);
}

NAN_METHOD(AudioContext::CreateOscillator) {
  // Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> oscillatorNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("OscillatorNode")));
  Local<Object> oscillatorNodeObj = audioContext->CreateOscillator(oscillatorNodeConstructor, audioContextObj);

  info.GetReturnValue().Set(oscillatorNodeObj);
}

NAN_METHOD(AudioContext::CreateBuffer) {
  // Nan::HandleScope scope;

  if (info[0]->IsNumber() && info[1]->IsNumber() && info[2]->IsNumber()) {
    uint32_t numOfChannels = TO_UINT32(info[0]);
    uint32_t length = TO_UINT32(info[1]);
    uint32_t sampleRate = TO_UINT32(info[2]);

    Local<Object> audioContextObj = info.This();
    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

    Local<Function> audioBufferConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioBuffer")));
    Local<Object> audioBufferObj = audioContext->CreateBuffer(audioBufferConstructor, numOfChannels, length, sampleRate);

    info.GetReturnValue().Set(audioBufferObj);
  } else {
    Nan::ThrowError("AudioContext::CreateBuffer: invalid arguments");
  }
}

NAN_METHOD(AudioContext::CreateEmptyBuffer) {
  // Nan::HandleScope scope;
  
  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> audioBufferConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioBuffer")));
  uint32_t sampleRate = (uint32_t)audioContext->audioContext->sampleRate();
  Local<Object> audioBufferObj = audioContext->CreateEmptyBuffer(audioBufferConstructor, sampleRate);

  info.GetReturnValue().Set(audioBufferObj);
}

NAN_METHOD(AudioContext::CreateBufferSource) {
  // Nan::HandleScope scope;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> audioBufferSourceNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("AudioBufferSourceNode")));
  Local<Object> audioBufferSourceNodeObj = audioContext->CreateBufferSource(audioBufferSourceNodeConstructor, audioContextObj);

  info.GetReturnValue().Set(audioBufferSourceNodeObj);
}

NAN_METHOD(AudioContext::CreateScriptProcessor) {
  // Nan::HandleScope scope;

  uint32_t bufferSize = info[0]->IsNumber() ? TO_UINT32(info[0]) : 256;
  uint32_t numberOfInputChannels = info[1]->IsNumber() ? TO_UINT32(info[1]) : 2;
  uint32_t numberOfOutputChannels = info[2]->IsNumber() ? TO_UINT32(info[2]) : 2;

  Local<Object> audioContextObj = info.This();
  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

  Local<Function> scriptProcessorNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("ScriptProcessorNode")));
  Local<Object> scriptProcessorNodeObj = audioContext->CreateScriptProcessor(scriptProcessorNodeConstructor, bufferSize, numberOfInputChannels, numberOfOutputChannels, audioContextObj);

  info.GetReturnValue().Set(scriptProcessorNodeObj);
}

NAN_METHOD(AudioContext::CreateWaveShaper) {
	// Nan::HandleScope scope;

	Local<Object> audioContextObj = info.This();
	AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);

	Local<Function> waveShaperNodeConstructor = Local<Function>::Cast(JS_OBJ(audioContextObj->Get(JS_STR("constructor")))->Get(JS_STR("WaveShaperNode")));
	Local<Object> waveShaperNodeObj = audioContext->CreateGain(waveShaperNodeConstructor, audioContextObj);

	info.GetReturnValue().Set(waveShaperNodeObj);
}


NAN_METHOD(AudioContext::Suspend) {
  // Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->Suspend();
}

NAN_METHOD(AudioContext::Resume) {
  // Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->Resume();
}

NAN_METHOD(AudioContext::Close) {
  // Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  audioContext->Close();
}

NAN_METHOD(AudioContext::Destroy) {
  // Nan::HandleScope scope;

  if (_webAudioAsync) {
    _webAudioAsync.reset();
  }
}

NAN_GETTER(AudioContext::CurrentTimeGetter) {
  // Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  info.GetReturnValue().Set(JS_NUM(audioContext->audioContext->currentTime()));
}

NAN_GETTER(AudioContext::SampleRateGetter) {
  // Nan::HandleScope scope;

  AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(info.This());
  info.GetReturnValue().Set(JS_NUM(audioContext->audioContext->sampleRate()));
}

WebAudioAsync::WebAudioAsync() : threadAsync(new uv_async_t()) {
  uv_async_init(windowsystembase::GetEventLoop(), threadAsync, RunInMainThread);
  threadAsync->data = this;
}

WebAudioAsync::~WebAudioAsync() {
  uv_close((uv_handle_t *)threadAsync, [](uv_handle_t *handle) {
    delete handle;
  });
}

void WebAudioAsync::QueueOnMainThread(lab::ContextRenderLock &r, function<void()> &&newThreadFn) {
  {
    std::lock_guard<std::mutex> lock(mutex);

    threadFns.emplace_back(std::move(newThreadFn));
  }
  uv_async_send(threadAsync);
}
void WebAudioAsync::QueueOnMainThread(function<void()> &&newThreadFn) {
  {
    std::lock_guard<std::mutex> lock(mutex);

    threadFns.emplace_back(std::move(newThreadFn));
  }
  uv_async_send(threadAsync);
}
void WebAudioAsync::RunInMainThread(uv_async_t *handle) {
  WebAudioAsync *webAudioAsync = (WebAudioAsync *)handle->data;

  for (;;) {
    function<void()> threadFn;
    {
      std::lock_guard<std::mutex> lock(webAudioAsync->mutex);

      if (webAudioAsync->threadFns.size() > 0) {
        threadFn = std::move(webAudioAsync->threadFns.front());
        webAudioAsync->threadFns.pop_front();
      }
    }
    if (threadFn) {
      threadFn();
    } else {
      break;
    }
  }
}

thread_local unique_ptr<lab::AudioContext> _defaultAudioContext;
thread_local unique_ptr<WebAudioAsync> _webAudioAsync;

}

/* // XXX hack until we rebuild LabSound for android audio bindings support
#ifdef ANDROID

typedef int32_t MLResult;
typedef uint64_t MLHandle;
typedef void MLAudioBuffer;
typedef void MLAudioBufferFormat;
typedef void (*MLAudioBufferCallback)(MLHandle handle, void *callback_context);

extern "C" {
MLResult MLAudioCreateSoundWithOutputStream(const MLAudioBufferFormat *format, uint32_t buffer_size, MLAudioBufferCallback callback, void *callback_context, MLHandle *out_handle) { return 0; }
MLResult MLAudioCreateInputFromVoiceComm(const MLAudioBufferFormat *format, uint32_t buffer_size, MLAudioBufferCallback callback, void *callback_context, MLHandle *out_handle) { return 0; }
MLResult MLAudioReleaseOutputStreamBuffer(MLHandle handle) { return 0; }
MLResult MLAudioReleaseInputStreamBuffer(MLHandle handle) { return 0; }
MLResult MLAudioGetOutputStreamBuffer(MLHandle handle, MLAudioBuffer *out_buf) { return 0; }
MLResult MLAudioGetInputStreamBuffer(MLHandle handle, MLAudioBuffer *out_buf) { return 0; }
MLResult MLAudioStartSound(MLHandle handle) { return 0; }
MLResult MLAudioStopSound(MLHandle handle) { return 0; }
MLResult MLAudioStartInput(MLHandle handle) { return 0; }
MLResult MLAudioStopInput(MLHandle handle) { return 0; }
}

#endif */
