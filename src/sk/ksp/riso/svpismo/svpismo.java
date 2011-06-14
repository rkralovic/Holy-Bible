package sk.ksp.riso.svpismo;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.content.res.*;
import android.util.Log;
import android.widget.Button;
import android.view.View;
import android.view.Window;

import java.lang.Object;
import sk.ksp.riso.svpismo.JSInterface;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import android.net.Uri;
import android.content.Intent;

public class svpismo extends Activity
{
    static final String TAG = "svpismo";
    MappedByteBuffer db, css;
    long db_len, css_len;

    public WebView wv;

    public void load(String url) {
      String cnt = process(db, db_len, css, css_len, url);
      wv.loadData(cnt, "text/html", "utf-8");
    }

    public void back() {
      wv.goBack();
    }

    public void forward() {
      wv.goForward();
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        final svpismo myself = this;

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.svpismo);

        wv = (WebView)findViewById(R.id.wv);
        ((Button)findViewById(R.id.backBtn)).setOnClickListener(new View.OnClickListener() {
          public void onClick(View v) {
            wv.goBack();
          }
        });
   
        ((Button)findViewById(R.id.forwardBtn)).setOnClickListener(new View.OnClickListener() {
          public void onClick(View v) {
            wv.goForward();
          }
        });
   
        ((Button)findViewById(R.id.downBtn)).setOnClickListener(new View.OnClickListener() {
          public void onClick(View v) {
            wv.pageDown(true);
          }
        });
   
        ((Button)findViewById(R.id.pgdnBtn)).setOnClickListener(new View.OnClickListener() {
          public void onClick(View v) {
            wv.pageDown(false);
          }
        });

        try {
          {
            AssetFileDescriptor dbf = getAssets().openFd("pismo.bin");
            FileInputStream fis = dbf.createInputStream();
            FileChannel channel = fis.getChannel();
            db = channel.map(FileChannel.MapMode.READ_ONLY, dbf.getStartOffset(), dbf.getLength());
            db_len = dbf.getLength();
          }

          {
            AssetFileDescriptor dbf = getAssets().openFd("breviar.css");
            FileChannel channel = dbf.createInputStream().getChannel();
            css = channel.map(FileChannel.MapMode.READ_ONLY, dbf.getStartOffset(), dbf.getLength());
            css_len = dbf.getLength();
          }

          Intent I = getIntent();
          if (I.getAction().equals("sk.ksp.riso.svpismo.action.SHOW")) {
            load("pismo.cgi?" + I.getData().getQuery());
          } else {
            if (wv.restoreState(savedInstanceState) == null)
              load("pismo.cgi?c=Jn1,1");
          }

          wv.getSettings().setJavaScriptEnabled(true);
          wv.addJavascriptInterface(new JSInterface(this), "bridge");

          wv.setWebViewClient( new WebViewClient() {
            svpismo parent;
            { parent = myself; }
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
              parent.load(url);
              return true;
            }
          });

          wv.getSettings().setBuiltInZoomControls(true);

        } catch (IOException e) {
          wv.loadData("Some problem.", "text/html", "utf-8");
        }
    }

    protected void onSaveInstanceState(Bundle outState) {
      wv.saveState(outState);
    }

    public native String process(ByteBuffer db, long db_len, ByteBuffer css, long css_len, String querystring);

    static {
        System.loadLibrary("pismo");
    }
}
