#include "globe.h"
#include <algorithm>
#include <cmath>
#include <cstring>

static float rnd(uint32_t& state,float minv,float maxv) {
    state ^= state<<13;state ^= state>>17;state ^= state<<5;
    return minv + (maxv-minv)*(state & 0xffffff)/16777215.f;
}
void Engine::setupParticles(int glassCount) {
    randomSeed=0x14142u;particles.clear();
    const int opaqueCount=7;
    for(int i=0;i<opaqueCount+glassCount;++i) {
        bool glass=i>=opaqueCount;
        float radius=glass?rnd(randomSeed,.18f,.25f):rnd(randomSeed,.30f,.39f);
        Body p{};p.r=radius;p.glass=glass;
        bool placed=false;
        for(int attempt=0;attempt<1000;++attempt){
            p.x=rnd(randomSeed,-1.83f,1.83f);
            p.y=rnd(randomSeed,-1.83f,1.83f);
            p.z=rnd(randomSeed,-1.83f,1.83f);
            if(std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z)>2.36f-radius)continue;
            bool hit=false;
            for(const auto &other:particles){
                if(glass&&other.glass)continue;
                float dx=p.x-other.x,dy=p.y-other.y,dz=p.z-other.z;
                float limit=p.r+other.r+.12f;
                if(dx*dx+dy*dy+dz*dz<limit*limit){hit=true;break;}
            }
            if(!hit){placed=true;break;}
        }
        if(!placed)continue;
        p.vx=rnd(randomSeed,-1.2f,1.2f);
        p.vy=rnd(randomSeed,-.8f,1.3f);
        p.vz=rnd(randomSeed,-1.2f,1.2f);
        particles.push_back(p);
    }
}
void Engine::physics(float dt) {
    if(dt<=0.f)return;
    for(auto &p:particles){
        p.vy-=.63f*dt;
        p.x+=p.vx*dt;p.y+=p.vy*dt;p.z+=p.vz*dt;
        float len=std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
        float limit=2.38f-p.r;
        if(len>limit){
            float nx=p.x/len,ny=p.y/len,nz=p.z/len;
            p.x=nx*limit;p.y=ny*limit;p.z=nz*limit;
            float vn=p.vx*nx+p.vy*ny+p.vz*nz;
            if(vn>0.f){p.vx-=1.94f*vn*nx;p.vy-=1.94f*vn*ny;p.vz-=1.94f*vn*nz;}
        }
    }
    for(int pass=0;pass<4;pass++)for(size_t i=0;i<particles.size();i++)for(size_t j=i+1;j<particles.size();j++){
        Body &a=particles[i], &b=particles[j];
        if(a.glass&&b.glass)continue;
        float dx=b.x-a.x,dy=b.y-a.y,dz=b.z-a.z;
        float dist=std::sqrt(dx*dx+dy*dy+dz*dz);
        float wanted=a.r+b.r+((a.glass!=b.glass)? .15f : .008f);
        if(dist>=wanted)continue;
        if(dist<1e-5f){dx=1.f;dy=dz=0.f;dist=1.f;}
        float nx=dx/dist,ny=dy/dist,nz=dz/dist;
        float ia=1.f/(a.r*a.r*a.r),ib=1.f/(b.r*b.r*b.r);
        float overlap=(wanted-dist)/(ia+ib);
        a.x-=nx*overlap*ia;a.y-=ny*overlap*ia;a.z-=nz*overlap*ia;
        b.x+=nx*overlap*ib;b.y+=ny*overlap*ib;b.z+=nz*overlap*ib;
        float rel=(b.vx-a.vx)*nx+(b.vy-a.vy)*ny+(b.vz-a.vz)*nz;
        if(rel<0){float impulse=-1.94f*rel/(ia+ib);
            a.vx-=impulse*ia*nx;a.vy-=impulse*ia*ny;a.vz-=impulse*ia*nz;
            b.vx+=impulse*ib*nx;b.vy+=impulse*ib*ny;b.vz+=impulse*ib*nz;
        }
    }
}
void Engine::prepareUniform(Uniforms& u,float yaw,float pitch,float blend,int glassCount){
    std::memset(&u,0,sizeof(u));
    u.camera={yaw,pitch,float(size.width)/float(std::max(1u,size.height)),2.51f};
    u.controls={blend,float(particles.size()),float(glassCount),simClock};
    for(size_t i=0;i<particles.size() && i<80;++i){
        const auto&p=particles[i];u.spheres[i]={p.x,p.y,p.z,p.glass?p.r:-p.r};
    }
}
