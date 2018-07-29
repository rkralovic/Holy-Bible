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

  static class Entry {
    public Entry(String url) {
      this.url = url;
      this.scroll = -1;
    }

    public Entry(String url, float scroll) {
      this.url = url;
      this.scroll = scroll;
    }

    public String url;
    public float scroll;
  }

  Entry default_entry;

  static final int MAX_HISTORY = 1000;
  ArrayDeque<Entry> history;
  ArrayDeque<Entry> forward_history;
  int marker;

  History(Context ctx) {
    dbHelper = new Db(ctx);
    db = dbHelper.getWritableDatabase();

    history = readTable(Db.HISTORY_TABLE);
    forward_history = readTable(Db.HISTORY_TABLE_FWD);
    default_entry = new Entry("pismo.cgi", -1);
    marker = -1;
  }

  ArrayDeque<Entry> readTable(String table) {
    ArrayDeque<Entry> data = new ArrayDeque<Entry>();
    Cursor c = db.rawQuery("select url, scroll from " + table +
                           " order by position asc", null);
    while (c.moveToNext()) {
      data.addLast(new Entry(c.getString(0), c.getFloat(1)));
    }
    return data;
  }

  void writeTable(String table, ArrayDeque<Entry> data) {
    db.execSQL("delete from " + table);
    int i = 0;
    for (Entry e : data) {
      ++i;
      ContentValues v = new ContentValues();
      v.put("position", i);
      v.put("url", e.url);
      v.put("scroll", e.scroll);
      db.insert(table, null, v);
    }
  }

  public void sync() {
    //Log.v("svpismo", "Sync history");
    db.beginTransaction();
    writeTable(Db.HISTORY_TABLE, history);
    writeTable(Db.HISTORY_TABLE_FWD, forward_history);
    db.setTransactionSuccessful();
    db.endTransaction();
  }

  public void setMark() {
    marker = 0;
  }

  public void clearMark() {
    marker = -1;
  }

  public boolean isAtMark() {
    return marker == 0;
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
    increaseMarker();
  }

  void increaseMarker() {
    if (marker >= 0) {
      marker++;
    }
    if (history.size() > MAX_HISTORY) {
      history.pollFirst();
    }
  }

  public Entry getCurrent() {
    try {
      return history.getLast();
    } catch (java.util.NoSuchElementException e) {
      return default_entry;
    }
  }

  public void setCurrentScroll(float scroll) {
    if (isEmpty()) return;
    getCurrent().scroll = scroll;
  }

  public void pop() {
    if (history.isEmpty()) return;
    forward_history.addLast(history.pollLast());
    if (marker >= 0) {
      marker--;
    }
    if (history.isEmpty()) {
      marker = -1;
    }
  }

  public void push(Entry entry) {
    history.addLast(entry);
    if (!forward_history.isEmpty()) {
      forward_history = new ArrayDeque<Entry>();
    }
    increaseMarker();
  }
}
