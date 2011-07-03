package sk.ksp.riso.svpismo;

import android.app.Activity;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.content.res.AssetManager;
import android.util.Log;
import sk.ksp.riso.svpismo.svpismo;

import java.lang.Object;

public class JSInterface {
    svpismo mContext;

    JSInterface (svpismo c) {
      mContext = c;
    }

    public void loadit(final String url) {
//      Log.v("svpismo", "bridge called: "+url);
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.load(url);
        }
      });
    }

    public void scrollbottom() {
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.wv.pageDown(true);
        }
      });
    }

    public void back() {
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.back();
        }
      });
    }

    public void forward() {
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.forward();
        }
      });
    }

}
