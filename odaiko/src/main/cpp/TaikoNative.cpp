#include <jni.h>
#include <oboe/Oboe.h>
#include <atomic>
#include <memory>
#include <cmath>
#include <algorithm>
#include "OdaikoProfile.h"
namespace {
constexpr uint32_t QSIZE=128;
class Engine : public oboe::AudioStreamDataCallback {
public:
 std::shared_ptr<oboe::AudioStream> stream;
 odaiko::Strike queue[QSIZE]{};
 std::atomic<uint32_t> head{0},tail{0};
 std::atomic<uint64_t> drops{0},callbacks{0},frames{0};
 float gain=6.30957f;
 bool running=false;
 oboe::DataCallbackResult onAudioReady(oboe::AudioStream*,void* data,int32_t numFrames) override{
  float *dst=static_cast<float*>(data);
  uint32_t h=head.load(std::memory_order_relaxed);
  uint32_t t=tail.load(std::memory_order_acquire);
  while(h!=t){if(!odaiko::enqueue(queue[h%QSIZE]))drops.fetch_add(1,std::memory_order_relaxed);h++;}
  head.store(h,std::memory_order_release);
  int cursor=0;
  while(cursor<numFrames){
   int n=std::min(128,int(numFrames)-cursor);
   int ok=odaiko::render(n);
   const float *src=odaiko::output();
   for(int i=0;i<n;i++){
    float s=ok==n?src[i]*gain:0.f;
    if(!std::isfinite(s))s=0;
    s=std::max(-.89125f,std::min(.89125f,s));
    dst[2*(cursor+i)]=s;dst[2*(cursor+i)+1]=s;
   }
   cursor+=n;
  }
  callbacks.fetch_add(1,std::memory_order_relaxed);
  frames.fetch_add(numFrames,std::memory_order_relaxed);
  return oboe::DataCallbackResult::Continue;
 }
 bool hit(double x,double y,double strength,int art){
  uint32_t t=tail.load(std::memory_order_relaxed),h=head.load(std::memory_order_acquire);
  if(t-h>=QSIZE){drops++;return false;}
  auto &s=queue[t%QSIZE];s.x=x;s.y=y;s.strength=strength;s.art=art;
  odaiko::prepare(s); // non-realtime thread; the audio callback only copies
  tail.store(t+1,std::memory_order_release);
  return true;
 }
 void stop(){
  if(stream){stream->requestStop();stream->close();stream.reset();}
  running=false;
  head.store(0);tail.store(0);
 }
 int start(int quality,int wetPercent){
  stop();
  oboe::AudioStreamBuilder b;
  b.setDirection(oboe::Direction::Output);
  b.setPerformanceMode(oboe::PerformanceMode::LowLatency);
  b.setSharingMode(oboe::SharingMode::Exclusive);
  b.setFormat(oboe::AudioFormat::Float);
  b.setChannelCount(2);
  b.setDataCallback(this);
  if(b.openStream(stream)!=oboe::Result::OK)return -1;
  int sr=stream->getSampleRate();
  if(sr<=0){stop();return -2;}
  odaiko::initialize(sr,quality,wetPercent*.01);
  int burst=stream->getFramesPerBurst();
  if(burst>0)stream->setBufferSizeInFrames(burst*2);
  frames.store(0);callbacks.store(0);drops.store(0);head.store(0);tail.store(0);
  if(stream->requestStart()!=oboe::Result::OK){stop();return -3;}
  running=true;return sr;
 }
};
Engine engine;
}
extern "C" JNIEXPORT jint JNICALL Java_com_example_odaiko_TaikoActivity_nativeStart(JNIEnv*,jclass,jint quality,jint wet){return engine.start(quality,wet);}
extern "C" JNIEXPORT void JNICALL Java_com_example_odaiko_TaikoActivity_nativeStop(JNIEnv*,jclass){engine.stop();}
extern "C" JNIEXPORT jboolean JNICALL Java_com_example_odaiko_TaikoActivity_nativeHit(JNIEnv*,jclass,jfloat x,jfloat y,jfloat strength,jint art){return engine.running&&engine.hit(x,y,strength,art)?JNI_TRUE:JNI_FALSE;}
extern "C" JNIEXPORT void JNICALL Java_com_example_odaiko_TaikoActivity_nativeGain(JNIEnv*,jclass,jfloat db){engine.gain=std::pow(10.f,db/20.f);}
extern "C" JNIEXPORT jlongArray JNICALL Java_com_example_odaiko_TaikoActivity_nativeStats(JNIEnv* env,jclass){
 jlong a[]={jlong(engine.frames.load()),jlong(engine.callbacks.load()),jlong(engine.drops.load()),jlong(odaiko::modeCount())};
 jlongArray out=env->NewLongArray(4);env->SetLongArrayRegion(out,0,4,a);return out;
}
