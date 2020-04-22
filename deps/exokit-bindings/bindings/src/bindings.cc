#include "bindings.h"

Local<Object> makeConsole() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(console::Initialize(isolate));
}

Local<Object> makeCache() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(cache::Initialize(isolate));
}

std::pair<Local<Object>, Local<FunctionTemplate>> makeGl() {
  return WebGLRenderingContext::Initialize(Isolate::GetCurrent());
}

std::pair<Local<Object>, Local<FunctionTemplate>> makeGl2(Local<FunctionTemplate> baseCtor) {
  return WebGL2RenderingContext::Initialize(Isolate::GetCurrent(), baseCtor);
}

Local<Object> makeImage() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(Image::Initialize(isolate));
}

Local<Object> makeImageData() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(ImageData::Initialize(isolate));
}

Local<Object> makeImageBitmap() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(ImageBitmap::Initialize(isolate));
}

Local<Object> makeCanvasRenderingContext2D(Local<Value> imageDataCons, Local<Value> canvasGradientCons, Local<Value> canvasPatternCons) {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(CanvasRenderingContext2D::Initialize(isolate, imageDataCons, canvasGradientCons, canvasPatternCons));
}

Local<Object> makePath2D() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(Path2D::Initialize(isolate));
}

Local<Object> makeCanvasGradient() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(CanvasGradient::Initialize(isolate));
}

Local<Object> makeCanvasPattern() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  return scope.Escape(CanvasPattern::Initialize(isolate));
}

Local<Object> makeVideo(Local<Value> imageDataCons) {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  Local<Object> exports = Nan::New<Object>();

  exports->Set(JS_STR("Video"), ffmpeg::Video::Initialize(isolate));
  exports->Set(JS_STR("VideoDevice"), ffmpeg::VideoDevice::Initialize(isolate, imageDataCons));

  return scope.Escape(exports);
}

Local<Object> makeAudio() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  Local<Object> exports = Nan::New<Object>();

  exports->Set(JS_STR("Audio"), webaudio::Audio::Initialize(isolate));

  Local<Value> audioParamCons = webaudio::AudioParam::Initialize(isolate);
  exports->Set(JS_STR("AudioParam"), audioParamCons);

  Local<Value> fakeAudioParamCons = webaudio::FakeAudioParam::Initialize(isolate);
  exports->Set(JS_STR("FakeAudioParam"), fakeAudioParamCons);

  Local<Value> audioListenerCons = webaudio::AudioListener::Initialize(isolate, fakeAudioParamCons);
  exports->Set(JS_STR("AudioListener"), audioListenerCons);

  Local<Value> audioSourceNodeCons = webaudio::AudioSourceNode::Initialize(isolate);
  exports->Set(JS_STR("AudioSourceNode"), audioSourceNodeCons);

  Local<Value> audioDestinationNodeCons = webaudio::AudioDestinationNode::Initialize(isolate);
  exports->Set(JS_STR("AudioDestinationNode"), audioDestinationNodeCons);

  Local<Value> gainNodeCons = webaudio::GainNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("GainNode"), gainNodeCons);

  Local<Value> analyserNodeCons = webaudio::AnalyserNode::Initialize(isolate);
  exports->Set(JS_STR("AnalyserNode"), analyserNodeCons);

  Local<Value> biquadFilterNodeCons = webaudio::BiquadFilterNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("BiquadFilterNode"), biquadFilterNodeCons);

  Local<Value> channelMergerNodeCons = webaudio::ChannelMergerNode::Initialize(isolate);
  exports->Set(JS_STR("ChannelMergerNode"), channelMergerNodeCons);

  Local<Value> channelSplitterNodeCons = webaudio::ChannelSplitterNode::Initialize(isolate);
  exports->Set(JS_STR("ChannelSplitterNode"), channelSplitterNodeCons);

  Local<Value> convolverNodeCons = webaudio::ConvolverNode::Initialize(isolate);
  exports->Set(JS_STR("ConvolverNode"), convolverNodeCons);

  Local<Value> delayNodeCons = webaudio::DelayNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("DelayNode"), delayNodeCons);

  Local<Value> dynamicsCompressorNodeCons = webaudio::DynamicsCompressorNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("DynamicsCompressorNode"), dynamicsCompressorNodeCons);

  Local<Value> pannerNodeCons = webaudio::PannerNode::Initialize(isolate, fakeAudioParamCons);
  exports->Set(JS_STR("PannerNode"), pannerNodeCons);

  Local<Value> stereoPannerNodeCons = webaudio::StereoPannerNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("StereoPannerNode"), stereoPannerNodeCons);

  Local<Value> oscillatorNodeCons = webaudio::OscillatorNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("OscillatorNode"), oscillatorNodeCons);

  Local<Value> audioBufferCons = webaudio::AudioBuffer::Initialize(isolate);
  exports->Set(JS_STR("AudioBuffer"), audioBufferCons);

  Local<Value> audioBufferSourceNodeCons = webaudio::AudioBufferSourceNode::Initialize(isolate, audioParamCons);
  exports->Set(JS_STR("AudioBufferSourceNode"), audioBufferSourceNodeCons);

  Local<Value> audioProcessingEventCons = webaudio::AudioProcessingEvent::Initialize(isolate);
  exports->Set(JS_STR("AudioProcessingEvent"), audioProcessingEventCons);

  Local<Value> scriptProcessorNodeCons = webaudio::ScriptProcessorNode::Initialize(isolate, audioBufferCons, audioProcessingEventCons);
  exports->Set(JS_STR("ScriptProcessorNode"), scriptProcessorNodeCons);

  Local<Value> waveShaperNodeCons = webaudio::WaveShaperNode::Initialize(isolate);
  exports->Set(JS_STR("WaveShaperNode"), waveShaperNodeCons);

  Local<Value> mediaStreamTrackCons = webaudio::MediaStreamTrack::Initialize(isolate);
  exports->Set(JS_STR("MediaStreamTrack"), mediaStreamTrackCons);

  Local<Value> microphoneMediaStreamCons = webaudio::MicrophoneMediaStream::Initialize(isolate, mediaStreamTrackCons);
  exports->Set(JS_STR("MicrophoneMediaStream"), microphoneMediaStreamCons);

  exports->Set(JS_STR("AudioContext"), webaudio::AudioContext::Initialize(isolate, audioListenerCons, audioSourceNodeCons, audioDestinationNodeCons, gainNodeCons, analyserNodeCons, biquadFilterNodeCons, channelMergerNodeCons, channelSplitterNodeCons, convolverNodeCons, delayNodeCons, dynamicsCompressorNodeCons, pannerNodeCons, audioBufferCons, audioBufferSourceNodeCons, audioProcessingEventCons, stereoPannerNodeCons, oscillatorNodeCons, scriptProcessorNodeCons, waveShaperNodeCons, mediaStreamTrackCons, microphoneMediaStreamCons));

  return scope.Escape(exports);
}

#if defined(ANDROID) || defined(LUMIN)
Local<Object> makeBrowser() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  Local<Object> exports = Nan::New<Object>();

  exports->Set(JS_STR("Browser"), browser::Browser::Initialize(isolate));

  return scope.Escape(exports);
}
#endif

Local<Object> makeRtc() {
  Isolate *isolate = Isolate::GetCurrent();

  Nan::EscapableHandleScope scope;

  Local<Object> exports = Nan::New<Object>();

  node_webrtc::init(exports);

  return scope.Escape(exports);
}
