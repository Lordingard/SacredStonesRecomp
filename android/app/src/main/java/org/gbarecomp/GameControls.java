package org.gbarecomp;

public final class GameControls {
    private GameControls() {}
    public static native void requestBack();
    public static native void setDisplayCutoutInsets(int left, int top, int right, int bottom);
}
