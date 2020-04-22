#ifndef _AUDIO_CONTEXT_H_
#define _AUDIO_CONTEXT_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <Audio.h>
#include <AudioNode.h>
#include <AudioSourceNode.h>
#include <AudioDestinationNode.h>
#include <GainNode.h>
#include <AnalyserNode.h>
#include <BiquadFilterNode.h>
#include <ChannelMergerNode.h>
#include <ChannelSplitterNode.h>
#include <ConvolverNode.h>
#include <DelayNode.h>
#include <DynamicsCompressorNode.h>
#include <PannerNode.h>
#include <OscillatorNode.h>
#include <StereoPannerNode.h>
#include <AudioBuffer.h>
#include <ScriptProcessorNode.h>
#include <WaveShaperNode.h>
#include <AudioListener.h>
#include <MediaStreamTrack.h>
#include <MicrophoneMediaStream.h>
#include <AudioDestinationGenericImpl.h>
#include <threadpool.h>
#include <windowsystem.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {

class WebAudioAsync;

WebAudioAsync *getWebAudioAsync();
lab::AudioContext *getDefaultAudioContext(float sampleRate = lab::DefaultSampleRate);

class AudioContext : public ObjectWrap {
public:
  static Local<Object> Initialize(
	  Isolate *isolate, 
	  Local<Value> audioListenerCons, 
	  Local<Value> audioSourceNodeCons, 
	  Local<Value> audioDestinationNodeCons, 
	  Local<Value> gainNodeCons, 
	  Local<Value> analyserNodeCons, 
	  Local<Value> biquadFilterNodeCons, 
	  Local<Value> ChannelMergerNodeCons,
	  Local<Value> ChannelSplitterNodeCons,
	  Local<Value> ConvolverNodeCons,
	  Local<Value> DelayNodeCons,
	  Local<Value> dynamicsCompressorNodeCons, 
	  Local<Value> pannerNodeCons, 
	  Local<Value> audioBufferCons, 
	  Local<Value> audioBufferSourceNodeCons, 
	  Local<Value> audioProcessingEventCons, 
	  Local<Value> stereoPannerNodeCons, 
	  Local<Value> oscillatorNodeCons, 
	  Local<Value> scriptProcessorNodeCons, 
	  Local<Value> waveShaperCons,
	  Local<Value> mediaStreamTrackCons, 
	  Local<Value> microphoneMediaStreamCons);
  void Close();
  Local<Object> CreateMediaElementSource(Local<Function> audioDestinationNodeConstructor, Local<Object> mediaElement, Local<Object> audioContextObj);
  Local<Object> CreateMediaStreamSource(Local<Function> audioSourceNodeConstructor, Local<Object> mediaStream, Local<Object> audioContextObj);
  void CreateMediaStreamDestination();
  void CreateMediaStreamTrackSource();
  Local<Object> CreateGain(Local<Function> gainNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateAnalyser(Local<Function> analyserNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateBiquadFilter(Local<Function> biquadFilterNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateChannelMerger(Local<Function> ChannelMergerNodeConstructor, uint32_t numberOfInputs, Local<Object> audioContextObj);
  Local<Object> CreateChannelSplitter(Local<Function> ChannelSplitterNodeConstructor, uint32_t numberOfOutputs, Local<Object> audioContextObj);
  Local<Object> CreateConvolver(Local<Function> ConvolverNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateDelay(Local<Function> DelayNodeConstructor, double maxDelayTime, float sampleRate, Local<Object> audioContextObj);
  Local<Object> CreateDynamicsCompressor(Local<Function> dynamicsCompressorNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreatePanner(Local<Function> pannerNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateStereoPanner(Local<Function> stereoPannerNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateOscillator(Local<Function> oscillatorNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateBuffer(Local<Function> audioBufferConstructor, uint32_t numOfChannels, uint32_t length, uint32_t sampleRate);
  Local<Object> CreateEmptyBuffer(Local<Function> audioBufferConstructor, uint32_t sampleRate);
  Local<Object> CreateBufferSource(Local<Function> audioBufferSourceNodeConstructor, Local<Object> audioContextObj);
  Local<Object> CreateScriptProcessor(Local<Function> scriptProcessorNodeConstructor, uint32_t bufferSize, uint32_t numberOfInputChannels, uint32_t numberOfOutputChannels, Local<Object> audioContextObj);
  Local<Object> CreateWaveShaper(Local<Function> WaveShaperNodeConstructor, Local<Object> audioContextObj);
  void Suspend();
  void Resume();

  static NAN_METHOD(New);
  static NAN_METHOD(Close);
  static NAN_METHOD(Destroy);
  static NAN_METHOD(CreateMediaElementSource);
  static NAN_METHOD(CreateMediaStreamSource);
  static NAN_METHOD(CreateMediaStreamDestination);
  static NAN_METHOD(CreateMediaStreamTrackSource);
  static NAN_METHOD(CreateGain);
  static NAN_METHOD(CreateAnalyser);
  static NAN_METHOD(CreateBiquadFilter);
  static NAN_METHOD(CreateChannelMerger);
  static NAN_METHOD(CreateChannelSplitter);
  static NAN_METHOD(CreateConvolver);
  static NAN_METHOD(CreateDelay);
  static NAN_METHOD(CreateDynamicsCompressor);
  static NAN_METHOD(CreatePanner);
  static NAN_METHOD(CreateStereoPanner);
  static NAN_METHOD(CreateOscillator);
  static NAN_METHOD(CreateBuffer);
  static NAN_METHOD(CreateEmptyBuffer);
  static NAN_METHOD(CreateBufferSource);
  static NAN_METHOD(CreateScriptProcessor);
  static NAN_METHOD(CreateWaveShaper);
  static NAN_METHOD(Suspend);
  static NAN_METHOD(Resume);
  static NAN_GETTER(CurrentTimeGetter);
  static NAN_GETTER(SampleRateGetter);

  AudioContext(float sampleRate);
  ~AudioContext();

// protected:
  std::unique_ptr<lab::AudioContext> audioContext;

  friend class Audio;
  friend class AudioListener;
  friend class AudioNode;
  friend class AudioSourceNode;
  friend class AudioDestinationNode;
  friend class GainNode;
  friend class AudioParam;
  friend class AudioAnalyser;
  friend class AudioBufferSourceNode;
  friend class ScriptProcessorNode;
};

class WebAudioAsync {
public:
  WebAudioAsync();
  ~WebAudioAsync();

  void QueueOnMainThread(lab::ContextRenderLock &r, function<void()> &&newThreadFn);
  void QueueOnMainThread(function<void()> &&newThreadFn);
  static void RunInMainThread(uv_async_t *handle);

// protected:
  std::deque<function<void()>> threadFns;
  std::mutex mutex;
  uv_async_t *threadAsync;
};

extern thread_local unique_ptr<lab::AudioContext> _defaultAudioContext;
extern thread_local unique_ptr<WebAudioAsync> _webAudioAsync;

}

#endif
