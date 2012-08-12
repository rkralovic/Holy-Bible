package sk.ksp.riso.svpismo;

import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;

public class Preferences extends PreferenceActivity {
  static final int PREFERENCES = 2;
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    PreferenceManager prefMgr = getPreferenceManager();
    prefMgr.setSharedPreferencesName(sk.ksp.riso.svpismo.svpismo.prefname);
    prefMgr.setSharedPreferencesMode(0);
    addPreferencesFromResource(R.xml.main_preferences);
  }
}
