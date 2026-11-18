package org.urbanchaos.launcher;
final class NativeGame {
    private static boolean loaded;
    static synchronized void load(){if(!loaded){System.loadLibrary("SDL3");System.loadLibrary("main");loaded=true;}}
    static native String validate(String path);
    static native void menuButton(int key,boolean down);
}
