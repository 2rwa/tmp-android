package com.example.jpegstream;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;

/** Draws the latest decoded frame without queuing old frames on the UI thread. */
public final class FrameView extends View {
    private final Object lock = new Object();
    private Bitmap bitmap;
    private int cursorX = 32768;
    private int cursorY = 32768;
    private final Paint imagePaint = new Paint(Paint.FILTER_BITMAP_FLAG);
    private final Paint crossPaint = new Paint();
    private final RectF dest = new RectF();

    public FrameView(Context context) { super(context); setBackgroundColor(Color.BLACK); }

    public void setFrame(Bitmap next, int x, int y) {
        synchronized (lock) {
            Bitmap old = bitmap;
            bitmap = next;
            cursorX = x;
            cursorY = y;
            if (old != null && old != next) old.recycle();
        }
        postInvalidate();
    }

    public void clear() {
        synchronized (lock) {
            if (bitmap != null) bitmap.recycle();
            bitmap = null;
        }
        postInvalidate();
    }

    @Override protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        synchronized (lock) {
            if (bitmap == null || bitmap.isRecycled()) return;
            float scale = Math.min(getWidth() / (float) bitmap.getWidth(),
                    getHeight() / (float) bitmap.getHeight());
            float w = bitmap.getWidth() * scale, h = bitmap.getHeight() * scale;
            float x = (getWidth() - w) / 2f, y = (getHeight() - h) / 2f;
            dest.set(x, y, x + w, y + h);
            canvas.drawBitmap(bitmap, null, dest, imagePaint);
            float cx = x + (cursorX / 65535f) * w;
            float cy = y + (cursorY / 65535f) * h;
            crossPaint.setStyle(Paint.Style.STROKE);
            crossPaint.setStrokeWidth(3);
            crossPaint.setColor(Color.BLACK);
            drawCross(canvas, cx, cy);
            crossPaint.setStrokeWidth(1.5f);
            crossPaint.setColor(Color.YELLOW);
            drawCross(canvas, cx, cy);
        }
    }

    private void drawCross(Canvas c, float x, float y) {
        c.drawLine(x - 18, y, x - 5, y, crossPaint);
        c.drawLine(x + 5, y, x + 18, y, crossPaint);
        c.drawLine(x, y - 18, x, y - 5, crossPaint);
        c.drawLine(x, y + 5, x, y + 18, crossPaint);
    }
}
