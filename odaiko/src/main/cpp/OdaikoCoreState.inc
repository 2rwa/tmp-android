#include <stdint.h>
#include <stddef.h>

#define MAX_HEAD 384
#define MAX_SHELL 24
#define MAX_CAV 12
#define PITCH_STRIDE 480
#define MAX_EVENTS 512
#define MAX_CONTACTS 32
#define MAX_BLOCK 2048
#define ROOM_LINES 6
#define ROOM_MAX 8192
#define EARLY_MAX 8192
#define PRE_MAX 8192

#include <math.h>
static inline double js_pow(double a,double b){return pow(a,b);}
void *memcpy(void *dst,const void *src,size_t n){unsigned char *d=(unsigned char*)dst;const unsigned char *s=(const unsigned char*)src;for(size_t i=0;i<n;i++)d[i]=s[i];return dst;}
void *memset(void *dst,int c,size_t n){unsigned char *d=(unsigned char*)dst;for(size_t i=0;i<n;i++)d[i]=(unsigned char)c;return dst;}


static inline double dmin(double a,double b){return a<b?a:b;}
static inline double dmax(double a,double b){return a>b?a:b;}
static inline double clampd(double x,double a,double b){return dmax(a,dmin(b,x));}
static inline double absd(double x){return x<0?-x:x;}

typedef struct {
  double sampleRate, dt, area, volume, cavityBulk, cavityPressureAlpha, couplingScale;
  double nlAlpha, nlSafety, nlStrength, energyResponse;
  double reciprocalGain, shellModeGain, shellRadiationGain, floorCoupling;
  double contactK, contactExponent, contactLoss, contactMassDen;
  double bachiMass, rimDecay, hardness, baseTangential, distanceGain;
  double radiationFront, radiationRear, radiationSide, radiationBody, contactGain, cavityModeGain, masterGain;
  double cavityPressureFactor;
  double frontNorm,rearNorm,shellNorm,cavityNorm;
  double standA11,standA12,standA21,standA22,standInvOmega2;
  double floorA11,floorA12,floorA21,floorA22,floorInvOmega2;
  double roomWet,roomDamp,roomEarlyLevel;
} Config;
static Config cfg;
static int nFront=0,nRear=0,nShell=0,nCav=0,frontNonlinear=0,rearNonlinear=0;

#define DARR(name,N) static double name[N]
#define IARR(name,N) static int32_t name[N]
DARR(fq,MAX_HEAD); DARR(fv,MAX_HEAD); DARR(fshape,MAX_HEAD); DARR(fmic,MAX_HEAD); DARR(fside,MAX_HEAD); DARR(fboundary,MAX_HEAD);
DARR(fev,MAX_HEAD); DARR(feq,MAX_HEAD); DARR(fmean,MAX_HEAD); DARR(finv,MAX_HEAD); DARR(fa11,MAX_HEAD); DARR(fa12,MAX_HEAD); DARR(fa21,MAX_HEAD); DARR(fa22,MAX_HEAD); DARR(finvw2,MAX_HEAD); DARR(fsidefactor,MAX_HEAD); DARR(ffastcav,MAX_HEAD); DARR(ffastboundary,MAX_HEAD); DARR(ffastcross,MAX_HEAD); IARR(fm,MAX_HEAD); DARR(fpitch,MAX_HEAD*PITCH_STRIDE);
DARR(rq,MAX_HEAD); DARR(rv,MAX_HEAD); DARR(rmic,MAX_HEAD); DARR(rside,MAX_HEAD); DARR(rboundary,MAX_HEAD);
DARR(revscale,MAX_HEAD); DARR(reqscale,MAX_HEAD); DARR(rmean,MAX_HEAD); DARR(rinv,MAX_HEAD); DARR(ra11,MAX_HEAD); DARR(ra12,MAX_HEAD); DARR(ra21,MAX_HEAD); DARR(ra22,MAX_HEAD); DARR(rinvw2,MAX_HEAD); DARR(rsidefactor,MAX_HEAD); DARR(rfastcav,MAX_HEAD); DARR(rfastboundary,MAX_HEAD); IARR(rm,MAX_HEAD); DARR(rpitch,MAX_HEAD*PITCH_STRIDE);
DARR(sq,MAX_SHELL); DARR(sv,MAX_SHELL); DARR(sgain,MAX_SHELL); DARR(sa11,MAX_SHELL); DARR(sa12,MAX_SHELL); DARR(sa21,MAX_SHELL); DARR(sa22,MAX_SHELL); DARR(sinvw2,MAX_SHELL);
DARR(cq,MAX_CAV); DARR(cv,MAX_CAV); DARR(comega,MAX_CAV); DARR(cgain,MAX_CAV); DARR(cfrontsign,MAX_CAV); DARR(crearsign,MAX_CAV); DARR(coverlap,MAX_CAV); DARR(ca11,MAX_CAV); DARR(ca12,MAX_CAV); DARR(ca21,MAX_CAV); DARR(ca22,MAX_CAV); DARR(cinvw2,MAX_CAV);

static float outDry[MAX_BLOCK],outWet[MAX_BLOCK],outFront[MAX_BLOCK],outRear[MAX_BLOCK],outBody[MAX_BLOCK],outFrontState[MAX_BLOCK],outRearState[MAX_BLOCK],outShellState[MAX_BLOCK];

typedef struct { int32_t targetFrame,kind; double velocity,contactScale,transientScale,rimScale,relativeVelocity,tangential; } Event;
static Event events[MAX_EVENTS]; static double eventShapes[MAX_EVENTS*MAX_HEAD]; static int eventHead=0,eventTail=0,eventCount=0; static int32_t currentFrame=0;
typedef struct { double z,v,lastForce,age,maxForce,impulse,transientScale,contactScale,relativeVelocity,tangential; int ever,contactSamples; } Contact;
static Contact contacts[MAX_CONTACTS]; static int activeCount=0;

static uint32_t rngState=0x12345678u; static double noiseLP=0,nlEnvFront=0,nlEnvRear=0,cavityPressureLP=0,rimTransientEnv=0,standQ=0,standV=0,floorQ=0,floorV=0,peakDry=0;

static int roomEnabled=0,roomDelay[ROOM_LINES],roomIdx[ROOM_LINES],roomPreDelay=0,roomPreIdx=0,roomEarlyDelay[4],roomEarlyLen=1,roomEarlyIdx=0;
static double roomGain[ROOM_LINES],roomLp[ROOM_LINES];
static float roomBuf[ROOM_LINES][ROOM_MAX],roomPre[PRE_MAX],roomEarly[EARLY_MAX];

uintptr_t config_ptr(void){return (uintptr_t)&cfg;}
uintptr_t array_ptr(int id){
 switch(id){
#define CASE(x,n) case n:return (uintptr_t)(x)
 CASE(fq,1);CASE(fv,2);CASE(fshape,3);CASE(fmic,4);CASE(fside,5);CASE(fboundary,6);CASE(fev,7);CASE(feq,8);CASE(fmean,9);CASE(finv,10);CASE(fa11,11);CASE(fa12,12);CASE(fa21,13);CASE(fa22,14);CASE(finvw2,15);CASE(fsidefactor,16);CASE(ffastcav,17);CASE(ffastboundary,18);CASE(ffastcross,19);CASE(fm,20);CASE(fpitch,21);
 CASE(rq,31);CASE(rv,32);CASE(rmic,33);CASE(rside,34);CASE(rboundary,35);CASE(revscale,36);CASE(reqscale,37);CASE(rmean,38);CASE(rinv,39);CASE(ra11,40);CASE(ra12,41);CASE(ra21,42);CASE(ra22,43);CASE(rinvw2,44);CASE(rsidefactor,45);CASE(rfastcav,46);CASE(rfastboundary,47);CASE(rm,48);CASE(rpitch,49);
 CASE(sq,61);CASE(sv,62);CASE(sgain,63);CASE(sa11,64);CASE(sa12,65);CASE(sa21,66);CASE(sa22,67);CASE(sinvw2,68);
 CASE(cq,71);CASE(cv,72);CASE(comega,73);CASE(cgain,74);CASE(cfrontsign,75);CASE(crearsign,76);CASE(coverlap,77);CASE(ca11,78);CASE(ca12,79);CASE(ca21,80);CASE(ca22,81);CASE(cinvw2,82);
 CASE(outDry,101);CASE(outWet,102);CASE(outFront,103);CASE(outRear,104);CASE(outBody,105);CASE(outFrontState,106);CASE(outRearState,107);CASE(outShellState,108);
 default:return 0;
 }
}
void set_counts(int nf,int nr,int ns,int nc,int fnl,int rnl){nFront=nf;nRear=nr;nShell=ns;nCav=nc;frontNonlinear=fnl;rearNonlinear=rnl;}
void set_rng(uint32_t s){rngState=s?s:0x12345678u;}
int room_config(int enabled,int predelay,int earlyLen,int d0,int d1,int d2,int d3,int d4,int d5,int e0,int e1,int e2,int e3){
 roomEnabled=enabled;roomPreDelay=predelay;roomEarlyLen=earlyLen;if(predelay>=PRE_MAX||earlyLen>=EARLY_MAX)return 0;int d[6]={d0,d1,d2,d3,d4,d5};for(int i=0;i<6;i++){if(d[i]>=ROOM_MAX)return 0;roomDelay[i]=d[i];roomIdx[i]=0;roomLp[i]=0;}roomEarlyDelay[0]=e0;roomEarlyDelay[1]=e1;roomEarlyDelay[2]=e2;roomEarlyDelay[3]=e3;return 1;
}
uintptr_t room_gain_ptr(void){return (uintptr_t)roomGain;}
void reset_state(void){
 for(int i=0;i<nFront;i++){fq[i]=fv[i]=0;}for(int i=0;i<nRear;i++){rq[i]=rv[i]=0;}for(int i=0;i<nShell;i++){sq[i]=sv[i]=0;}for(int i=0;i<nCav;i++){cq[i]=cv[i]=0;}
 currentFrame=0;eventHead=eventTail=eventCount=activeCount=0;noiseLP=nlEnvFront=nlEnvRear=cavityPressureLP=rimTransientEnv=standQ=standV=floorQ=floorV=peakDry=0;roomPreIdx=roomEarlyIdx=0;
 for(int k=0;k<6;k++){roomIdx[k]=0;roomLp[k]=0;for(int i=0;i<roomDelay[k]&&i<ROOM_MAX;i++)roomBuf[k][i]=0;}for(int i=0;i<=roomPreDelay&&i<PRE_MAX;i++)roomPre[i]=0;for(int i=0;i<roomEarlyLen&&i<EARLY_MAX;i++)roomEarly[i]=0;
}
int enqueue_event(int targetFrame,int kind,double velocity,double contactScale,double transientScale,double rimScale,double relativeVelocity,double tangential){
 if(eventCount>=MAX_EVENTS)return -1;if(eventCount>0){int last=(eventTail+MAX_EVENTS-1)%MAX_EVENTS;if(targetFrame<events[last].targetFrame)return -2;}int slot=eventTail;events[slot]=(Event){targetFrame,kind,velocity,contactScale,transientScale,rimScale,relativeVelocity,tangential};eventTail=(eventTail+1)%MAX_EVENTS;eventCount++;return slot;
}
uintptr_t event_shape_ptr(int slot){if(slot<0||slot>=MAX_EVENTS)return 0;return (uintptr_t)&eventShapes[slot*MAX_HEAD];}

