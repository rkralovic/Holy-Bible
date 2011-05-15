package sk.kbs.dkc.android;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.content.res.*;
import android.util.Log;

import java.lang.Object;

import sk.kbs.dkc.android.JSInterface;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;


public class svpismo extends Activity
{
    static final String TAG = "svpismo";
    MappedByteBuffer db, css;
    long db_len, css_len;

    public WebView wv;

    static final int HISTSIZE = 1000;
    String hist[];
    int histHead, histAct, histTail;

    void rawLoad(String url) {
      String cnt = process(db, db_len, css, css_len, url);
      wv.loadData(cnt, "text/html", "utf-8");
    }

    public void load(String url) {
      hist[histAct] = url;
      histAct = (histAct+1) % HISTSIZE;
      histTail = histAct;
      if (histHead == histAct) histHead = (histHead+1) % HISTSIZE;
      rawLoad(url);
    }

    public void back() {
      if (histHead != histAct) {
        if ((histHead+1)%HISTSIZE != histAct) {
          histAct = (histAct + HISTSIZE-1) % HISTSIZE;
          rawLoad( hist[(histAct + HISTSIZE-1) % HISTSIZE] );
        }
      }
    }

    public void forward() {
      if (histAct != histTail) {
        rawLoad( hist[histAct] );
        histAct = (histAct + 1) % HISTSIZE;
      }
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        final svpismo myself = this;

        wv = new WebView(this);

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

          wv.getSettings().setJavaScriptEnabled(true);
          wv.addJavascriptInterface(new JSInterface(this), "bridge");
          histHead = 0;
          histAct = 0;
          hist = new String[HISTSIZE];
          load("pismo.cgi?c=Jn1,1");

          wv.setWebViewClient( new WebViewClient() {
            svpismo parent;
            { parent = myself; }
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
              parent.load(url);
              return true;
            }
          });

          setContentView(wv);
        } catch (IOException e) {
          wv.loadData("Some problem.", "text/html", "utf-8");
        }
    }

    public native String process(ByteBuffer db, long db_len, ByteBuffer css, long css_len, String querystring);

    static {
        System.loadLibrary("pismo");
    }
}
