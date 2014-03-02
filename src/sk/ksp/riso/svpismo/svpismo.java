package sk.ksp.riso.svpismo;

import android.app.Activity;
import android.os.Build;
import android.os.Bundle;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.content.res.*;
import android.util.Log;
import android.widget.Button;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import sk.ksp.riso.svpismo.JSInterface;
import sk.ksp.riso.svpismo.Bookmarks;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import android.content.Intent;
import android.content.SharedPreferences;

public class svpismo extends Activity
{
    static final String TAG = "svpismo";
    static final String prefname = "SvPismoPrefs";
    MappedByteBuffer db, css, css_inv;
    long db_len, css_len, css_inv_len;
    int scale;
    float scroll_to = -1;

    public WebView wv;
    boolean wv_initialized = false;
    boolean comments, nightmode, fullscreen;
    String active_url = "";
    final String toc_url = "pismo.php?obsah=long";

    public void load(String url) {
      if (wv_initialized) {
        scale = (int)(wv.getScale()*100);
//        Log.v("svpismo", "load: getScale " + scale);
      }
      String cnt;
      if (nightmode) {
        cnt = "data:text/html;charset=UTF-8," +
               process(db, db_len, css_inv, css_inv_len, url, comments);
      } else {
        cnt = "data:text/html;charset=UTF-8," +
              process(db, db_len, css, css_len, url, comments);
      }
      wv.loadUrl(cnt);
      wv.setInitialScale(scale);
      wv_initialized = true;
      active_url = url;
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

        SharedPreferences settings = getSharedPreferences(prefname, 0);
        scale = settings.getInt("scale", 100);
        comments = settings.getBoolean("comments", true);
        nightmode = settings.getBoolean("nightmode", false);
        fullscreen = settings.getBoolean("fullscreen", false);
//        Log.v("svpismo", "init with scale " + scale);

        wv = (WebView)findViewById(R.id.wv);
        wv.getSettings().setBuiltInZoomControls(true);
        wv.setInitialScale(scale);

        ((Button)findViewById(R.id.pgupBtn)).setOnClickListener(new View.OnClickListener() {
          public void onClick(View v) {
            wv.pageUp(false);
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

          {
            AssetFileDescriptor dbf = getAssets().openFd("breviar-invert.css");
            FileChannel channel = dbf.createInputStream().getChannel();
            css_inv = channel.map(FileChannel.MapMode.READ_ONLY, dbf.getStartOffset(), dbf.getLength());
            css_inv_len = dbf.getLength();
          }

          Intent I = getIntent();
          if (wv.restoreState(savedInstanceState) == null) {
            if (I.getAction().equals("sk.ksp.riso.svpismo.action.SHOW")) {
              if (I.hasExtra("nightmode")) {
                nightmode = I.getBooleanExtra("nightmode", false);
                syncPreferences();
              }
              load("pismo.cgi?" + I.getData().getQuery());
            } else {
              load("pismo.cgi");
            }
          }

          wv.getSettings().setJavaScriptEnabled(true);
          wv.addJavascriptInterface(new JSInterface(this), "bridge");

          wv.setWebViewClient( new WebViewClient() {
            svpismo parent;
            boolean scaleChangedRunning = false;
            { parent = myself; }
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
              parent.load(url);
              return true;
            }

            @Override
            public void onScaleChanged(WebView view, float oldSc, float newSc) {
              parent.scale = (int)(newSc*100);
              if (Build.VERSION.SDK_INT < 19) {  // pre-KitKat
                view.setInitialScale(parent.scale);
              } else {
                if (scaleChangedRunning) return;
                scaleChangedRunning = true;
                final WebView final_view = view;
                view.postDelayed(new Runnable() {
                  @Override
                  public void run() {
                    try {
                      final_view.evaluateJavascript(
                          "document.getElementById('contentRoot').style.width = window.innerWidth;",
                          null);
                    } catch (java.lang.IllegalStateException e) {
                      Log.v("svpismo", "Cannot call evaluateJavascript. Cyanogenmod weirdness?");
                    }
                    scaleChangedRunning = false;
                  }
                }, 100);
              }
//              Log.v("svpismo", "onScaleChanged " + parent.scale);
            }

            @Override
            public void onPageFinished(WebView view, String url) {
              super.onPageFinished(view, url);
              // Ugly hack. But we have no reliable notification when is webview scrollable.
              final WebView wv = view;
              view.postDelayed(new Runnable() {
                public void run() {
                  if (parent.scroll_to >= 0) {
                    int Y = (int)(parent.scroll_to*wv.getContentHeight());
                    wv.scrollTo(0, Y);
                  }
                  parent.scroll_to = -1;
                }
              }, 400);
            }

          });

          updateFullscreen();
        } catch (IOException e) {
          wv.loadData("Some problem.", "text/html", "utf-8");
        }
    }

    protected void onSaveInstanceState(Bundle outState) {
      scale = (int)(wv.getScale()*100);
      wv.setInitialScale(scale);
      wv.saveState(outState);
//      Log.v("svpismo", "onSaveInstanceState " + scale);
      syncPreferences();
    }

    void syncPreferences() {
      SharedPreferences settings = getSharedPreferences(prefname, 0);
      SharedPreferences.Editor editor = settings.edit();
      editor.putInt("scale", scale);
      editor.putBoolean("comments", comments);
      editor.putBoolean("nightmode", nightmode);
      editor.putBoolean("fullscreen", fullscreen);
      editor.commit();
    }

    protected void onStop(){
      scale = (int)(wv.getScale()*100);
      wv.setInitialScale(scale);
//      Log.v("svpismo", "onStop " + scale);
      syncPreferences();
      super.onStop();
    }

    @Override
    public boolean onCreateOptionsMenu( Menu menu ) {
      // Inflate the currently selected menu XML resource.
      MenuInflater inflater = getMenuInflater();
      inflater.inflate( R.menu.menu, menu );
      return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
      // Handle item selection
      switch (item.getItemId()) {
        case R.id.toc:
          load(toc_url);
          return true;
        case R.id.comments_toggle:
          comments = !comments;
          syncPreferences();
          load(active_url);
          return true;
        case R.id.nightmode_toggle:
          nightmode = !nightmode;
          syncPreferences();
          load(active_url);
          return true;
        case R.id.fullscreen_toggle:
          fullscreen = !fullscreen;
          updateFullscreen();
          syncPreferences();
          return true;
        case R.id.bookmarks:
	  Intent i = new Intent(this, Bookmarks.class);
	  i.putExtra("location", active_url); 
	  i.putExtra("position", wv.getScrollY() / (float)wv.getContentHeight());
	  startActivityForResult(i, Bookmarks.BOOKMARKS);
          return true;
        default:
          return super.onOptionsItemSelected(item);
      }
    }    

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
      if ((keyCode == KeyEvent.KEYCODE_BACK) && wv.canGoBack()) {
        wv.goBack();
        return true;
      }
      if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
        wv.pageUp(false);
        return true;
      }
      if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
        wv.pageDown(false);
        return true;
      }
      return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
      if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
        return true;
      }
      if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
        return true;
      }
      return super.onKeyUp(keyCode, event);
    }

    public native String process(ByteBuffer db, long db_len, ByteBuffer css,
        long css_len, String querystring, boolean comments);

    static {
        System.loadLibrary("pismo");
    }

    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
      if ((active_url != toc_url) != (menu.findItem(R.id.toc).isVisible())) {
    	  menu.findItem(R.id.toc).setVisible(active_url != toc_url);
      };
      if (comments) {
        menu.findItem(R.id.comments_toggle).setTitle(R.string.comments_off);
      } else {
        menu.findItem(R.id.comments_toggle).setTitle(R.string.comments_on);
      }
      if (nightmode) {
        menu.findItem(R.id.nightmode_toggle).setTitle(R.string.nightmode_off);
      } else {
        menu.findItem(R.id.nightmode_toggle).setTitle(R.string.nightmode_on);
      }
      if (fullscreen) {
        menu.findItem(R.id.fullscreen_toggle).setTitle(R.string.fullscreen_off);
      } else {
        menu.findItem(R.id.fullscreen_toggle).setTitle(R.string.fullscreen_on);
      }
      return super.onPrepareOptionsMenu(menu);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
      switch (requestCode) {
        case Bookmarks.BOOKMARKS:
	  if (resultCode == RESULT_OK) {
            scroll_to = data.getFloatExtra("position", 0);
            load(data.getStringExtra("location"));
	  }
	  break;
        default:
          super.onActivityResult(requestCode, resultCode, data);
      }
    }    

    void updateFullscreen() {
      WindowManager.LayoutParams params = getWindow().getAttributes();
      if (fullscreen) {
        findViewById(R.id.navbar).setVisibility(View.GONE);
        params.flags |= WindowManager.LayoutParams.FLAG_FULLSCREEN;
      } else {
        findViewById(R.id.navbar).setVisibility(View.VISIBLE);
        params.flags &= ~WindowManager.LayoutParams.FLAG_FULLSCREEN;
      }
      getWindow().setAttributes(params);
    }
}
