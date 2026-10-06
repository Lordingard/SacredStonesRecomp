package android.window;

public interface OnBackInvokedDispatcher {
    int PRIORITY_DEFAULT = 0;
    void registerOnBackInvokedCallback(int priority, OnBackInvokedCallback callback);
    void unregisterOnBackInvokedCallback(OnBackInvokedCallback callback);
}
