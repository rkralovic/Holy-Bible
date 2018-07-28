package sk.ksp.riso.svpismo;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;
import java.util.ArrayDeque;

public class History {
  Db dbHelper;
  SQLiteDatabase db;

  static final int MAX_HISTORY = 1000;
  ArrayDeque<String> history;
  ArrayDeque<String> forward_history;

  History(Context ctx) {
    dbHelper = new Db(ctx);
    db = dbHelper.getWritableDatabase();

    history = readTable(Db.HISTORY_TABLE);
    forward_history = readTable(Db.HISTORY_TABLE_FWD);
  }

  ArrayDeque<String> readTable(String table) {
    ArrayDeque<String> data = new ArrayDeque<String>();
    Cursor c = db.rawQuery("select url from " + table +
                           " order by position asc", null);
    while (c.moveToNext()) {
      String url = c.getString(0);
      data.addLast(c.getString(0));
      Log.v("svpismo", table + " -> " + url);
    }
    return data;
  }

  void writeTable(String table, ArrayDeque<String> data) {
    db.execSQL("delete from " + table);
    int i = 0;
    for (String url: data) {
      ++i;
      ContentValues v = new ContentValues();
      v.put("position", i);
      v.put("url", url);
      db.insert(table, null, v);
      Log.v("svpismo", table + " <- " + url);
    }
  }

  public void sync() {
    Log.v("svpismo", "Sync history");
    db.beginTransaction();
    writeTable(Db.HISTORY_TABLE, history);
    writeTable(Db.HISTORY_TABLE_FWD, forward_history);
    db.setTransactionSuccessful();
    db.endTransaction();
  }

  public boolean isEmpty() {
    return history.isEmpty();
  }

  public boolean isLast() {
    return forward_history.isEmpty();
  }

  public void goForward() {
    if (isLast()) return;
    history.addLast(forward_history.pollLast());
  }

  public String getCurrent() {
    try {
      return history.getLast();
    } catch (java.util.NoSuchElementException e) {
      return "pismo.cgi";
    }
  }

  public void pop() {
    if (history.isEmpty()) return;
    forward_history.addLast(history.pollLast());
  }

  public void push(String url) {
    history.addLast(url);
    if (!forward_history.isEmpty()) {
      forward_history = new ArrayDeque<String>();
    }
    if (history.size() > MAX_HISTORY) {
      history.pollFirst();
    }
  }
}
