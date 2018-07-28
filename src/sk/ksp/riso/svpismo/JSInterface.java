package sk.ksp.riso.svpismo;

import android.webkit.JavascriptInterface;

//import android.util.Log;
import sk.ksp.riso.svpismo.svpismo;

public class JSInterface {
    svpismo mContext;

    JSInterface (svpismo c) {
      mContext = c;
    }

    @JavascriptInterface
    public void loadit(final String url) {
//      Log.v("svpismo", "bridge called: "+url);
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.loadUrl(url);
        }
      });
    }

    @JavascriptInterface
    public void scrollbottom() {
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.wv.pageDown(true);
        }
      });
    }

    @JavascriptInterface
    public void back() {
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.goBack();
        }
      });
    }

    @JavascriptInterface
    public void forward() {
      mContext.wv.post(new Runnable() {
        public void run() {
          mContext.goForward();
        }
      });
    }

}
