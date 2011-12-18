package sk.ksp.riso.svpismo;

import android.app.Activity;
import android.os.Bundle;
import android.os.Bundle;
import android.widget.*;
import android.view.View;
import android.content.Intent;
import android.content.ContentValues;
import android.database.sqlite.SQLiteDatabase;
import android.database.Cursor;

import android.util.Log;

import java.lang.Float;

import sk.ksp.riso.svpismo.Db;

public class Bookmarks extends Activity
{
    static final int BOOKMARKS = 1;

    SimpleCursorAdapter A;
    Db dbHelper;
    SQLiteDatabase db;

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
      super.onCreate(savedInstanceState);
      setContentView(R.layout.bookmarks);
      dbHelper = new Db(this);
      db = dbHelper.getWritableDatabase();
      final Bookmarks current_activity = this;
      ((Button)findViewById(R.id.new_bookmark)).setOnClickListener(new View.OnClickListener() {
        public void onClick(View v) {
	  Intent i = current_activity.getIntent();
	  ContentValues r = new ContentValues();
	  r.put("location", i.getStringExtra("location"));
	  r.put("position", i.getFloatExtra("position", 0));
	  r.put("stamp", System.currentTimeMillis());
	  // fixme
	  r.put("label", i.getStringExtra("location"));
	  db.insert(Db.BOOKMARKS_TABLE, null, r);
	  current_activity.bookmarkAdded();
	}
      });
    }

    public void bookmarkAdded() {
      setResult(RESULT_CANCELED, null);
      finish();
    }

    @Override
    public void onResume() {
    
      Cursor c = db.rawQuery("select label as _id, location " +
                             "  from " + Db.BOOKMARKS_TABLE + " order by stamp desc", null);

      if (A==null) {
        A = new SimpleCursorAdapter(this, R.layout.bookmarks_list, c, 
            new String[] { "_id" }, new int[] { R.id.listLabel } );
        A.setViewBinder ( new SimpleCursorAdapter.ViewBinder() {
          public boolean setViewValue (View view, Cursor cursor, int columnIndex) {
            if (columnIndex == 0) {
              ((TextView)view).setText(cursor.getString(columnIndex));
            }
            return true;
          }
        });
        ((ListView)findViewById(R.id.listBookmarks)).setAdapter(A);
      } else {
        A.changeCursor(c);
      }

      super.onResume();
    }

    @Override
    public void onDestroy() {
      db.close();
      super.onDestroy();
    }
}
