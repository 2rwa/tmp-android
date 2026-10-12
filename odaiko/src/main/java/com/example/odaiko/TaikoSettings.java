package com.example.odaiko;
import android.view.View;
import android.widget.*;
import android.content.res.ColorStateList;
import android.graphics.Color;
final class TaikoSettings {
 final TaikoActivity a;final LinearLayout settings;
 TaikoSettings(TaikoActivity activity,LinearLayout layout){a=activity;settings=layout;}
 TextView text(String value,int size){return a.text(value,size);}
 Button button(String value){return a.button(value);}
 void create(){
  settings.addView(text("INPUT · Mobile Multi-Touch v2",18));
  spinner("Strength source",new String[]{"Auto (calibrated)","Pressure","Contact area","Fixed"},0,value->a.mode=value);
  seek("Fixed strength",10,100,65,value->a.fixed=value*.01f);
  spinner("Attack window",new String[]{"0 ms","8 ms","12 ms","20 ms"},2,v->a.windowMs=new int[]{0,8,12,20}[v]);
  CheckBox rim=new CheckBox(a);rim.setText("Auto KA at rim ≥ 0.93");rim.setTextColor(Color.WHITE);rim.setChecked(true);settings.addView(rim);rim.setOnCheckedChangeListener((b,on)->a.autoRim=on);
  Button calibrate=button("Calibrate: weak × 4 / strong × 4");settings.addView(calibrate);calibrate.setOnClickListener(v->{a.calibrationIndex=0;a.showMessage("Calibration: weak 1/4");});
  Button reset=button("Reset calibration");settings.addView(reset);reset.setOnClickListener(v->{a.pValid=a.cValid=false;a.prefs.edit().clear().apply();a.showMessage("Calibration cleared");});
  settings.addView(text("SOUND · apply restarts output",18));
  spinner("Quality",new String[]{"ECO 64/4/2","NORMAL 128/8/4","HEAVY 240/16/8","MAX TEST 384/24/12"},1,v->a.quality=v);
  seek("Room wet (%)",0,40,18,v->a.wet=v);
  seek("Master gain (dB)",0,24,16,v->{a.gain=v;if(a.playing)TaikoActivity.nativeGain(v);});
  Button apply=button("Apply audio settings");settings.addView(apply);apply.setOnClickListener(v->{if(a.playing){int sr=TaikoActivity.nativeStart(a.quality,a.wet);TaikoActivity.nativeGain(a.gain);a.playing=sr>0;a.showMessage(a.playing?"Applied "+sr+" Hz":"Apply failed "+sr);}a.toggleSettings();});
  settings.addView(text("Speaker or wired output recommended for low latency.",12));
 }
 interface Selection {void set(int n);}
 void spinner(String title,String[] items,int selected,Selection cb){
  settings.addView(text(title,13));Spinner sp=new Spinner(a);ArrayAdapter<String> ad=new ArrayAdapter<>(a,android.R.layout.simple_spinner_dropdown_item,items);sp.setAdapter(ad);settings.addView(sp);sp.setSelection(selected);
  sp.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener(){public void onNothingSelected(android.widget.AdapterView<?> p){}public void onItemSelected(android.widget.AdapterView<?>p,View v,int pos,long id){cb.set(pos);}});
 }
 void seek(String title,int min,int max,int value,Selection cb){
  TextView label=text(title+" : "+value,13);settings.addView(label);SeekBar sb=new SeekBar(a);sb.setMax(max-min);sb.setProgress(value-min);settings.addView(sb);
  sb.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){public void onStartTrackingTouch(SeekBar b){}public void onStopTrackingTouch(SeekBar b){}public void onProgressChanged(SeekBar b,int v,boolean user){cb.set(v+min);label.setText(title+" : "+(v+min));}});
 }
}