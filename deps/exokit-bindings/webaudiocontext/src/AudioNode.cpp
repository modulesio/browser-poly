#include <AudioNode.h>
#include <AudioContext.h>

namespace webaudio {

AudioNode::AudioNode() : inputAudioNodes(Nan::New<Array>(0)), outputAudioNodes(Nan::New<Array>(0)) {}

AudioNode::~AudioNode() {}

Local<Object> AudioNode::Initialize(Isolate *isolate) {
  Nan::EscapableHandleScope scope;

  // constructor
  Local<FunctionTemplate> ctor = Nan::New<FunctionTemplate>(New);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("AudioNode"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  AudioNode::InitializePrototype(proto);

  Local<Function> ctorFn = Nan::GetFunction(ctor).ToLocalChecked();

  return scope.Escape(ctorFn);
}

void AudioNode::InitializePrototype(Local<ObjectTemplate> proto) {
  Nan::SetMethod(proto, "connect", Connect);
  Nan::SetMethod(proto, "disconnect", Disconnect);
  Nan::SetAccessor(proto, JS_STR("context"), ContextGetter);
  Nan::SetAccessor(proto, JS_STR("numberOfInputsGetter"), NumberOfInputsGetter);
  Nan::SetAccessor(proto, JS_STR("numberOfOutputsGetter"), NumberOfOutputsGetter);
  Nan::SetAccessor(proto, JS_STR("channelCount"), ChannelCountGetter);
  Nan::SetAccessor(proto, JS_STR("channelCountMode"), ChannelCountModeGetter);
  Nan::SetAccessor(proto, JS_STR("channelInterpretation"), ChannelInterpretationGetter);
}

NAN_METHOD(AudioNode::New) {
  Nan::HandleScope scope;

  AudioNode *audioNode = new AudioNode();
  Local<Object> audioNodeObj = info.This();
  audioNode->Wrap(audioNodeObj);

  info.GetReturnValue().Set(audioNodeObj);
}

NAN_METHOD(AudioNode::Connect) {
  Nan::HandleScope scope;

  if (info[0]->IsObject()) {
    Local<Value> constructorName = JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"));

    if (
      constructorName->StrictEquals(JS_STR("AudioSourceNode")) ||
      constructorName->StrictEquals(JS_STR("AudioDestinationNode")) ||
      constructorName->StrictEquals(JS_STR("GainNode")) ||
	  constructorName->StrictEquals(JS_STR("AnalyserNode")) ||
	  constructorName->StrictEquals(JS_STR("BiquadFilterNode")) ||
	  constructorName->StrictEquals(JS_STR("ChannelMergerNode")) ||
	  constructorName->StrictEquals(JS_STR("ChannelSplitterNode")) ||
	  constructorName->StrictEquals(JS_STR("ConvolverNode")) ||
	  constructorName->StrictEquals(JS_STR("DelayNode")) ||
	  constructorName->StrictEquals(JS_STR("DynamicsCompressorNode")) ||
	  constructorName->StrictEquals(JS_STR("OscillatorNode")) ||
	  constructorName->StrictEquals(JS_STR("PannerNode")) ||
      constructorName->StrictEquals(JS_STR("StereoPannerNode")) ||
      constructorName->StrictEquals(JS_STR("ScriptProcessorNode")) ||
	  constructorName->StrictEquals(JS_STR("WaveShaperNode"))
    ) {
      unsigned int outputIndex = info[1]->IsNumber() ? TO_UINT32(info[1]) : 0;
      unsigned int inputIndex = info[2]->IsNumber() ? TO_UINT32(info[2]) : 0;

      Local<Object> srcAudioNodeObj = info.This();
      AudioNode *srcAudioNode = ObjectWrap::Unwrap<AudioNode>(srcAudioNodeObj);
      shared_ptr<lab::AudioNode> srcLabAudioNode = srcAudioNode->audioNode;

      Local<Object> dstAudioNodeObj = Local<Object>::Cast(info[0]);
      AudioNode *dstAudioNode = ObjectWrap::Unwrap<AudioNode>(Local<Object>::Cast(dstAudioNodeObj));
      shared_ptr<lab::AudioNode> dstLabAudioNode = dstAudioNode->audioNode;

      Local<Object> audioContextObj = Nan::New(srcAudioNode->context);
      AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
      // lab::AudioContext *labAudioContext = audioContext->audioContext;

      // try {
        audioContext->audioContext->connect(dstLabAudioNode, srcLabAudioNode, outputIndex, inputIndex);
      /* } catch (const std::exception &e) {
        Nan::ThrowError(e.what());
        return;
      } catch (...) {
        Nan::ThrowError("unknown exception");
        return;
      } */

      Nan::New(srcAudioNode->outputAudioNodes)->Set(outputIndex, dstAudioNodeObj);
      Nan::New(dstAudioNode->inputAudioNodes)->Set(inputIndex, srcAudioNodeObj);

      info.GetReturnValue().Set(info[0]);
    } else {
      Nan::ThrowError("AudioNode::Connect: invalid arguments");
    }
  } else {
    Nan::ThrowError("AudioNode::Connect: invalid arguments");
  }
}

NAN_METHOD(AudioNode::Disconnect) {
  Nan::HandleScope scope;

  if (info.Length() == 0) {
    Local<Object> srcAudioNodeObj = info.This();
    AudioNode *srcAudioNode = ObjectWrap::Unwrap<AudioNode>(srcAudioNodeObj);
    shared_ptr<lab::AudioNode> srcLabAudioNode = srcAudioNode->audioNode;

    Local<Object> audioContextObj = Nan::New(srcAudioNode->context);
    AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
    // lab::AudioContext *labAudioContext = audioContext->audioContext;

    // try {
      audioContext->audioContext->disconnect(nullptr, srcLabAudioNode);
    /* } catch (const std::exception &e) {
      Nan::ThrowError(e.what());
      return;
    } catch (...) {
      Nan::ThrowError("unknown exception");
      return;
    } */

    Local<Array> outputAudioNodes = Nan::New(srcAudioNode->outputAudioNodes);
    Local<Array> inputAudioNodes = Nan::New(srcAudioNode->inputAudioNodes);
    size_t numOutputAudioNodes = outputAudioNodes->Length();
    size_t numInputAudioNodes = inputAudioNodes->Length();
    for (size_t i = 0; i < numOutputAudioNodes; i++) {
      Local<Value> outputAudioNode = outputAudioNodes->Get(i);

      if (TO_BOOL(outputAudioNode)) {
        Local<Object> outputAudioNodeObj = Local<Object>::Cast(outputAudioNode);
        AudioNode *outputAudioNode = ObjectWrap::Unwrap<AudioNode>(outputAudioNodeObj);

        for (size_t j = 0; j < numInputAudioNodes; j++) {
          Local<Value> inputAudioNode = inputAudioNodes->Get(j);

          if (TO_BOOL(inputAudioNode)) {
            Local<Object> inputAudioNodeObj = Local<Object>::Cast(inputAudioNode);
            AudioNode *inputAudioNode = ObjectWrap::Unwrap<AudioNode>(inputAudioNodeObj);
            if (inputAudioNode == srcAudioNode) {
              inputAudioNodes->Set(j, Nan::Null());
            }
          }
        }
        outputAudioNodes->Set(i, Nan::Null());
      }
    }
  } else {
    if (info[0]->IsObject()) {
      Local<Value> constructorName = JS_OBJ(JS_OBJ(info[0])->Get(JS_STR("constructor")))->Get(JS_STR("name"));

      if (
		constructorName->StrictEquals(JS_STR("AudioSourceNode")) ||
		constructorName->StrictEquals(JS_STR("AudioDestinationNode")) ||
		constructorName->StrictEquals(JS_STR("GainNode")) ||
		constructorName->StrictEquals(JS_STR("AnalyserNode")) ||
		constructorName->StrictEquals(JS_STR("BiquadFilterNode")) ||
		constructorName->StrictEquals(JS_STR("ChannelMergerNode")) ||
		constructorName->StrictEquals(JS_STR("ChannelSplitterNode")) ||
		constructorName->StrictEquals(JS_STR("ConvolverNode")) ||
		constructorName->StrictEquals(JS_STR("DelayNode")) ||
		constructorName->StrictEquals(JS_STR("DynamicsCompressorNode")) ||
		constructorName->StrictEquals(JS_STR("OscillatorNode")) ||
		constructorName->StrictEquals(JS_STR("PannerNode")) ||
		constructorName->StrictEquals(JS_STR("StereoPannerNode")) ||
		constructorName->StrictEquals(JS_STR("ScriptProcessorNode")) ||
		constructorName->StrictEquals(JS_STR("WaveShaperNode"))
		  ) {
        Local<Object> srcAudioNodeObj = info.This();
        AudioNode *srcAudioNode = ObjectWrap::Unwrap<AudioNode>(srcAudioNodeObj);
        shared_ptr<lab::AudioNode> srcLabAudioNode = srcAudioNode->audioNode;

        Local<Object> dstAudioNodeObj = Local<Object>::Cast(info[0]);
        AudioNode *dstAudioNode = ObjectWrap::Unwrap<AudioNode>(dstAudioNodeObj);
        shared_ptr<lab::AudioNode> dstLabAudioNode = dstAudioNode->audioNode;

        Local<Object> audioContextObj = Nan::New(srcAudioNode->context);
        AudioContext *audioContext = ObjectWrap::Unwrap<AudioContext>(audioContextObj);
        // lab::AudioContext *labAudioContext = audioContext->audioContext;
        
        // try {
          audioContext->audioContext->disconnect(dstLabAudioNode, srcLabAudioNode);
        /* } catch (const std::exception &e) {
          Nan::ThrowError(e.what());
          return;
        } catch (...) {
          Nan::ThrowError("unknown exception");
          return;
        } */

        Local<Array> outputAudioNodes = Nan::New(srcAudioNode->outputAudioNodes);
        Local<Array> inputAudioNodes = Nan::New(srcAudioNode->inputAudioNodes);
        size_t numOutputAudioNodes = outputAudioNodes->Length();
        size_t numInputAudioNodes = inputAudioNodes->Length();
        for (size_t i = 0; i < numOutputAudioNodes; i++) {
          Local<Value> outputAudioNode = outputAudioNodes->Get(i);

          if (TO_BOOL(outputAudioNode)) {
            Local<Object> outputAudioNodeObj = Local<Object>::Cast(outputAudioNode);
            AudioNode *outputAudioNode = ObjectWrap::Unwrap<AudioNode>(outputAudioNodeObj);

            if (outputAudioNode == dstAudioNode) {
              for (size_t j = 0; j < numInputAudioNodes; j++) {
                Local<Value> inputAudioNode = inputAudioNodes->Get(j);

                if (TO_BOOL(inputAudioNode)) {
                  Local<Object> inputAudioNodeObj = Local<Object>::Cast(inputAudioNode);
                  AudioNode *inputAudioNode = ObjectWrap::Unwrap<AudioNode>(inputAudioNodeObj);
                  
                  if (inputAudioNode == srcAudioNode) {
                    inputAudioNodes->Set(j, Nan::Null());
                  }
                }
              }
              outputAudioNodes->Set(i, Nan::Null());
            }
          }
        }

        info.GetReturnValue().Set(info[0]);
      } else {
        Nan::ThrowError("AudioNode::Disconnect: invalid arguments");
      }
    } else {
      Nan::ThrowError("AudioNode::Disconnect: invalid arguments");
    }
  }
}

NAN_GETTER(AudioNode::ContextGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());
  if (!audioNode->context.IsEmpty()) {
    info.GetReturnValue().Set(Nan::New(audioNode->context));
  } else {
    info.GetReturnValue().Set(Nan::Null());
  }
}

NAN_GETTER(AudioNode::NumberOfInputsGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  unsigned int numberOfInputs = audioNode->audioNode->numberOfInputs();

  info.GetReturnValue().Set(JS_INT(numberOfInputs));
}

NAN_GETTER(AudioNode::NumberOfOutputsGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  unsigned int numberOfOutputs = audioNode->audioNode->numberOfOutputs();

  info.GetReturnValue().Set(JS_INT(numberOfOutputs));
}

NAN_GETTER(AudioNode::ChannelCountGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  unsigned int channelCount = audioNode->audioNode->channelCount();

  info.GetReturnValue().Set(JS_INT(channelCount));
}

NAN_GETTER(AudioNode::ChannelCountModeGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  lab::ChannelCountMode channelCountMode = audioNode->audioNode->channelCountMode();
  Local<String> result;
  switch (channelCountMode) {
    case lab::ChannelCountMode::Max: {
      result = Nan::New<String>("max").ToLocalChecked();
      break;
    }
    case lab::ChannelCountMode::ClampedMax: {
      result = Nan::New<String>("clamped-max").ToLocalChecked();
      break;
    }
    case lab::ChannelCountMode::Explicit: {
      result = Nan::New<String>("explicit").ToLocalChecked();
      break;
    }
    default: {
      result = Nan::New<String>("").ToLocalChecked();
      break;
    }
  }

  info.GetReturnValue().Set(result);
}

NAN_GETTER(AudioNode::ChannelInterpretationGetter) {
  Nan::HandleScope scope;

  AudioNode *audioNode = ObjectWrap::Unwrap<AudioNode>(info.This());

  lab::ChannelInterpretation channelInterpretation = audioNode->audioNode->channelInterpretation();
  Local<String> result;
  switch (channelInterpretation) {
    case lab::ChannelInterpretation::Speakers: {
      result = Nan::New<String>("speakers").ToLocalChecked();
      break;
    }
    case lab::ChannelInterpretation::Discrete: {
      result = Nan::New<String>("discrete").ToLocalChecked();
      break;
    }
    default: {
      result = Nan::New<String>("").ToLocalChecked();
      break;
    }
  }

  info.GetReturnValue().Set(result);
}

}
