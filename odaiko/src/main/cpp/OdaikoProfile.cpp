#include "OdaikoProfileMath.h"
#include "OdaikoProfile.h"
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>
extern "C" {
 uintptr_t config_ptr(void);
 uintptr_t array_ptr(int);
 void set_counts(int,int,int,int,int,int);
 void set_rng(uint32_t);
 int room_config(int,int,int,int,int,int,int,int,int,int,int,int,int);
 uintptr_t room_gain_ptr(void);
 void reset_state(void);
 int enqueue_event(int,int,double,double,double,double,double,double);
 uintptr_t event_shape_ptr(int);
 int process_quantum(int);
 int get_current_frame(void);
}
namespace odaiko {
static std::vector<Mode> front, rear;
static int sr=48000;
static double wet=.18;
static inline double *a(int id){return reinterpret_cast<double*>(array_ptr(id));}
static inline int32_t *ai(int id){return reinterpret_cast<int32_t*>(array_ptr(id));}
static int countNL(const std::vector<Mode>& mm){int n=0;for(size_t i=0;i<mm.size();i++)if(mm[i].f<=650)n=int(i+1);return n;}
static void fillHead(const std::vector<Mode>& modes,bool f){
 const int mic=f?4:33,side=f?5:34,bound=f?6:35,ev=f?7:36,eq=f?8:37,mean=f?9:38,inv=f?10:39,ca11=f?11:40,ca12=f?12:41,ca21=f?13:42,ca22=f?14:43,invw=f?15:44,factor=f?16:45,cav=f?17:46,bndry=f?18:47,mc=f?20:48,pitch=f?21:49;
 const double R=.525,area=PI*R*R,speed=343,mdist=1.5,absorption=.08,cavAxis=.012;
 for(size_t i=0;i<modes.size();i++){
  const auto&m=modes[i];int j=int(i);Osc v=osc(m.w,m.sigma,sr);
  double orderPenalty=1/(1+.040*m.m*m.m),freqRad=.48+.52*(1-std::exp(-m.f/420));
  double airLoss=std::exp(-absorption*mdist*std::pow(m.f/6000,1.35));
  double kr=TAU*m.f*R/speed,ang=f?0:PI;
  a(mic)[j]=orderPenalty*freqRad*airLoss*radiation(m.m,kr,ang);
  a(side)[j]=0; // micAngleDeg 0: side radiation is zero by definition
  a(bound)[j]=clamp(.25+.75*m.alpha/(m.alpha+8+m.m),.20,.95);
  a(ev)[j]=.5*m.mass;a(eq)[j]=.5*m.mass*m.w*m.w;
  a(mean)[j]=m.mean;a(inv)[j]=1/m.mass;
  a(ca11)[j]=v.a11;a(ca12)[j]=v.a12;a(ca21)[j]=v.a21;a(ca22)[j]=v.a22;a(invw)[j]=v.inv;
  a(factor)[j]=m.m>0?double(m.m)/(m.m+2):0;
  double weight=m.m==0?m.mean:cavAxis*(m.orient==0?1.:-1.)/(1+m.m);
  a(cav)[j]=-area*weight/m.mass;
  a(bndry)[j]=a(bound)[j]*m.w*(f?.004:.0035);
  ai(mc)[j]=m.m;
  if(f)a(19)[j]=.008*m.w*.018;
  if(m.f<=650)for(int n=0;n<96;n++){
   double scale=1+.35*n/95.,om=m.w*scale;Osc p=osc(om,m.sigma,sr);
   int off=j*480+n*5;a(pitch)[off]=p.a11;a(pitch)[off+1]=p.a12;a(pitch)[off+2]=p.a21;a(pitch)[off+3]=p.a22;a(pitch)[off+4]=p.inv;
  }
 }
}
static void fillShell(int N){
 const double R=.56,th=.045,rho=690,ec=1.15e9,ref=std::sqrt(ec/rho)/(TAU*R)*.11;
 const double ratios[]={1,1.34,1.72,2.18,2.78,3.52,4.46,5.55,6.8,8.2,9.8,11.6,13.6,15.8,18.2,20.8,23.6,26.6,29.8,33.2,36.8,40.6,44.6,48.8};
 for(int i=0;i<N;i++){
  double split=1+(i%2==0?-.5:.5)*.012;
  double f=clamp(ref*ratios[i]*(1+.04*th/R)*split,45,5000);
  double t60=clamp((.72/(1+.25*i)+.035)/(1+1.2*.22),.035,1.3);
  Osc c=osc(TAU*f,LN1000/std::max(.025,t60),sr);
  a(63)[i]=1/(1+.16*i);a(64)[i]=c.a11;a(65)[i]=c.a12;a(66)[i]=c.a21;a(67)[i]=c.a22;a(68)[i]=c.inv;
 }
}
static void fillCavity(int N){
 const double L=1.22,R=.56,speed=343;
 for(int i=0;i<N;i++){
  int axial=1+i/3,radial=i%3;double kz=axial/L,kr=(radial==0?0:(radial==1?.586:1.05))/R;
  double f=clamp(.5*speed*std::sqrt(kz*kz+kr*kr)*(.96+.08*.58),70,4500);
  double t60=clamp(.50/(1+.18*i)/(1+2.2*.35)+.025,.03,.9);
  Osc c=osc(TAU*f,LN1000/t60,sr);
  a(73)[i]=TAU*f;a(74)[i]=1/(1+.22*i);a(75)[i]=1;
  a(76)[i]=axial%2==0?1:-1;a(77)[i]=1/(1+.14*i+.18*radial);
  a(78)[i]=c.a11;a(79)[i]=c.a12;a(80)[i]=c.a21;a(81)[i]=c.a22;a(82)[i]=c.inv;
 }
}
void initialize(int sampleRate,int quality,double roomWet){
 sr=sampleRate;wet=clamp(roomWet,0,.40);
 const int heads[]={64,128,240,384},shells[]={4,8,16,24},cavs[]={2,4,8,12};
 quality=std::max(0,std::min(3,quality));
 front=modes(true,heads[quality]);rear=modes(false,heads[quality]);
 int nf=int(front.size()),nr=int(rear.size()),ns=shells[quality],nc=cavs[quality];
 set_counts(nf,nr,ns,nc,countNL(front),countNL(rear));set_rng(24681357);
 fillHead(front,true);fillHead(rear,false);fillShell(ns);fillCavity(nc);
 const double dt=1.0/sr,R=.525,area=PI*R*R,V=PI*.56*.56*1.22*(.82+.18*.58);
 const double bulk=1.2*343*343*.18, effectiveK=2.1e7, hardness=clamp((std::log10(effectiveK)-6.5)/1.7,0,1);
 const double standOmega=std::sqrt(4.5e6/45.),standT60=clamp(1.15*(1-.38)+.09,.08,1.2);
 Osc stand=osc(standOmega,LN1000/standT60,sr);
 Osc floor=osc(TAU*43,LN1000/std::max(.04,.55*(1-.42*.65)),sr);
 const double vals[]={
  double(sr),dt,area,V,bulk,clamp(dt*3500*(1-.10*.75),0,1),1.,
  clamp(dt/(.42*.08),0,1), (1-.92)*.7,.055,1.,
  .10,.18,.20,.16,effectiveK,1.5*(.94+.12*.72),.10+.75*.06,
  std::max(.03,.31*std::sqrt(.54/.52)),.31,std::exp(-dt/.0032),hardness,0.,
  1/1.5,1.,.48,.12,.16,0.,.08,.060,
  1.2*343*343*.08,
  1/std::sqrt(nf),1/std::sqrt(nr),1/std::sqrt(ns),1/std::sqrt(nc),
  stand.a11,stand.a12,stand.a21,stand.a22,stand.inv,
  floor.a11,floor.a12,floor.a21,floor.a22,floor.inv,
  wet,.38,.24
 };
 std::copy(std::begin(vals),std::end(vals),reinterpret_cast<double*>(config_ptr()));
 const int pred=int(.018*sr),early[]={std::max(1,int((.011*.72+.018)*sr)),std::max(1,int((.019*.72+.018)*sr)),std::max(1,int((.029*.72+.018)*sr)),std::max(1,int((.043*.72+.018)*sr))};
 const int earlyLen=early[3]+2;
 const double times[]={.0371,.0411,.0437,.0479,.0533,.0599};
 int d[6];for(int i=0;i<6;i++)d[i]=std::max(17,int(times[i]*.72*sr));
 room_config(wet>1e-5?1:0,pred,earlyLen,d[0],d[1],d[2],d[3],d[4],d[5],early[0],early[1],early[2],early[3]);
 auto gain=reinterpret_cast<double*>(room_gain_ptr());for(int i=0;i<6;i++)gain[i]=std::pow(10.,-3.*(d[i]/double(sr))/(1.55*1.225));
 reset_state();
}
void prepare(Strike &h){
 double r=clamp(std::hypot(h.x,h.y),0,.96),theta=std::atan2(h.y,h.x);
 const double spreadScales[]={1,1.16,.92,.62,1};
 double scale=spreadScales[std::max(0,std::min(4,h.art))];
 double tip=.5*(.020+.5*.040),contactSpread=.020*std::pow(std::max(.4,tip/.019),.45)*(1.14+(.88-1.14)*.72);
 double spread=clamp(contactSpread*scale/.525,.002,.30);
 for(size_t i=0;i<front.size();i++){
  auto&m=front[i];double radial=jn(m.m,m.alpha*r),shape=m.m==0?radial:radial*(m.orient==0?std::cos(m.m*theta):std::sin(m.m*theta));
  h.shape[i]=shape*std::exp(-.5*std::pow(m.alpha*spread,2))*m.var;
 }
}
bool enqueue(const Strike &h){
 const double stiff[]={1,.82,1.03,1.38,1},transient[]={1,.62,1.08,2.45,1.20},angle[]={0,0,0,26,8};
 int art=std::max(0,std::min(4,h.art));
 double velocity=1.8+5.9*std::pow(clamp(h.strength,0,1),1.18);
 double rel=std::max(.1,velocity*std::max(.12,std::cos(angle[art]*PI/180))*std::sqrt(.54/.52));
 int frame=get_current_frame();int slot=enqueue_event(frame,art==4?1:0,velocity,stiff[art],transient[art],1.,rel,std::abs(std::sin(angle[art]*PI/180)));
 if(slot<0)return false;
 if(art!=4)std::copy(h.shape,h.shape+front.size(),reinterpret_cast<double*>(event_shape_ptr(slot)));
 return true;
}
int render(int n){return process_quantum(n);}
const float* output(){return reinterpret_cast<const float*>(array_ptr(102));}
int modeCount(){return int(front.size());}
}
