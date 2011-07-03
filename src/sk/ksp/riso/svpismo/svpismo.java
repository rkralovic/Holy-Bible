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
import java.io.*;
import android.net.Uri;
import android.content.Intent;

import sk.ksp.riso.svpismo.Server;

public class svpismo extends Activity
{
    static final String TAG = "svpismo";
    static final String scriptname = "pismo.cgi";
    Server S = null;

    public WebView wv;

    public void back() {
      wv.goBack();
    }

    public void forward() {
      wv.goForward();
    }

    public void load(String qs) {
      String url = "http://localhost:" + S.port + "/" + scriptname + "?" + qs;
//      Log.v("SvPismo", "Opening " + url);
      wv.loadUrl(url);
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
          if (S==null) {
            S = new Server(this, scriptname);
            S.start();
          }

          Intent I = getIntent();
          if (I.getAction().equals("sk.ksp.riso.svpismo.action.SHOW")) {
            load(I.getData().getQuery());
          } else {
            if (wv.restoreState(savedInstanceState) == null)
              load("c=Jn1,1");
          }

        } catch (IOException e) {
          wv.loadData("Some problem. Cannot initialize webserver:" + e.getMessage(), "text/html", "utf-8");
        }

        wv.getSettings().setBuiltInZoomControls(true);
        wv.getSettings().setJavaScriptEnabled(true);
        wv.setWebViewClient(new WebViewClient() {
          @Override
          public boolean shouldOverrideUrlLoading(WebView view, String url) {
            view.loadUrl(url);
            return true;
          }
        } );

    }

    protected void onSaveInstanceState(Bundle outState) {
      wv.saveState(outState);
    }

    @Override
    public void onDestroy() {
      if (S != null) S.stopServer();
      super.onDestroy();
    }

}
