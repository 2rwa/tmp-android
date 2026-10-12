#include "../src/main/cpp/OdaikoProfile.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cstdlib>
static int fail(const char*s){std::fprintf(stderr,"FAIL: %s\n",s);return 1;}
int main(){
 for(int q=0;q<4;q++){
  odaiko::initialize(48000,q,.18);
  odaiko::Strike a{};a.x=.10;a.y=0;a.strength=.65;a.art=0;
  odaiko::prepare(a);if(!odaiko::enqueue(a))return fail("enqueue");
  double energy=0,maxAbs=0;int frames=48000;
  while(frames>0){int n=std::min(frames,128);if(odaiko::render(n)!=n)return fail("block");const float *p=odaiko::output();for(int j=0;j<n;j++){if(!std::isfinite(p[j]))return fail("nonfinite");energy+=double(p[j])*p[j];maxAbs=std::max(maxAbs,double(std::abs(p[j])));}frames-=n;}
  if(energy<1e-14)return fail("silent");std::printf("quality=%d modes=%d energy=%.12g peak=%.9g PASS\n",q,odaiko::modeCount(),energy,maxAbs);
 }
 // Verify KA/rim path and changing strike position do not silence the model.
 odaiko::initialize(44100,1,.18);
 for(int art=0;art<5;art++){odaiko::Strike h{};h.x=art%2?.72:.1;h.y=.2;h.strength=.8;h.art=art;odaiko::prepare(h);if(!odaiko::enqueue(h))return fail("art enqueue");for(int k=0;k<15;k++)if(odaiko::render(128)!=128)return fail("art render");}
 std::puts("all articulations PASS");return 0;
}
