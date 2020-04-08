#ifndef __BGJSGLVIEW_H
#define __BGJSGLVIEW_H	1

#include "BGJSCanvasContext.h"
#include "BGJSV8Engine.h"
#include "../v8/JNIV8Object.h"
#include "os-android.h"

/**
 * BGJSGLView
 * Wrapper class around native windows that expose OpenGL operations
 *
 * Copyright 2014 Kevin Read <me@kevin-read.com> and BörseGo AG (https://github.com/godmodelabs/ejecta-v8/)
 * Licensed under the MIT license.
 */

class BGJSGLView : public JNIScope<BGJSGLView, JNIV8Object> {
public:
	BGJSGLView(jobject obj, JNIClassInfo *info) : JNIScope(obj, info) {};

    static void setViewData(JNIEnv *env, jobject objWrapped, float density, bool doNoClearOnFlip, int width, int height);
	virtual void onSetViewData(float density, bool doNoClearOnFlip, int width, int height);

	static bool isWrappableV8Object(v8::Local<v8::Object> object);
	static void initializeJNIBindings(JNIClassInfo *info, bool isReload);
	static void initializeV8Bindings(JNIV8ClassInfo *info);

    virtual ~BGJSGLView();
    static void prepareRedraw(JNIEnv *env, jobject objWrapped);
    virtual void onPrepareRedraw();
    static void endRedraw(JNIEnv *env, jobject objWrapped);
    virtual void onEndRedraw();

    static void viewWasResized(JNIEnv *env, jobject objWrapped, int width, int height);

	static void setTouchPosition(JNIEnv *env, jobject objWrapped, int x, int y);
    virtual void onSetTouchPosition(int x, int y);
    void swapBuffers();

	BGJSCanvasContext *context2d;

    int getWidth();
    int getHeight();
    float getDensity();

protected:
    bool noFlushOnRedraw = false;
    bool noClearOnFlip = false;

	float _density = 0;
    int _width = 0;
    int _height = 0;
};

BGJS_JNI_LINK_DEF(BGJSGLView)

#endif
