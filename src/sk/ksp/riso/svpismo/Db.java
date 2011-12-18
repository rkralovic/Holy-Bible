package sk.ksp.riso.svpismo;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.content.Context;

class Db extends SQLiteOpenHelper {
  static final String BOOKMARKS_TABLE = "bookmarks";
  Db(Context ctx) {
    super(ctx, "svpismo", null, 1);
  }

  public void onCreate(SQLiteDatabase db) {
    db.execSQL("CREATE TABLE " + BOOKMARKS_TABLE + 
               " ( label text, location text, position float, stamp bigint )");
  }

  public void onUpgrade(SQLiteDatabase db, int oldV, int newV) {
    db.execSQL("drop table if exists " + BOOKMARKS_TABLE);
    onCreate(db);
  }
}
