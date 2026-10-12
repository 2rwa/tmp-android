#pragma once
namespace odaiko {
struct Strike {double x,y,strength;int art;double shape[384];};
void initialize(int sampleRate,int quality,double roomWet);
void prepare(Strike &h);
bool enqueue(const Strike &h);
int render(int frames);
const float* output();
int modeCount();
}