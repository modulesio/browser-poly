#ifndef _AUDIO_BUFFER_H_
#define _AUDIO_BUFFER_H_

#include <v8.h>
#include <node.h>
#include <nan.h>
#include <functional>
// #include "LabSound/extended/LabSound.h"
#include <defines.h>
#include <AudioNode.h>

using namespace std;
using namespace v8;
using namespace node;

namespace webaudio {

class AudioBufferSourceNode;
class ScriptProcessorNode;

class AudioBuffer : public ObjectWrap {
public:
  static Local<Object> Initialize(Isolate *isolate);

protected:
  static NAN_METHOD(New);

  AudioBuffer(uint32_t sampleRate, Local<Array> buffers);
  ~AudioBuffer();

  static NAN_GETTER(SampleRate);
  static NAN_GETTER(Length);
  static NAN_GETTER(Duration);
  static NAN_GETTER(NumberOfChannels);
  static NAN_METHOD(GetChannelData);
  static NAN_METHOD(CopyFromChannel);
  static NAN_METHOD(CopyToChannel);
  static NAN_METHOD(Load);
  
  void Load(Local<ArrayBuffer> arrayBuffer, size_t byteOffset, size_t byteLength, Local<Function> cbFn);
  static void ProcessLoadInMainThread(AudioBuffer *audioBuffer);

  uint32_t sampleRate;
  Nan::Persistent<Array> buffers;
  Nan::Persistent<Function> cbFn;
  shared_ptr<lab::AudioBus> audioBus;
  string error;

  friend class AudioBufferSourceNode;
  friend class ScriptProcessorNode;
  friend class ConvolverNode;
};

class AudioBufferSourceNode : public AudioNode {
public:
  static Local<Object> Initialize(Isolate *isolate, Local<Value> audioParamCons);
  static void InitializePrototype(Local<ObjectTemplate> proto);

protected:
  AudioBufferSourceNode();
  ~AudioBufferSourceNode();

  static NAN_METHOD(New);
  static NAN_METHOD(Start);
  static NAN_METHOD(Stop);
  static NAN_GETTER(BufferGetter);
  static NAN_SETTER(BufferSetter);
  static NAN_GETTER(OnEndedGetter);
  static NAN_SETTER(OnEndedSetter);
  static void ProcessInMainThread(AudioBufferSourceNode *self);

  Nan::Persistent<Object> buffer;
  Nan::Persistent<Function> onended;
};

}

#endif
