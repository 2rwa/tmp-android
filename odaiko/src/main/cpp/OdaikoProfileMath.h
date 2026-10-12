#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "OdaikoZeros.h"
namespace odaiko {
constexpr double PI=3.14159265358979323846, TAU=2*PI, LN1000=6.907755278982137;
inline double clamp(double x,double lo,double hi){return std::max(lo,std::min(hi,x));}
struct Rng {
 uint32_t s;
 explicit Rng(uint32_t v):s(v?v:0x12345678u){}
 double next(){s^=s<<13;s^=s>>17;s^=s<<5;return double(s)/4294967296.0;}
 double gaussian(){return std::sqrt(-2*std::log(std::max(1e-9,next())))*std::cos(TAU*std::max(1e-9,next()));}
};
inline double j0(double x){
 double ax=std::abs(x),y,z,xx,a,b;
 if(ax<8){y=x*x;a=57568490574.+y*(-13362590354.+y*(651619640.7+y*(-11214424.18+y*(77392.33017+y*(-184.9052456)))));b=57568490411.+y*(1029532985.+y*(9494680.718+y*(59272.64853+y*(267.8532712+y))));return a/b;}
 z=8/ax;y=z*z;xx=ax-.785398164;
 a=1+y*(-.1098628627e-2+y*(.2734510407e-4+y*(-.2073370639e-5+y*.2093887211e-6)));
 b=-.1562499995e-1+y*(.1430488765e-3+y*(-.6911147651e-5+y*(.7621095161e-6-y*.934945152e-7)));
 return std::sqrt(.636619772/ax)*(std::cos(xx)*a-z*std::sin(xx)*b);
}
inline double j1(double x){
 double ax=std::abs(x),y,z,xx,a,b,v;
 if(ax<8){y=x*x;a=x*(72362614232.+y*(-7895059235.+y*(242396853.1+y*(-2972611.439+y*(15704.48260+y*(-30.16036606))))));b=144725228442.+y*(2300535178.+y*(18583304.74+y*(99447.43394+y*(376.9991397+y))));return a/b;}
 z=8/ax;y=z*z;xx=ax-2.356194491;
 a=1+y*(.183105e-2+y*(-.3516396496e-4+y*(.2457520174e-5+y*(-.240337019e-6))));
 b=.04687499995+y*(-.2002690873e-3+y*(.8449199096e-5+y*(-.88228987e-6+y*.105787412e-6)));
 v=std::sqrt(.636619772/ax)*(std::cos(xx)*a-z*std::sin(xx)*b);return x<0?-v:v;
}
inline double jn(int n,double x){
 if(n==0)return j0(x);if(n==1)return j1(x);double ax=std::abs(x);if(ax<1e-12)return 0;
 if(ax<n+8){double fact=1;for(int k=2;k<=n;k++)fact*=k;double t=std::pow(ax*.5,n)/fact,sum=t,xx=ax*ax*.25;
 for(int k=1;k<120;k++){t*= -xx/(k*(n+k));sum+=t;if(std::abs(t)<1e-16*std::max(1.,std::abs(sum)))break;}return x<0&&(n&1)?-sum:sum;}
 double prev=j0(x),v=j1(x);for(int k=1;k<n;k++){double w=2*k/x*v-prev;prev=v;v=w;}return v;
}
inline double radiation(int order,double kr,double th){
 double s=std::abs(std::sin(th)),phys=order==0?1/std::sqrt(1+.18*kr*kr*s*s):std::pow(s,std::min(4,order))*std::pow(kr/(1+kr),std::min(2,order));
 return 1+.6*(phys-1);
}
struct Osc{double a11,a12,a21,a22,inv;};
inline Osc osc(double w,double sigma,int sr){
 double dt=1./sr,e=std::exp(-sigma*dt),disc=w*w-sigma*sigma,c,phi;
 if(disc>1e-12){double wd=std::sqrt(disc);c=std::cos(wd*dt);phi=std::sin(wd*dt)/wd;}
 else if(disc< -1e-12){double d=std::sqrt(-disc);c=std::cosh(d*dt);phi=std::sinh(d*dt)/d;}
 else{c=1;phi=dt;}
 return {e*(c+sigma*phi),e*phi,e*(-w*w*phi),e*(c-sigma*phi),1/std::max(1e-12,w*w)};
}
struct Mode{int m,n,orient;double alpha,f,w,sigma,t60,mean,mass,var;};
inline std::vector<Mode> modes(bool front,int count){
 const double radius=.525, baseMu=2000*(front?.0015:.00145),tension=front?12800:13400;
 const double thick=front?.0015:.00145,young=3.5e9,stiff=front?.10:.09,split=front?.0035:.0030,low=front?2.15:2.28,slope=front?.82:.78;
 Rng rng(12345+(front?11:97)),splitRng(54321+(front?31:131));std::vector<Mode> all;all.reserve(544);
 for(const auto &z:kZeros){
  double perturb=clamp(1+.012*.55*rng.gaussian(),.82,1.18),mu=baseMu*perturb;
  double f=z.alpha/(TAU*radius)*std::sqrt(tension/mu);
  double bendD=young*std::pow(thick*perturb,3)/(12*(1-.30*.30));
  f*=std::sqrt(1+stiff*(bendD*z.alpha*z.alpha/(tension*radius*radius)))*(1-.040*.025-.025*.10);
  double randT=.012*.5*rng.gaussian(),tack=.018*.16*(z.m+1.)/(z.m+z.n+2.)*rng.gaussian();
  f*=std::sqrt(std::max(.75,1+randT))*(1+tack);
  if(f>8000*1.15)continue;
  for(int orient=0;orient<(z.m==0?1:2);orient++){
   double ff=f;if(z.m>0){double ss=split*(.45+.55*splitRng.next())*(1+.12*z.m);ff*=1+(orient==0?-.5:.5)*ss;double anis=front?.018:0;ff*=1+(orient==0?1:-1)*anis*.012*z.m/(z.m+1.);}
   double t60=clamp(((low/(1+1.8*slope*std::pow(ff/900,1.25)))+.018)/(1+1.7*.03+.7*.018),.018,8);
   double mean=z.m==0?2*j1(z.alpha)/z.alpha:0;
   double mass=mu*PI*radius*radius*clamp(.19+.035*z.n+.008*z.m,.20,.48);
   double variation=(1+.055*(2*rng.next()-1))*(1-.12*.018*z.m/(z.m+2.));
   if(ff<=8000)all.push_back({z.m,z.n,orient,z.alpha,ff,TAU*ff,LN1000/t60,t60,mean,mass,variation});
  }
 }
 std::stable_sort(all.begin(),all.end(),[](const Mode&a,const Mode&b){return a.f<b.f;});
 if(all.size()>size_t(count))all.resize(count);return all;
}
}
