#include "globe.h"
#include <jni.h>
#include <android/native_window_jni.h>
#include <android/asset_manager_jni.h>
#include <mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <string>
#include <cstdio>
#include <android/log.h>

namespace {
std::mutex gate;
std::thread worker;
std::atomic<bool> running{false};
std::atomic<int> glass{24};
std::atomic<float> mergeWidth{.32f},speed{1.f},yaw{.32f},pitch{.18f};
std::string status="Native Vulkan not started";
std::mutex statusMutex;
void setStatus(const char* msg){std::lock_guard<std::mutex> guard(statusMutex);status=msg;
    __android_log_print(ANDROID_LOG_INFO,"LiquidGlass","%s",msg);}
void stopWorker(){running=false;if(worker.joinable())worker.join();}
}
void Engine::setMessage(const char* msg){setStatus(msg);}
extern "C" JNIEXPORT void JNICALL
Java_com_example_liquidglass_GlobeActivity_nativeStop(JNIEnv*,jclass){
    std::lock_guard<std::mutex> guard(gate);stopWorker();
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_liquidglass_GlobeActivity_nativeStart(JNIEnv* env,jclass,jobject surface,jobject assetManager){
    std::lock_guard<std::mutex> guard(gate);
    stopWorker();
    ANativeWindow *window=ANativeWindow_fromSurface(env,surface);
    AAssetManager *assets=AAssetManager_fromJava(env,assetManager);
    if(!window||!assets){if(window)ANativeWindow_release(window);setStatus("Surface or assets unavailable");return;}
    running=true;
    worker=std::thread([window,assets]{
        Engine engine;engine.window=window;engine.assets=assets;
        if(!engine.init()){engine.shutdown();ANativeWindow_release(window);running=false;return;}
        int lastCount=-1;
        auto previous=std::chrono::steady_clock::now();
        setStatus("VULKAN ACTIVE · Native GPU");
        while(running){
            int count=glass.load();
            if(lastCount!=count){engine.setupParticles(count);lastCount=count;}
            auto now=std::chrono::steady_clock::now();
            float dt=std::chrono::duration<float>(now-previous).count();previous=now;
            dt=std::min(.045f,dt)*speed.load();
            if(dt>0){for(int i=0;i<3;++i)engine.physics(dt/3.f);engine.simClock+=dt;}
            Uniforms u{};engine.prepareUniform(u,yaw.load(),pitch.load(),mergeWidth.load(),count);
            if(engine.mapped)std::memcpy(engine.mapped,&u,sizeof(u));
            if(!engine.draw())break;
            std::this_thread::sleep_for(std::chrono::milliseconds(7));
        }
        engine.shutdown();ANativeWindow_release(window);
        if(running)setStatus("Vulkan swapchain stopped");
    });
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_liquidglass_GlobeActivity_nativeSettings(JNIEnv*,jclass,jint count,jfloat blend,jfloat movement,jfloat azimuth,jfloat elevation){
    glass=std::max(0,std::min(64,int(count)));
    mergeWidth=std::max(.02f,std::min(.65f,float(blend)));
    speed=std::max(0.f,std::min(2.5f,float(movement)));
    yaw=azimuth;pitch=elevation;
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_liquidglass_GlobeActivity_nativeStatus(JNIEnv* env,jclass){
    std::lock_guard<std::mutex> guard(statusMutex);
    return env->NewStringUTF(status.c_str());
}
