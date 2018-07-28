package sk.ksp.riso.svpismo;

import android.app.Activity;
import android.content.Intent;
import android.content.res.*;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.os.PowerManager;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.view.MenuItemCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.ScaleGestureDetector;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.CompoundButton;

import java.io.*;
import java.nio.*;
import java.nio.channels.*;

import sk.ksp.riso.svpismo.JSInterface;
import sk.ksp.riso.svpismo.Bookmarks;

public class svpismo extends AppCompatActivity
                     implements GestureDetector.OnDoubleTapListener,
                                NavigationView.OnNavigationItemSelectedListener {
    static final String TAG = "svpismo";
    static final String prefname = "SvPismoPrefs";
    MappedByteBuffer db, css, css_inv;
    long db_len, css_len, css_inv_len;
    int scale;
    float scroll_to = -1;

    public WebView wv;
    boolean wv_initialized = false;
    boolean comments, nightmode, fullscreen, screenlock, translation_nvg;
    History history;
    final String toc_url = "pismo.php?obsah=long";

    PowerManager.WakeLock lock;

    boolean is_broken_kitkat() {
      if (Build.VERSION.SDK_INT >= 19) return true;
      return false;
    }

    public void loadUrl(String url) {
      history.push(url);
      load();
    }

    public void load() {
      String url = history.getCurrent();
      Log.v("svpismo", "load: " + url);
      if (wv_initialized) {
        scale = (int)(wv.getScale()*100);
//        Log.v("svpismo", "load: getScale " + scale);
      }
      String cnt;
      if (nightmode) {
        cnt = process(db, db_len, css_inv, css_inv_len, url, comments, is_broken_kitkat());
      } else {
        cnt = process(db, db_len, css, css_len, url, comments, is_broken_kitkat());
      }
      wv.loadDataWithBaseURL("data:" + url, cnt, "text/html", "UTF-8", url);
      wv.setInitialScale(scale);
      wv_initialized = true;
    }

    public boolean canGoBack() {
      return !history.isEmpty();
    }

    public void goBack() {
      history.pop();
      load();
    }

    public void goForward() {
      history.goForward();
      load();
    }

    DrawerLayout drawer;
    NavigationView navigationView = null;
    Toolbar toolbar = null;
    GestureDetector tap_gesture_detector;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);

      final svpismo myself = this;

      lock = ((PowerManager)getSystemService(POWER_SERVICE))
                 .newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "svpismo");

      requestWindowFeature(Window.FEATURE_NO_TITLE);
      setContentView(R.layout.svpismo);

      toolbar = (Toolbar) findViewById(R.id.svpismo_toolbar);
      setSupportActionBar(toolbar);
      getSupportActionBar().setDisplayHomeAsUpEnabled(true);
      getSupportActionBar().setTitle(getString(R.string.app_name));

      navigationView = (NavigationView) findViewById(R.id.navigation);
      navigationView.setNavigationItemSelectedListener(this);

      Menu menu = navigationView.getMenu();
      try {
        ((CompoundButton)MenuItemCompat.getActionView(menu.findItem(R.id.nightmode_toggle)))
            .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override public void onCheckedChanged(CompoundButton button,
                                                           boolean isChecked) {
                      if (isChecked == nightmode) {
                        return;
                      }
                      toggleNightMode();
                    }
                });

        ((CompoundButton)MenuItemCompat.getActionView(menu.findItem(R.id.comments_toggle)))
            .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override public void onCheckedChanged(CompoundButton button,
                                                           boolean isChecked) {
                      if (isChecked == comments) {
                        return;
                      }
                      toggleComments();
                    }
                });

        ((CompoundButton)MenuItemCompat.getActionView(menu.findItem(R.id.fullscreen_toggle)))
            .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override public void onCheckedChanged(CompoundButton button,
                                                           boolean isChecked) {
                      if (isChecked == fullscreen) {
                        return;
                      }
                      toggleFullscreen();
                    }
                });

        ((CompoundButton)MenuItemCompat.getActionView(menu.findItem(R.id.screenlock_toggle)))
            .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override public void onCheckedChanged(CompoundButton button,
                                                           boolean isChecked) {
                      if (isChecked == screenlock) {
                        return;
                      }
                      toggleScreenlock();
                    }
                });

        ((CompoundButton)MenuItemCompat.getActionView(menu.findItem(R.id.translation_toggle)))
            .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                    @Override public void onCheckedChanged(CompoundButton button,
                                                           boolean isChecked) {
                      if (isChecked == translation_nvg) {
                        return;
                      }
                      toggleTranslation();
                    }
                });

      } catch (java.lang.NullPointerException e) {
        Log.v("breviar", "Cannot setup navigation view!");
      }

      drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
      ActionBarDrawerToggle drawerToggle = new ActionBarDrawerToggle(
          this,  drawer, toolbar,
          R.string.navigation_drawer_open, R.string.navigation_drawer_close
      );
      drawer.setDrawerListener(drawerToggle);
      getSupportActionBar().setDisplayHomeAsUpEnabled(true);
      getSupportActionBar().setHomeButtonEnabled(true);
      drawerToggle.syncState();

      SharedPreferences settings = getSharedPreferences(prefname, 0);
      scale = settings.getInt("scale", 100);
      comments = settings.getBoolean("comments", true);
      nightmode = settings.getBoolean("nightmode", false);
      fullscreen = settings.getBoolean("fullscreen", false);
      screenlock = settings.getBoolean("screenlock", true);
      translation_nvg = settings.getBoolean("translation_nvg", false);
//        Log.v("svpismo", "init with scale " + scale);

      wv = (WebView)findViewById(R.id.wv);
      wv.getSettings().setBuiltInZoomControls(true);
      wv.getSettings().setDisplayZoomControls(false);
      wv.getSettings().setSupportZoom(true);
      wv.getSettings().setUseWideViewPort(false);
      wv.setInitialScale(scale);

      final svpismo parent = this;
      history = new History(this);

      ((Button)findViewById(R.id.pgupBtn)).setOnClickListener(new View.OnClickListener() {
        public void onClick(View v) {
          wv.pageUp(false);
        }
      });

      ((Button)findViewById(R.id.forwardBtn)).setOnClickListener(new View.OnClickListener() {
        public void onClick(View v) {
          goForward();
        }
      });
      /*
      ((Button)findViewById(R.id.menuBtn)).setOnClickListener(new View.OnClickListener() {
        public void onClick(View v) {
          parent.openOptionsMenu();
        }
      });
      */


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

        tap_gesture_detector = new GestureDetector(this,
                                       new GestureDetector.SimpleOnGestureListener());
        tap_gesture_detector.setOnDoubleTapListener(this);

        wv.getSettings().setJavaScriptEnabled(true);
        wv.addJavascriptInterface(new JSInterface(this), "bridge");

        wv.setWebViewClient( new WebViewClient() {
          svpismo parent;
          boolean scaleChangedRunning = false;
          { parent = myself; }
          public boolean shouldOverrideUrlLoading(WebView view, String url) {
            parent.loadUrl(url);
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
                    double w = final_view.getWidth() * 100.0 / parent.scale;
                    final_view.evaluateJavascript(
                        "document.getElementById('contentRoot').style.width = " +
                        (int)(0.95 * w) + ";",
                        null);
                    final_view.evaluateJavascript("document.getElementById('scaler').style.width = " +
                        (int)(1.5 * w) + ";", null);
                    Log.v("svpismo", "Rescaled");
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

        Intent I = getIntent();
        if (wv.restoreState(savedInstanceState) == null) {
          if (I.getAction().equals("sk.ksp.riso.svpismo.action.SHOW")) {
            if (I.hasExtra("nightmode")) {
              nightmode = I.getBooleanExtra("nightmode", false);
              syncPreferences();
            }
            loadUrl("pismo.cgi?" + I.getData().getQuery());
          } else {
            load();
          }
        } else {
          Log.v("svpismo", "Restored webview state");
        }

        updateFullscreen();
      } catch (IOException e) {
        wv.loadData("Some problem.", "text/html", "utf-8");
      }
      if (translation_nvg) {
        setTranslationNvg();
      } else {
        setTranslationSsv();
      }
      updateMenu();
    }

    protected void onSaveInstanceState(Bundle outState) {
      scale = (int)(wv.getScale()*100);
      wv.setInitialScale(scale);
      // wv.saveState(outState);
      Log.v("svpismo", "onSaveInstanceState " + scale);
      syncPreferences();
      // super.onSaveInstanceState(outState);
    }

    void syncPreferences() {
      SharedPreferences settings = getSharedPreferences(prefname, 0);
      SharedPreferences.Editor editor = settings.edit();
      editor.putInt("scale", scale);
      editor.putBoolean("comments", comments);
      editor.putBoolean("nightmode", nightmode);
      editor.putBoolean("fullscreen", fullscreen);
      editor.putBoolean("screenlock", screenlock);
      editor.putBoolean("translation_nvg", translation_nvg);
      editor.commit();
      history.sync();
    }

    protected void onStop(){
      scale = (int)(wv.getScale()*100);
      wv.setInitialScale(scale);
      // Log.v("svpismo", "onStop " + scale);
      syncPreferences();
      super.onStop();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
      getMenuInflater().inflate(R.menu.toolbar_menu, menu);
      return true;
    }

    void updateMenuItemSwitch(MenuItem item, boolean value) {
      if (item == null) return;
      CompoundButton s = (CompoundButton)MenuItemCompat.getActionView(item);
      if (s == null) return;
      s.setChecked(value);
    }

    public void updateMenu() {
      if (navigationView == null) return;
      Menu menu = navigationView.getMenu();
      if (menu == null) return;

      MenuItem drawer_item = menu.findItem(R.id.nightmode_toggle);
      MenuItem action_item = toolbar.getMenu().findItem(R.id.nightmode_toggle_toolbar);

      if (nightmode) {
        updateMenuItemSwitch(drawer_item, true);
        if (action_item != null) {
          action_item.setTitle(R.string.nightmode_off);
          action_item.setIcon(R.drawable.ic_wb_sunny_white_24dp);
        }
      } else {
        updateMenuItemSwitch(drawer_item, false);
        if (action_item != null) {
          action_item.setTitle(R.string.nightmode_on);
          action_item.setIcon(R.drawable.ic_brightness_3_white_24dp);
        }
      }

      drawer_item = menu.findItem(R.id.fullscreen_toggle);
      action_item = null;
      //action_item = toolbar.getMenu().findItem(R.id.fullscreen_toggle_toolbar);

      if (fullscreen) {
        updateMenuItemSwitch(drawer_item, true);
        if (action_item != null) {
          action_item.setTitle(R.string.fullscreen_off);
          //action_item.setIcon(R.drawable.ic_wb_sunny_white_24dp);
        }
      } else {
        updateMenuItemSwitch(drawer_item, false);
        if (action_item != null) {
          action_item.setTitle(R.string.fullscreen_on);
          //action_item.setIcon(R.drawable.ic_brightness_3_white_24dp);
        }
      }

      drawer_item = menu.findItem(R.id.comments_toggle);
      action_item = null;
      //action_item = toolbar.getMenu().findItem(R.id.comments_toggle_toolbar);

      if (comments) {
        updateMenuItemSwitch(drawer_item, true);
        if (action_item != null) {
          action_item.setTitle(R.string.comments_off);
          //action_item.setIcon(R.drawable.ic_wb_sunny_white_24dp);
        }
      } else {
        updateMenuItemSwitch(drawer_item, false);
        if (action_item != null) {
          action_item.setTitle(R.string.comments_on);
          //action_item.setIcon(R.drawable.ic_brightness_3_white_24dp);
        }
      }

      drawer_item = menu.findItem(R.id.screenlock_toggle);
      action_item = null;
      //action_item = toolbar.getMenu().findItem(R.id.screenlock_toggle_toolbar);

      if (screenlock) {
        updateMenuItemSwitch(drawer_item, true);
        if (action_item != null) {
          action_item.setTitle(R.string.screenlock_off);
          //action_item.setIcon(R.drawable.ic_wb_sunny_white_24dp);
        }
      } else {
        updateMenuItemSwitch(drawer_item, false);
        if (action_item != null) {
          action_item.setTitle(R.string.screenlock_on);
          //action_item.setIcon(R.drawable.ic_brightness_3_white_24dp);
        }
      }

      drawer_item = menu.findItem(R.id.translation_toggle);
      action_item = null;
      //action_item = toolbar.getMenu().findItem(R.id.translation_toggle_toolbar);

      if (translation_nvg) {
        updateMenuItemSwitch(drawer_item, true);
        if (action_item != null) {
          action_item.setTitle(R.string.translation_ssv);
          //action_item.setIcon(R.drawable.ic_wb_sunny_white_24dp);
        }
      } else {
        updateMenuItemSwitch(drawer_item, false);
        if (action_item != null) {
          action_item.setTitle(R.string.translation_nvg);
          //action_item.setIcon(R.drawable.ic_brightness_3_white_24dp);
        }
      }
    }


    void toggleNightMode() {
      nightmode = !nightmode;
      syncPreferences();
      load();
      updateMenu();
    }

    void toggleComments() {
      comments = !comments;
      syncPreferences();
      load();
      updateMenu();
    }

    void toggleFullscreen() {
      fullscreen = !fullscreen;
      updateFullscreen();
      syncPreferences();
      updateMenu();
    }

    void toggleScreenlock() {
      screenlock = !screenlock;
      if (screenlock) {
        lock.acquire();
      } else {
        lock.release();
      }
      syncPreferences();
      syncPreferences();
      updateMenu();
    }

    void toggleTranslation() {
      translation_nvg = !translation_nvg;
      if (translation_nvg) {
        setTranslationNvg();
      } else {
        setTranslationSsv();
      }
      syncPreferences();
      load();
      updateMenu();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
      // Handle item selection
      switch (item.getItemId()) {
        case R.id.toc_toolbar:
          loadUrl("pismo.cgi");
          return true;
        case R.id.nightmode_toggle_toolbar:
          toggleNightMode();
          return true;
        case R.id.bookmarks_toolbar:
	  Intent i = new Intent(this, Bookmarks.class);
	  i.putExtra("location", history.getCurrent());
	  i.putExtra("position", wv.getScrollY() / (float)wv.getContentHeight());
	  startActivityForResult(i, Bookmarks.BOOKMARKS);
          return true;
          /*
        case R.id.comments_toggle_toolbar:
          toggleComments();
          return true;
        case R.id.fullscreen_toggle_toolbar:
          toggleFullscreen();
          return true;
        case R.id.screenlock_toggle_toolbar:
          toggleScreenlock();
          return true;
        case R.id.translation_toggle_toolbar:
          toggleTranslation();
          return true;
          */
        default:
          return super.onOptionsItemSelected(item);
      }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
      if ((keyCode == KeyEvent.KEYCODE_BACK) && canGoBack()) {
        goBack();
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
        long css_len, String querystring, boolean comments, boolean broken_kitkat);

    public native void setTranslationSsv();
    public native void setTranslationNvg();

    static {
        System.loadLibrary("pismo");
    }

    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
      switch (item.getItemId()) {
        case R.id.toc:
          loadUrl(toc_url);
          break;
        case R.id.comments_toggle:
          toggleComments();
          break;
        case R.id.nightmode_toggle:
          toggleNightMode();
          break;
        case R.id.fullscreen_toggle:
          toggleFullscreen();
          break;
        case R.id.screenlock_toggle:
          toggleScreenlock();
          break;
        case R.id.translation_toggle:
          toggleTranslation();
          break;
        case R.id.bookmarks:
	  Intent i = new Intent(this, Bookmarks.class);
	  i.putExtra("location", history.getCurrent());
	  i.putExtra("position", wv.getScrollY() / (float)wv.getContentHeight());
	  startActivityForResult(i, Bookmarks.BOOKMARKS);
          break;
      }
      drawer.closeDrawer(GravityCompat.START);
      return true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
      switch (requestCode) {
        case Bookmarks.BOOKMARKS:
	  if (resultCode == RESULT_OK) {
            scroll_to = data.getFloatExtra("position", 0);
            loadUrl(data.getStringExtra("location"));
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
        toolbar.setVisibility(View.GONE);
        params.flags |= WindowManager.LayoutParams.FLAG_FULLSCREEN;
      } else {
        findViewById(R.id.navbar).setVisibility(View.VISIBLE);
        toolbar.setVisibility(View.VISIBLE);
        params.flags &= ~WindowManager.LayoutParams.FLAG_FULLSCREEN;
      }
      getWindow().setAttributes(params);
    }

    @Override
    protected void onResume() {
      if (screenlock) {
        lock.acquire();
      }
      super.onResume();
    }

    @Override
    protected void onPause() {
      super.onPause();
      if (screenlock) {
        lock.release();
      }
    }

    @Override
    public boolean onDoubleTap(android.view.MotionEvent e) {
      toggleFullscreen();
      updateMenu();
      return true;
    }

    @Override
    public boolean onDoubleTapEvent(android.view.MotionEvent e) {
      return false;
    }

    @Override
    public boolean onSingleTapConfirmed(android.view.MotionEvent e) {
      return false;
    }

    @Override
    public boolean dispatchTouchEvent(android.view.MotionEvent e) {
      //scale_gesture_detector.onTouchEvent(e);
      tap_gesture_detector.onTouchEvent(e);
      return super.dispatchTouchEvent(e);
    }

}
