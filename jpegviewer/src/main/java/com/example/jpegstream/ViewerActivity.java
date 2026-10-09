package com.example.jpegstream;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.text.InputType;
import java.io.BufferedInputStream;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.Locale;

/**
 * TCP MJPG1 protocol:
 * 4-byte ASCII 'MJP1', uint32 big-endian JPEG size (1..4 MiB),
 * uint16 cursor x, uint16 cursor y (both normalized to 0..65535), JPEG bytes.
 * The Mac is the TCP server; the phone connects to its LAN IPv4 address.
 */
public final class ViewerActivity extends Activity {
    private static final int PORT = 5055;
    private static final int MAX_JPEG = 4 * 1024 * 1024;
    private volatile boolean running;
    private volatile Socket socket;
    private int generation;
    private Thread receiver;
    private Button button;
    private EditText address;
    private TextView status;
    private FrameView frameView;

    @Override public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        final float density = getResources().getDisplayMetrics().density;
        int pad = (int)(8 * density);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(pad, pad, pad, pad);
        LinearLayout top = new LinearLayout(this);
        top.setOrientation(LinearLayout.HORIZONTAL);
        top.setGravity(Gravity.CENTER_VERTICAL);
        address = new EditText(this);
        address.setSingleLine(true);
        address.setText(getPreferences(MODE_PRIVATE).getString("host", "192.168.1.2"));
        address.setHint("Mac の IP アドレス");
        address.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI);
        top.addView(address, new LinearLayout.LayoutParams(0, -2, 1));
        button = new Button(this);
        button.setText("接続");
        top.addView(button, new LinearLayout.LayoutParams(-2, -2));
        root.addView(top, new LinearLayout.LayoutParams(-1, -2));
        status = new TextView(this);
        status.setText("同じWi-Fi / LAN上のMacを指定 (TCP :5055)");
        root.addView(status, new LinearLayout.LayoutParams(-1, -2));
        frameView = new FrameView(this);
        root.addView(frameView, new LinearLayout.LayoutParams(-1, 0, 1));
        setContentView(root);
        button.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { if (running) disconnect(); else connect(); }
        });
    }

    private void connect() {
        String host = address.getText().toString().trim();
        if (host.length() == 0) { status.setText("Mac の IP アドレスを入力してください"); return; }
        getPreferences(MODE_PRIVATE).edit().putString("host", host).apply();
        final int id = ++generation;
        running = true;
        button.setText("切断");
        status.setText("接続中: " + host + ":" + PORT);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        receiver = new Thread(new Runnable() {
            @Override public void run() { receive(host, id); }
        }, "jpeg-receiver");
        receiver.start();
    }

    private void receive(String host, int id) {
        long first = System.nanoTime(), bytes = 0, count = 0;
        Socket s = new Socket();
        try {
            socket = s;
            s.connect(new InetSocketAddress(host, PORT), 4000);
            s.setTcpNoDelay(true);
            s.setSoTimeout(10000);
            DataInputStream stream = new DataInputStream(new BufferedInputStream(s.getInputStream(), 128 * 1024));
            byte[] jpeg = new byte[256 * 1024];
            byte[] magic = new byte[4];
            postStatus(id, "接続済み：JPEGデータ待機中");
            while (running && generation == id) {
                stream.readFully(magic);
                if (magic[0] != 'M' || magic[1] != 'J' || magic[2] != 'P' || magic[3] != '1')
                    throw new IOException("不正なフレームヘッダー");
                int length = stream.readInt();
                if (length < 1 || length > MAX_JPEG) throw new IOException("JPEGのサイズが不正: " + length);
                int x = stream.readUnsignedShort(), y = stream.readUnsignedShort();
                if (jpeg.length < length) jpeg = new byte[length];
                stream.readFully(jpeg, 0, length);
                Bitmap decoded = BitmapFactory.decodeByteArray(jpeg, 0, length);
                if (decoded == null) throw new IOException("JPEGデコード失敗");
                if (!running || generation != id) { decoded.recycle(); break; }
                frameView.setFrame(decoded, x, y);
                count++;
                bytes += length;
                long elapsed = System.nanoTime() - first;
                if (elapsed >= 1000000000L) {
                    final String report = String.format(Locale.US, "%.1f fps / %.2f Mbps / %dx%d",
                        count * 1e9 / elapsed, bytes * 8e3 / elapsed,
                        decoded.getWidth(), decoded.getHeight());
                    postStatus(id, report);
                    first = System.nanoTime();
                    bytes = 0;
                    count = 0;
                }
            }
        } catch (EOFException e) {
            postStatus(id, "Macが接続を終了しました");
        } catch (IOException e) {
            if (running && generation == id) postStatus(id, "通信エラー: " + e.getMessage());
        } catch (RuntimeException e) {
            if (running && generation == id) postStatus(id, "受信エラー: " + e.toString());
        } finally {
            try { s.close(); } catch (IOException ignored) {}
            runOnUiThread(new Runnable() {
                @Override public void run() {
                    if (generation == id) {
                        running = false;
                        socket = null;
                        button.setText("接続");
                        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                    }
                }
            });
        }
    }

    private void postStatus(final int id, final String message) {
        runOnUiThread(new Runnable() {
            @Override public void run() { if (generation == id) status.setText(message); }
        });
    }

    private void disconnect() {
        running = false;
        ++generation;
        Socket s = socket;
        if (s != null) try { s.close(); } catch (IOException ignored) {}
        socket = null;
        button.setText("接続");
        status.setText("切断しました");
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override protected void onDestroy() {
        disconnect();
        frameView.clear();
        super.onDestroy();
    }
}
