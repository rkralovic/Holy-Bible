package sk.ksp.riso.svpismo;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.content.Context;

class Db extends SQLiteOpenHelper {
  static final String BOOKMARKS_TABLE = "bookmarks";
  static final String HISTORY_TABLE = "history";
  static final String HISTORY_TABLE_FWD = "history_fwd";

  Db(Context ctx) {
    super(ctx, "svpismo", null, 2);
  }

  void createBookmarks(SQLiteDatabase db) {
    db.execSQL("CREATE TABLE " + BOOKMARKS_TABLE + 
               " ( label text, location text, position float, stamp bigint )");
  }

  void createHistory(SQLiteDatabase db) {
    db.execSQL("CREATE TABLE " + HISTORY_TABLE + " ( position int, url text )");
    db.execSQL("CREATE TABLE " + HISTORY_TABLE_FWD + " ( position int, url text )");
  }

  public void onCreate(SQLiteDatabase db) {
    createBookmarks(db);
    createHistory(db);
  }

  public void onUpgrade(SQLiteDatabase db, int oldV, int newV) {
    if (oldV < 2) {
      db.execSQL("drop table if exists " + HISTORY_TABLE);
      db.execSQL("drop table if exists " + HISTORY_TABLE_FWD);
      if (newV >= 2) {
        createHistory(db);
      }
    }
  }
}
