package com.example.liquidglass;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.graphics.Color;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

public class GlobeActivity extends Activity implements SurfaceHolder.Callback {
    static { System.loadLibrary("liquidglass"); }
    private static native void nativeStart(Surface surface, AssetManager assets);
    private static native void nativeStop();
    private static native void nativeSettings(int count, float merge, float speed, float yaw, float pitch);
    private static native String nativeStatus();
    private SurfaceView surface;
    private TextView status, glassLabel, mergeLabel, speedLabel;
    private int glassCount = 24, opaqueCount = 7, resolution = 1;
    private float blend = .32f, speed = 1f, yaw=.32f, pitch=.18f;
    private float lastX, lastY;
    private boolean paused = false;
    private final Handler handler=new Handler(Looper.getMainLooper());
    private final Runnable updateStatus = new Runnable() {
        @Override public void run() {
            if(status!=null)status.setText(nativeStatus());
            handler.postDelayed(this,1000);
        }
    };
    private int dp(int size) {return Math.round(size * getResources().getDisplayMetrics().density);}
    private TextView text(String caption,int size) {
        TextView t=new TextView(this);t.setText(caption);t.setTextColor(0xffd6f3fc);
        t.setTextSize(size);t.setPadding(dp(6),dp(2),dp(6),dp(2));return t;
    }
    private SeekBar slider(LinearLayout parent,int max,int initial,java.util.function.IntConsumer handler) {
        SeekBar bar=new SeekBar(this);bar.setMax(max);bar.setProgress(initial);
        parent.addView(bar,new LinearLayout.LayoutParams(-1,dp(35)));
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar s,int progress,boolean user){if(user)handler.accept(progress);}
            public void onStartTrackingTouch(SeekBar s){}
            public void onStopTrackingTouch(SeekBar s){}
        });return bar;
    }
    private void setNative(){nativeSettings(glassCount,blend,paused?0f:speed,yaw,pitch);}
    private void changeResolution() {
        int[] shortSides={160,240,320};
        android.util.DisplayMetrics dm=getResources().getDisplayMetrics();
        int shortPixels=shortSides[resolution];
        int longPixels=Math.round(shortPixels*(float)Math.max(dm.widthPixels,dm.heightPixels)/Math.max(1,Math.min(dm.widthPixels,dm.heightPixels)));
        surface.getHolder().setFixedSize(dm.widthPixels>dm.heightPixels?longPixels:shortPixels,
            dm.widthPixels>dm.heightPixels?shortPixels:longPixels);
    }
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        FrameLayout root=new FrameLayout(this);root.setBackgroundColor(Color.BLACK);
        surface=new SurfaceView(this);surface.getHolder().addCallback(this);
        root.addView(surface,new FrameLayout.LayoutParams(-1,-1));
        surface.setOnTouchListener((View v,MotionEvent event)->{
            if(event.getActionMasked()==MotionEvent.ACTION_DOWN){lastX=event.getX();lastY=event.getY();return true;}
            if(event.getActionMasked()==MotionEvent.ACTION_MOVE){yaw+=(event.getX()-lastX)*.006f;
                pitch=Math.max(-1.2f,Math.min(1.2f,pitch+(lastY-event.getY())*.006f));
                lastX=event.getX();lastY=event.getY();setNative();return true;}
            return true;
        });
        LinearLayout panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackgroundColor(0xd5132437);panel.setPadding(dp(10),dp(6),dp(10),dp(8));
        FrameLayout.LayoutParams pp=new FrameLayout.LayoutParams(dp(225),-2,Gravity.RIGHT|Gravity.TOP);
        pp.setMargins(0,dp(22),dp(10),0);root.addView(panel,pp);
        TextView title=text("LIQUID GLASS  •  Vulkan",14);panel.addView(title);
        status=text("Starting native Vulkan…",11);panel.addView(status);
        glassLabel=text("Glass droplets: 24",12);panel.addView(glassLabel);
        slider(panel,64,glassCount,n->{glassCount=n;glassLabel.setText("Glass droplets: "+n);setNative();});
        mergeLabel=text("Fusion: 0.32",12);panel.addView(mergeLabel);
        slider(panel,63,30,n->{blend=(n+2)/100f;mergeLabel.setText(String.format(java.util.Locale.US,"Fusion: %.2f",blend));setNative();});
        speedLabel=text("Speed: 1.0×",12);panel.addView(speedLabel);
        slider(panel,25,10,n->{speed=n*.1f;speedLabel.setText(String.format(java.util.Locale.US,"Speed: %.1f×",speed));setNative();});
        LinearLayout buttons=new LinearLayout(this);buttons.setOrientation(LinearLayout.HORIZONTAL);panel.addView(buttons);
        Button pause=new Button(this);pause.setText("Pause");buttons.addView(pause,new LinearLayout.LayoutParams(0,dp(48),1));
        pause.setOnClickListener(v->{paused=!paused;pause.setText(paused?"Play":"Pause");setNative();});
        Button quality=new Button(this);quality.setText("240p");buttons.addView(quality,new LinearLayout.LayoutParams(0,dp(48),1));
        quality.setOnClickListener(v->{resolution=(resolution+1)%3;changeResolution();quality.setText(new String[]{"160p","240p","320p"}[resolution]);});
        TextView foot=text("Drag background to orbit · Android 9+",10);panel.addView(foot);
        setContentView(root);
        changeResolution();setNative();handler.post(updateStatus);
    }
    @Override public void surfaceCreated(SurfaceHolder holder) {nativeStart(holder.getSurface(),getAssets());}
    @Override public void surfaceChanged(SurfaceHolder holder,int format,int width,int height) {
        nativeStop();nativeStart(holder.getSurface(),getAssets());
    }
    @Override public void surfaceDestroyed(SurfaceHolder holder) {nativeStop();}
    @Override protected void onDestroy(){handler.removeCallbacks(updateStatus);nativeStop();super.onDestroy();}
}
