package com.example.odaiko;
import android.view.View;
import android.view.MotionEvent;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.LinearGradient;
import android.graphics.Shader;
final class StrengthRail extends View {
 final TaikoActivity a;final Paint p=new Paint(3);float strength=.65f;int pointer=-1;
 StrengthRail(android.content.Context c,TaikoActivity owner){super(c);a=owner;}
 @Override protected void onDraw(Canvas canvas){
  float left=8,right=getWidth()-8,top=12,bottom=getHeight()-12;
  p.setShader(new LinearGradient(0,bottom,0,top,0xff352014,0xffe4ad5e,Shader.TileMode.CLAMP));
  canvas.drawRoundRect(left,top,right,bottom,12,12,p);p.setShader(null);
  float y=bottom-(bottom-top)*strength;
  p.setColor(0xffffeed2);canvas.drawRoundRect(left-3,y-4,right+3,y+4,4,4,p);
  p.setTextSize(13*getResources().getDisplayMetrics().scaledDensity);p.setColor(0xff201408);
  canvas.drawText("強",left+5,top+22,p);p.setColor(0xffffe2bb);canvas.drawText("弱",left+5,bottom-8,p);
 }
 private void set(float y,boolean hold){float v=(getHeight()-12-y)/Math.max(1,getHeight()-24);strength=TaikoActivity.clamp(v,.10f,1);a.setRail(strength,hold);invalidate();}
 @Override public boolean onTouchEvent(MotionEvent e){
  int action=e.getActionMasked();
  if(action==MotionEvent.ACTION_DOWN){pointer=e.getPointerId(0);set(e.getY(0),true);return true;}
  if(action==MotionEvent.ACTION_MOVE){int idx=e.findPointerIndex(pointer);if(idx>=0)set(e.getY(idx),true);return true;}
  if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){set(e.getY(0),false);pointer=-1;return true;}
  return true;
 }
}