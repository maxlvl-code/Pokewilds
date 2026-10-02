package com.maxlvl.pokewildsandroid;

import android.app.Presentation;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.view.Display;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.Space;
import android.widget.Spinner;
import android.widget.TextView;

import com.badlogic.gdx.backends.android.AndroidApplication;
import com.badlogic.gdx.backends.android.AndroidApplicationConfiguration;
import com.badlogic.gdx.controllers.Controller;
import com.badlogic.gdx.controllers.ControllerMapping;
import com.pkmngen.game.Game;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.Field;

public class MainActivity extends AndroidApplication implements DisplayManager.DisplayListener {
    private DisplayManager displayManager;
    private ThorPresentation thorPresentation;
    private final String[] fallbackLocalAssets = {"cloud5.png", "stars1.png", "island1.png"};

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        System.setProperty("user.dir", getFilesDir().getAbsolutePath());
        seedLocalFallbackAssets();

        AndroidApplicationConfiguration cfg = new AndroidApplicationConfiguration();
        cfg.useAccelerometer = false;
        cfg.useCompass = false;
        cfg.useGyroscope = false;
        cfg.useImmersiveMode = true;
        cfg.useWakelock = true;
        initialize(new Game(), cfg);

        displayManager = (DisplayManager)getSystemService(Context.DISPLAY_SERVICE);
        displayManager.registerDisplayListener(this, null);
        getWindow().getDecorView().postDelayed(this::attachThorScreenIfAvailable, 500);
        getWindow().getDecorView().postDelayed(this::applySavedMapping, 1000);
    }

    @Override protected void onResume() {
        super.onResume();
        if (getWindow() != null) {
            getWindow().getDecorView().postDelayed(this::attachThorScreenIfAvailable, 300);
            getWindow().getDecorView().postDelayed(this::applySavedMapping, 700);
        }
    }

    @Override protected void onDestroy() {
        if (displayManager != null) displayManager.unregisterDisplayListener(this);
        if (thorPresentation != null) thorPresentation.dismiss();
        super.onDestroy();
    }

    @Override public void onDisplayAdded(int displayId) { attachThorScreenIfAvailable(); }
    @Override public void onDisplayRemoved(int displayId) { attachThorScreenIfAvailable(); }
    @Override public void onDisplayChanged(int displayId) { }

    private void seedLocalFallbackAssets() {
        for (String name : fallbackLocalAssets) {
            File out = new File(getFilesDir(), name);
            if (out.exists()) continue;
            try (InputStream in = getAssets().open(name);
                 OutputStream os = new FileOutputStream(out)) {
                byte[] b = new byte[64 * 1024];
                int n;
                while ((n = in.read(b)) > 0) os.write(b, 0, n);
            } catch (IOException ignored) { }
        }
        new File(getFilesDir(), "mods").mkdirs();
    }

    private Display findSecondaryDisplay() {
        if (displayManager == null) return null;
        Display[] presentationDisplays =
                displayManager.getDisplays(DisplayManager.DISPLAY_CATEGORY_PRESENTATION);
        for (Display d : presentationDisplays) {
            if (d.getDisplayId() != Display.DEFAULT_DISPLAY) return d;
        }
        for (Display d : displayManager.getDisplays()) {
            if (d.getDisplayId() != Display.DEFAULT_DISPLAY) return d;
        }
        return null;
    }

    private void attachThorScreenIfAvailable() {
        Display d = findSecondaryDisplay();
        if (d == null) {
            if (thorPresentation != null) {
                thorPresentation.dismiss();
                thorPresentation = null;
            }
            return;
        }
        if (thorPresentation != null &&
            thorPresentation.getDisplay().getDisplayId() == d.getDisplayId()) return;

        if (thorPresentation != null) thorPresentation.dismiss();
        thorPresentation = new ThorPresentation(this, d);
        thorPresentation.show();
    }

    void sendVirtualKey(int androidKeyCode, boolean down) {
        long now = android.os.SystemClock.uptimeMillis();
        KeyEvent event = new KeyEvent(
                now, now,
                down ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP,
                androidKeyCode, 0);
        super.dispatchKeyEvent(event);
    }

    private SharedPreferences prefs() {
        return getSharedPreferences("thor_controls", MODE_PRIVATE);
    }

    void saveMapping(String a, String b, int deadzonePct) {
        prefs().edit()
                .putString("a", a)
                .putString("b", b)
                .putInt("deadzone", deadzonePct)
                .apply();
        applySavedMapping();
    }

    private int mappedButton(ControllerMapping m, String label) {
        switch (label) {
            case "B": return m.buttonB;
            case "X": return m.buttonX;
            case "Y": return m.buttonY;
            default: return m.buttonA;
        }
    }

    void applySavedMapping() {
        try {
            Controller c = Game.gamepad;
            if (c == null) return;

            ControllerMapping m = c.getMapping();
            String a = prefs().getString("a", "A");
            String b = prefs().getString("b", "B");
            int dead = prefs().getInt("deadzone", 25);

            Class<?> ip = Class.forName("com.pkmngen.game.InputProcessor");
            int ai = mappedButton(m, a);
            int bi = mappedButton(m, b);
            setStatic(ip, "gamepadA1", ai);
            setStatic(ip, "gamepadA2", ai);
            setStatic(ip, "gamepadB1", bi);
            setStatic(ip, "gamepadB2", bi);

            Field dz = ip.getField("gamepadMinAxis");
            dz.setDouble(null, Math.max(0.05, Math.min(0.95, dead / 100.0)));

            if (thorPresentation != null) thorPresentation.updateControllerText();
        } catch (Throwable ignored) { }
    }

    private static void setStatic(Class<?> c, String name, int value) throws Exception {
        Field f = c.getField(name);
        f.setInt(null, value);
    }

    String controllerSummary() {
        Controller c = Game.gamepad;
        return c == null ? "Controller: none detected" : "Controller: " + c.getName();
    }

    final class ThorPresentation extends Presentation {
        TextView status;
        Spinner aMap;
        Spinner bMap;
        SeekBar deadzone;

        ThorPresentation(Context context, Display display) {
            super(context, display);
        }

        @Override protected void onCreate(Bundle b) {
            super.onCreate(b);
            Window w = getWindow();
            if (w != null) {
                w.addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE |
                           WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                w.setNavigationBarColor(Color.rgb(9, 14, 10));
            }
            setContentView(buildRoot());
        }

        private View buildRoot() {
            LinearLayout root = new LinearLayout(getContext());
            root.setOrientation(LinearLayout.VERTICAL);
            root.setPadding(24, 18, 24, 18);
            root.setBackgroundColor(Color.rgb(12, 18, 13));

            root.addView(text("POKÉWILDS • AYN THOR CONTROL DECK", 20, true),
                    new LinearLayout.LayoutParams(-1, -2));
            status = text(controllerSummary(), 14, false);
            root.addView(status, new LinearLayout.LayoutParams(-1, -2));

            LinearLayout body = new LinearLayout(getContext());
            body.setOrientation(LinearLayout.HORIZONTAL);
            root.addView(body, new LinearLayout.LayoutParams(-1, 0, 1f));

            body.addView(buildDpad(), new LinearLayout.LayoutParams(0, -1, 0.42f));
            body.addView(buildActions(), new LinearLayout.LayoutParams(0, -1, 0.28f));
            body.addView(buildMapper(), new LinearLayout.LayoutParams(0, -1, 0.30f));
            return root;
        }

        private View buildDpad() {
            GridLayout g = new GridLayout(getContext());
            g.setRowCount(3);
            g.setColumnCount(3);
            addBlank(g);
            addKey(g, "▲", KeyEvent.KEYCODE_DPAD_UP);
            addBlank(g);
            addKey(g, "◀", KeyEvent.KEYCODE_DPAD_LEFT);
            addBlank(g);
            addKey(g, "▶", KeyEvent.KEYCODE_DPAD_RIGHT);
            addBlank(g);
            addKey(g, "▼", KeyEvent.KEYCODE_DPAD_DOWN);
            addBlank(g);
            return g;
        }

        private View buildActions() {
            LinearLayout l = new LinearLayout(getContext());
            l.setOrientation(LinearLayout.VERTICAL);
            l.setGravity(Gravity.CENTER);

            LinearLayout ab = new LinearLayout(getContext());
            ab.setGravity(Gravity.CENTER);
            addKey(ab, "B", KeyEvent.KEYCODE_X);
            addKey(ab, "A", KeyEvent.KEYCODE_Z);
            l.addView(ab);

            LinearLayout lr = new LinearLayout(getContext());
            lr.setGravity(Gravity.CENTER);
            addKey(lr, "L", KeyEvent.KEYCODE_C);
            addKey(lr, "R", KeyEvent.KEYCODE_V);
            l.addView(lr);

            addKey(l, "START", KeyEvent.KEYCODE_ENTER);
            return l;
        }

        private View buildMapper() {
            LinearLayout l = new LinearLayout(getContext());
            l.setOrientation(LinearLayout.VERTICAL);
            l.setPadding(12, 4, 4, 4);

            l.addView(text("Physical button mapping", 16, true));
            String[] opts = {"A", "B", "X", "Y"};
            ArrayAdapter<String> adapter = new ArrayAdapter<>(
                    getContext(),
                    android.R.layout.simple_spinner_dropdown_item,
                    opts);

            l.addView(text("PokéWilds A", 13, false));
            aMap = new Spinner(getContext());
            aMap.setAdapter(adapter);
            aMap.setSelection(indexOf(opts, prefs().getString("a", "A")));
            l.addView(aMap);

            l.addView(text("PokéWilds B", 13, false));
            bMap = new Spinner(getContext());
            bMap.setAdapter(adapter);
            bMap.setSelection(indexOf(opts, prefs().getString("b", "B")));
            l.addView(bMap);

            l.addView(text("Analog deadzone", 13, false));
            deadzone = new SeekBar(getContext());
            deadzone.setMax(70);
            deadzone.setProgress(Math.max(0, prefs().getInt("deadzone", 25) - 10));
            l.addView(deadzone);

            Button apply = button("APPLY MAPPING");
            apply.setOnClickListener(v -> saveMapping(
                    (String)aMap.getSelectedItem(),
                    (String)bMap.getSelectedItem(),
                    deadzone.getProgress() + 10));
            l.addView(apply);

            l.addView(text(
                    "Top: game • Bottom: controls/menu\n" +
                    "Built-in D-pad, sticks, Start and shoulder buttons use LibGDX's Android controller backend.",
                    11, false));
            return l;
        }

        void updateControllerText() {
            if (status != null) status.setText(controllerSummary());
        }

        private TextView text(String s, int sp, boolean bold) {
            TextView v = new TextView(getContext());
            v.setText(s);
            v.setTextColor(Color.WHITE);
            v.setTextSize(sp);
            v.setPadding(8, 5, 8, 5);
            if (bold) v.setTypeface(v.getTypeface(), android.graphics.Typeface.BOLD);
            return v;
        }

        private Button button(String s) {
            Button b = new Button(getContext());
            b.setText(s);
            b.setTextSize(16);
            b.setMinHeight(72);
            return b;
        }

        private void addBlank(GridLayout g) {
            Space s = new Space(getContext());
            g.addView(s, new GridLayout.LayoutParams(
                    GridLayout.spec(GridLayout.UNDEFINED, 1, 1f),
                    GridLayout.spec(GridLayout.UNDEFINED, 1, 1f)));
        }

        private void addKey(android.view.ViewGroup parent, String label, int code) {
            Button b = button(label);
            b.setOnTouchListener((v, e) -> {
                if (e.getActionMasked() == MotionEvent.ACTION_DOWN) sendVirtualKey(code, true);
                else if (e.getActionMasked() == MotionEvent.ACTION_UP ||
                         e.getActionMasked() == MotionEvent.ACTION_CANCEL) sendVirtualKey(code, false);
                return true;
            });
            if (parent instanceof GridLayout) {
                ((GridLayout) parent).addView(b, new GridLayout.LayoutParams(
                        GridLayout.spec(GridLayout.UNDEFINED, 1, 1f),
                        GridLayout.spec(GridLayout.UNDEFINED, 1, 1f)));
            } else {
                parent.addView(b, new LinearLayout.LayoutParams(0, -2, 1f));
            }
        }

        private int indexOf(String[] arr, String value) {
            for (int i = 0; i < arr.length; i++) if (arr[i].equals(value)) return i;
            return 0;
        }
    }
}
