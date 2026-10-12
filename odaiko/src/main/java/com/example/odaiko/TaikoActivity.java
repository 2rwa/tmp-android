package com.example.odaiko;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.graphics.Color;
import android.view.View;
import android.view.WindowManager;
import android.widget.*;
import android.content.SharedPreferences;
import java.util.HashMap;
import java.util.Map;

public final class TaikoActivity extends Activity {
 static {System.loadLibrary("odaiko");}
 static native int nativeStart(int quality,int wet);
 static native void nativeStop();
 static native boolean nativeHit(float x,float y,float strength,int articulation);
 static native void nativeGain(float db);
 static native long[] nativeStats();

 final Handler handler=new Handler(Looper.getMainLooper());
 final Map<Integer, Touch> touches=new HashMap<>();
 public int articulation=0;
 int quality=1,wet=18,gain=16,windowMs=12,mode=0;
 float fixed=.65f,rail=.65f;
 boolean railHeld=false,autoRim=true,playing=false;
 private TextView status,detail;private LinearLayout root,performance,settings;private DrumView drum;
 private final Button[] arts=new Button[5];
 int calibrationIndex=-1;
 private final float[][] calibrationSamples=new float[8][2];
 float pLo=0,pHi=0,cLo=0,cHi=0;private boolean pValid=false,cValid=false;
 SharedPreferences prefs;
 private static final int TEXT=Color.rgb(248,234,211),BG=Color.rgb(16,12,10),PANEL=Color.rgb(44,31,23),GOLD=Color.rgb(214,163,88);
 static float clamp(float x,float lo,float hi){return Math.max(lo,Math.min(hi,x));}
 int dp(float value){return (int)(value*getResources().getDisplayMetrics().density+.5f);}
 static class Touch{
  final int id;float x,y,r,pressure,area;boolean fired=false;Runnable scheduled;
  Touch(int i,float x,float y,float r,float p,float a){id=i;this.x=x;this.y=y;this.r=r;pressure=p;area=a;}
 }
 @Override public void onCreate(Bundle b){
  super.onCreate(b);
  getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
  prefs=getSharedPreferences("odaiko-cal-v1",0);pLo=prefs.getFloat("plo",0);pHi=prefs.getFloat("phi",0);cLo=prefs.getFloat("clo",0);cHi=prefs.getFloat("chi",0);
  pValid=prefs.getBoolean("pv",false);cValid=prefs.getBoolean("cv",false);
  root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setBackgroundColor(BG);root.setPadding(dp(6),dp(6),dp(6),dp(6));setContentView(root);
  LinearLayout top=new LinearLayout(this);top.setGravity(android.view.Gravity.CENTER_VERTICAL);root.addView(top,new LinearLayout.LayoutParams(-1,dp(50)));
  TextView label=text("WADAIKO",19);label.setTypeface(null,1);top.addView(label,new LinearLayout.LayoutParams(0,-2,1));
  Button start=button("START");top.addView(start);start.setOnClickListener(v->{if(playing){nativeStop();playing=false;start.setText("START");}else{int sr=nativeStart(quality,wet);playing=sr>0;start.setText(playing?"STOP":"RETRY");nativeGain(gain);showMessage(playing?"Audio "+sr+" Hz":"Audio start error "+sr);}updateStatus();});
  Button panic=button("■");top.addView(panic);panic.setOnClickListener(v->{nativeStop();playing=false;start.setText("START");showMessage("SILENT");});
  Button cog=button("⚙");top.addView(cog);cog.setOnClickListener(v->toggleSettings());
  status=text("Audio OFF · tap START",12);root.addView(status);
  performance=new LinearLayout(this);performance.setOrientation(LinearLayout.VERTICAL);root.addView(performance,new LinearLayout.LayoutParams(-1,0,1));
  LinearLayout stage=new LinearLayout(this);performance.addView(stage,new LinearLayout.LayoutParams(-1,0,1));
  drum=new DrumView(this,this);stage.addView(drum,new LinearLayout.LayoutParams(0,-1,1));
  StrengthRail railView=new StrengthRail(this,this);stage.addView(railView,new LinearLayout.LayoutParams(dp(58),-1));
  LinearLayout bar=new LinearLayout(this);performance.addView(bar,new LinearLayout.LayoutParams(-1,dp(52)));
  String[] names={"DON","TSU","EDGE","SLAP","KA"};
  for(int i=0;i<5;i++){final int n=i;arts[i]=button(names[i]);bar.addView(arts[i],new LinearLayout.LayoutParams(0,-1,1));arts[i].setTextSize(11);arts[i].setPadding(0,0,0,0);arts[i].setOnClickListener(v->{articulation=n;updateArt();});}
  updateArt();
  detail=text("Strength 65% · v 5.35m/s · touches 0",11);performance.addView(detail);
  ScrollView scroll=new ScrollView(this);root.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
  settings=new LinearLayout(this);settings.setOrientation(LinearLayout.VERTICAL);scroll.addView(settings);
  // The entire settings scroll container must be toggled, not only its content.
  settings.setTag(scroll);scroll.setVisibility(View.GONE);
  // Keep the reference to the visible ancestor for the menu toggle.
  settingsParent=scroll;
  createSettings();
  handler.postDelayed(statsTick,800);
 }
 private ScrollView settingsParent;
 void toggleSettings(){boolean open=settingsParent.getVisibility()!=View.VISIBLE;settingsParent.setVisibility(open?View.VISIBLE:View.GONE);performance.setVisibility(open?View.GONE:View.VISIBLE);}
 TextView text(String value,int size){TextView t=new TextView(this);t.setText(value);t.setTextColor(TEXT);t.setTextSize(size);t.setPadding(dp(4),dp(6),dp(4),dp(6));return t;}
 Button button(String value){Button b=new Button(this);b.setAllCaps(false);b.setText(value);b.setTextColor(TEXT);b.setTextSize(12);b.setBackgroundTintList(android.content.res.ColorStateList.valueOf(PANEL));return b;}
 void showMessage(String s){status.setText(s);}
 void updateArt(){for(int i=0;i<5;i++)if(arts[i]!=null)arts[i].setBackgroundTintList(android.content.res.ColorStateList.valueOf(i==articulation?GOLD:PANEL));}
 void createSettings(){new TaikoSettings(this,settings).create();}
 public void setRail(float value,boolean held){rail=clamp(value,.1f,1.f);railHeld=held;updateStatus();}
 public void touchDown(int id,float x,float y,float r,float p,float area){
  if(r>1)return;Touch t=new Touch(id,x,y,r,p,area);touches.put(id,t);
  if(windowMs==0)fire(t);else {t.scheduled=()->fire(t);handler.postDelayed(t.scheduled,windowMs);}
  updateStatus();
 }
 public void touchMove(int id,float p,float a){Touch t=touches.get(id);if(t!=null&&!t.fired){t.pressure=Math.max(t.pressure,p);t.area=Math.max(t.area,a);}}
 public void touchUp(int id){Touch t=touches.remove(id);if(t!=null){if(t.scheduled!=null)handler.removeCallbacks(t.scheduled);if(!t.fired)fire(t);}updateStatus();}
 public void touchCancel(int id){Touch t=touches.remove(id);if(t!=null&&t.scheduled!=null)handler.removeCallbacks(t.scheduled);updateStatus();}
 void fire(Touch t){
  if(t.fired)return;t.fired=true;
  float strength=fixed;
  if(calibrationIndex>=0){strength=calibrationIndex>=4?.9f:.28f;recordCal(t);}
  else if(railHeld)strength=rail;
  else if(mode==1)strength=pValid?normalize(t.pressure,pLo,pHi):clamp(.1f+.9f*t.pressure,.1f,1);
  else if(mode==2)strength=cValid?normalize(t.area,cLo,cHi):fixed;
  else if(mode==0&&pValid)strength=normalize(t.pressure,pLo,pHi);
  else if(mode==0&&cValid)strength=normalize(t.area,cLo,cHi);
  int art=autoRim&&t.r>=.93?4:articulation;
  if(playing&&!nativeHit(t.x,t.y,strength,art))showMessage("Input queue full");
  if(drum!=null)drum.hit(t.x,t.y);
  lastStrength=strength;lastX=t.x;lastY=t.y;lastP=t.pressure;lastA=t.area;updateStatus();
 }
 float normalize(float x,float lo,float hi){return hi>lo?clamp(.12f+.88f*(x-lo)/(hi-lo),.1f,1):fixed;}
 void recordCal(Touch t){
  calibrationSamples[calibrationIndex][0]=t.pressure;calibrationSamples[calibrationIndex][1]=t.area;calibrationIndex++;
  if(calibrationIndex==8){
   float[] mean=new float[4];for(int i=0;i<8;i++)for(int j=0;j<2;j++)mean[(i<4?0:2)+j]+=calibrationSamples[i][j]/4;
   pLo=mean[0];cLo=mean[1];pHi=mean[2];cHi=mean[3];pValid=pHi>pLo+.08f;cValid=cHi>cLo*1.20f&&cHi>cLo+2;
   prefs.edit().putFloat("plo",pLo).putFloat("phi",pHi).putFloat("clo",cLo).putFloat("chi",cHi).putBoolean("pv",pValid).putBoolean("cv",cValid).apply();
   calibrationIndex=-1;showMessage("Calibrated: pressure "+pValid+" / area "+cValid);
  }else showMessage("Calibration: "+(calibrationIndex<4?"weak ":"strong ")+((calibrationIndex%4)+1)+"/4");
 }
 private float lastStrength=.65f,lastX=0,lastY=0,lastP=0,lastA=0;
 void updateStatus(){if(detail!=null)detail.setText(String.format(java.util.Locale.US,"Strength %d%% · v %.2f m/s · x %.2f y %.2f · pressure %.2f area %.0f · pointers %d",Math.round(lastStrength*100),1.8+5.9*Math.pow(lastStrength,1.18),lastX,lastY,lastP,lastA,touches.size()+(railHeld?1:0)));}
 final Runnable statsTick=new Runnable(){public void run(){if(playing&&calibrationIndex<0){long[] s=nativeStats();status.setText("NATIVE Oboe / AAudio · frames "+s[0]+" · events dropped "+s[2]+" · modes "+s[3]);}handler.postDelayed(this,1000);}};
 @Override protected void onPause(){super.onPause();if(playing){nativeStop();playing=false;showMessage("Audio paused · press START");}}
 @Override protected void onDestroy(){handler.removeCallbacksAndMessages(null);nativeStop();super.onDestroy();}
}
