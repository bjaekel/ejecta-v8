package ag.boersego.bgjs;

import android.util.Log;

/**
 * Created by martin on 07.09.17.
 */

public class V8TestClass2 extends V8TestClass {
    public V8TestClass2(V8Engine engine) {
        super(engine);
        Log.d("V8TestClass2", "Constructor");
    }

    public V8TestClass2(V8Engine engine, long jsObjPtr) {
        super(engine, jsObjPtr);
        Log.d("V8TestClass2", "Constructor:");
    }

    public void test3(long testL, float testF) {
        Log.d("V8TestClass2", "Overwritten Hello:" + Long.toString(testL) + ":" + Float.toString(testF));
    }

    public int testLong;
    public static long staticField;
}
