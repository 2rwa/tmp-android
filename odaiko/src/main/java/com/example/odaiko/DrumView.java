package com.example.odaiko;
import android.view.View;
import android.view.MotionEvent;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RadialGradient;
import android.graphics.Shader;
import android.os.SystemClock;

final class DrumView extends View {
 final TaikoActivity owner;
 final Paint p=new Paint(3);float hx=0,hy=0;
 long lastHit=0;
 DrumView(android.content.Context c,TaikoActivity owner){super(c);this.owner=owner;setLayerType(View.LAYER_TYPE_SOFTWARE,null);}
 private float radius(){return Math.max(1,Math.min(getWidth(),getHeight())*.465f);}
 private float cx(){return getWidth()*.5f;}
 private float cy(){return getHeight()*.5f;}
 @Override protected void onDraw(Canvas c){
  super.onDraw(c);float cx=cx(),cy=cy(),r=radius();
  p.setShader(null);p.setColor(Color.rgb(69,43,29));c.drawCircle(cx,cy,r+3,p);
  p.setShader(new RadialGradient(cx-r*.2f,cy-r*.22f,r*1.45f,new int[]{0xffead6af,0xffbd925c,0xff80522e},null,Shader.TileMode.CLAMP));
  c.drawCircle(cx,cy,r,p);p.setShader(null);
  p.setColor(0xffe8c597);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(3);c.drawCircle(cx,cy,r*.95f,p);
  p.setColor(0x44775132);p.setStrokeWidth(1);c.drawCircle(cx,cy,r*.75f,p);
  p.setStyle(Paint.Style.FILL);
  for(int i=0;i<20;i++){float a=(float)(i*Math.PI/10);p.setColor(0xff49321f);c.drawCircle(cx+(float)Math.cos(a)*r*.90f,cy+(float)Math.sin(a)*r*.90f,Math.max(2,r*.017f),p);}
  long dt=SystemClock.uptimeMillis()-lastHit;
  if(dt<220){p.setColor(0x99ffe4a8);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(3);c.drawCircle(cx+hx*r,cy+hy*r,(float)(r*.05+dt*r*.0006),p);p.setStyle(Paint.Style.FILL);postInvalidateDelayed(16);}
 }
 void hit(float x,float y){hx=x;hy=y;lastHit=SystemClock.uptimeMillis();invalidate();}
 private float[] point(MotionEvent e,int index){
  float x=(e.getX(index)-cx())/radius(),y=(e.getY(index)-cy())/radius();
  float r=(float)Math.hypot(x,y);
  return new float[]{Math.max(-.96f,Math.min(.96f,x)),Math.max(-.96f,Math.min(.96f,y)),r};
 }
 private float area(MotionEvent e,int i){float major=e.getTouchMajor(i),minor=e.getTouchMinor(i);return Math.max(0,major*minor);}
 @Override public boolean onTouchEvent(MotionEvent e){
  int a=e.getActionMasked(),index=e.getActionIndex();
  if(a==MotionEvent.ACTION_DOWN||a==MotionEvent.ACTION_POINTER_DOWN){
   float[] pt=point(e,index);owner.touchDown(e.getPointerId(index),pt[0],pt[1],pt[2],e.getPressure(index),area(e,index));
  }else if(a==MotionEvent.ACTION_MOVE){
   for(int i=0;i<e.getPointerCount();i++)owner.touchMove(e.getPointerId(i),e.getPressure(i),area(e,i));
  }else if(a==MotionEvent.ACTION_UP||a==MotionEvent.ACTION_POINTER_UP){
   owner.touchMove(e.getPointerId(index),e.getPressure(index),area(e,index));
   owner.touchUp(e.getPointerId(index));
  }else if(a==MotionEvent.ACTION_CANCEL){
   for(int i=0;i<e.getPointerCount();i++)owner.touchCancel(e.getPointerId(i));
  }
  return true;
 }
}