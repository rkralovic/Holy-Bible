package sk.ksp.riso.svpismo;

import java.net.Socket;
import java.net.ServerSocket;
import java.io.*;
import java.lang.Thread;
import android.content.Context;
import android.content.res.*;
import android.util.Log;
import sk.ksp.riso.svpismo.Copy;
import java.lang.InterruptedException;
import java.nio.*;
import java.nio.channels.*;

public class Server extends Thread
{
    public int port;
    ServerSocket listener;
    Context ctx;
    boolean running;
    static String scriptname;

    MappedByteBuffer db;
    long db_len;

    public Server(Context _ctx, String sn) throws IOException {
      int i;
      boolean ok = true;
      scriptname = sn;
      ctx = _ctx;
      for (i=50000; i>20000; i--) {
        ok = true;
        try {
          listener = new ServerSocket(i);
        } catch (IOException e) {
          ok = false;
        }
        if (ok) break;
      }
      if (!ok) throw new IOException("cannot allocate port for webserver");
      port = i;
      running = true;
      setDaemon(true);

      {
        AssetFileDescriptor dbf = ctx.getAssets().openFd("pismo.bin");
        FileInputStream fis = dbf.createInputStream();
        FileChannel channel = fis.getChannel();
        db = channel.map(FileChannel.MapMode.READ_ONLY, dbf.getStartOffset(), dbf.getLength());
        db_len = dbf.getLength();
      }

    }

    public void run() {
      while (running) {
        try {
          Socket client = listener.accept();
          handle(client);
          client.close();
        } catch (IOException e) {
          Log.v("SvPismo: Server:", "run failed: " + e.getMessage());
        }
      }
    }

    public void stopServer() {
      boolean intr;

      running = false;
      try {
        listener.close();
        do {
          intr = false;
          try { this.join(); } catch (java.lang.InterruptedException e) { intr = true; }
        } while (intr);
      } catch (IOException e) {
        Log.v("SvPismo: Server:", "stopServer failed: " + e.getMessage());
      }
    }

    synchronized void handle(Socket client) throws IOException {
      String dokument = "unknown";
      BufferedReader in = new BufferedReader(new InputStreamReader(client.getInputStream()));
      byte[] buf;

      boolean postmethod = false;
      int cntlen=0;
      while (true) {
        String s = in.readLine();
        if (s == null) break;
        s = s.trim();

        if (s.equals("")) break;

        if (s.substring(0, 3).equals("GET")) {
          int leerstelle = s.indexOf(" HTTP/");
          dokument = s.substring(5,leerstelle);
          dokument = dokument.replaceAll("[/]+","/");
        } else if (s.substring(0, 4).equals("POST")) {
          int leerstelle = s.indexOf(" HTTP/");
          dokument = s.substring(6,leerstelle);
          dokument = dokument.replaceAll("[/]+","/");
          postmethod = true;
        } else if (s.substring(0, 14).equals("Content-Length")) {
          cntlen = Integer.parseInt(s.substring(16));
        }
      }
/* // we do not need it now
      if (postmethod) {
        char[] buf2 = new char[cntlen];
        in.read(buf2, 0, cntlen);
        buf = new String(buf2).getBytes();
      } else {
        cntlen = 0;
        buf = new byte[0];
      }
*/

      // Log.v("SvPismo", "document = " + dokument);

      if (dokument.length() >=scriptname.length() && 
          dokument.substring(0,scriptname.length()).equals(scriptname)) {
        // Log.v("SvPismo", "handling cgi request");
        client.getOutputStream().write(
            (
            "HTTP/1.1 200 OK\n" +
            "Server: SvPismo\n" +
            "Connection: close\n"
            ).getBytes("UTF-8")
        );

        String qs = dokument.substring(scriptname.length()+1);
        client.getOutputStream().write( process(db, db_len, qs).getBytes("UTF-8") );
        client.getOutputStream().close();
      } else {
        try {
          InputStream infile = ctx.getAssets().open(dokument, AssetManager.ACCESS_STREAMING);
          client.getOutputStream().write(
              (
              "HTTP/1.1 200 OK\n" +
              "Server: SvPismo\n" +
              "Connection: close\n\n"
              ).getBytes("UTF-8")
          );
          // do not start new thread, just copy here.
          new Copy( infile, client.getOutputStream() ).run();
        } catch (IOException e) {
          client.getOutputStream().write(
              (
              "HTTP/1.1 404 - File Not Found\n" +
              "Server: SvPismo\n" +
              "Connection: close\n"
              ).getBytes("UTF-8")
          );
        }
      }
    }

    static {
        System.loadLibrary("pismo");
    }

    public native String process(ByteBuffer db, long db_len, String querystring);

}
