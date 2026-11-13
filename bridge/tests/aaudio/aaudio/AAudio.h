#pragma once
#include <cstdint>
using aaudio_result_t=int32_t;
using aaudio_data_callback_result_t=int32_t;
constexpr int AAUDIO_OK=0, AAUDIO_DIRECTION_OUTPUT=0, AAUDIO_FORMAT_PCM_I16=1,
AAUDIO_SHARING_MODE_SHARED=1, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY=12, AAUDIO_CALLBACK_RESULT_CONTINUE=0;
struct AAudioStream {}; struct AAudioStreamBuilder {};
using DataCallback=aaudio_data_callback_result_t(*)(AAudioStream*,void*,void*,int32_t);
using ErrorCallback=void(*)(AAudioStream*,void*,aaudio_result_t);
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder**);
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder*,int);
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder*,int);
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder*,int);
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder*,int);
void AAudioStreamBuilder_setSharingMode(AAudioStreamBuilder*,int);
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder*,int);
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder*,DataCallback,void*);
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder*,ErrorCallback,void*);
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder*,AAudioStream**);
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder*);
int AAudioStream_getFormat(AAudioStream*);
int AAudioStream_getChannelCount(AAudioStream*);
int AAudioStream_getSampleRate(AAudioStream*);
int AAudioStream_getFramesPerBurst(AAudioStream*);
int AAudioStream_setBufferSizeInFrames(AAudioStream*,int);
int AAudioStream_requestStart(AAudioStream*);
int AAudioStream_requestStop(AAudioStream*);
int AAudioStream_close(AAudioStream*);
int AAudioStream_getXRunCount(AAudioStream*);
int AAudioStream_getState(AAudioStream*);
