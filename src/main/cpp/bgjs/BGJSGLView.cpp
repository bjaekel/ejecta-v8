//
//  BGJSGLView.m
//  pushlib
//
//  Created by Kevin Read
//
//

#include "BGJSGLView.h"
#include "BGJSCanvasContext.h"

#include "GLcompat.h"

#include <EGL/egl.h>
#include <string.h>

#include <v8.h>

#include "os-android.h"
#include "../jni/JNIWrapper.h"

#undef DEBUG
// #define DEBUG 1
//#undef DEBUG
// #define DEBUG_GL 0
#undef DEBUG_GL
#define LOG_TAG "BGJSGLView"

using namespace v8;

/**
 * BGJSGLView
 * Wrapper class around native windows that expose OpenGL operations
 *
 * Copyright 2014 Kevin Read <me@kevin-read.com> and BörseGo AG (https://github.com/godmodelabs/ejecta-v8/)
 * Licensed under the MIT license.
 */

static void checkGlError(const char* op) {
#ifdef DEBUG_GL
	for (GLint error = glGetError(); error; error = glGetError()) {
		LOGI("after %s() glError (0x%x)\n", op, error);
	}
#endif
}

BGJS_JNI_LINK(BGJSGLView, "ag/boersego/bgjs/BGJSGLView");

void BGJSGLView::initializeJNIBindings(JNIClassInfo *info, bool isReload) {
	info->registerNativeMethod("Create", "(Lag/boersego/bgjs/V8Engine)Lag/boersego/bgjs/JNIV8GenericObject;", (void*)BGJSGLView::jniCreate);
    info->registerNativeMethod("prepareRedraw", "()V", (void*)BGJSGLView::prepareRedraw);
    info->registerNativeMethod("endRedraw", "()V", (void*)BGJSGLView::endRedraw);
    info->registerNativeMethod("setTouchPosition", "(II)V", (void*)BGJSGLView::setTouchPosition);
    info->registerNativeMethod("setViewData", "(FZII)V", (void*)BGJSGLView::setViewData);
}

void BGJSGLView::initializeV8Bindings(JNIV8ClassInfo *info) {

}

jobject BGJSGLView::jniCreate(JNIEnv *env, jobject obj, jobject engineObj) {
	auto engine = JNIWrapper::wrapObject<BGJSV8Engine>(engineObj);

	v8::Isolate* isolate = engine->getIsolate();
	v8::Locker l(isolate);
	v8::Isolate::Scope isolateScope(isolate);
	v8::HandleScope scope(isolate);
	v8::Local<v8::Context> context = engine->getContext();
	v8::Context::Scope ctxScope(context);

	v8::Local<v8::Object> objRef;

	objRef = v8::Object::New(isolate);

	return JNIV8Wrapper::wrapObject<BGJSGLView>(objRef)->getJObject();
}

void BGJSGLView::setViewData(float pixelRatio, bool doNoClearOnFlip, int width, int height) {

	noFlushOnRedraw = false;
    noClearOnFlip = doNoClearOnFlip;

	const char* eglVersion = eglQueryString(eglGetCurrentDisplay(), EGL_VERSION);
	LOGD("egl version %s", eglVersion);
	// bzero (_frameRequests, sizeof(_frameRequests));

	context2d = new BGJSCanvasContext(width, height);
	context2d->backingStoreRatio = pixelRatio;
#ifdef DEBUG
	LOGI("pixel Ratio %f", pixelRatio);
#endif
	context2d->create();
}

BGJSGLView::~BGJSGLView() {
	if (context2d) {
		delete (context2d);
	}
}

void BGJSGLView::prepareRedraw() {
	context2d->startRendering();
}

static unsigned int nextPowerOf2(unsigned int n)
{
    unsigned int p = 1;
    if (n && !(n & (n - 1)))
        return n;

    while (p < n) {
        p <<= 1;
    }
    return p;
}

void BGJSGLView::endRedraw() {
	context2d->endRendering();
	if (!noFlushOnRedraw) {
	    this->swapBuffers();
	}
}

void BGJSGLView::setTouchPosition(int x, int y) {
    // A NOP. Subclasses might be interested in this though
}

void BGJSGLView::swapBuffers() {
	// At least HC on Tegra 2 doesn't like this
	EGLDisplay display = eglGetCurrentDisplay();
	EGLSurface surface = eglGetCurrentSurface(EGL_DRAW);
	eglSurfaceAttrib(display, surface, EGL_SWAP_BEHAVIOR, noClearOnFlip ? EGL_BUFFER_PRESERVED : EGL_BUFFER_DESTROYED);
	EGLBoolean res = eglSwapBuffers (display, surface);
	// LOGD("eglSwap %d", (int)res);
	// checkGlError("eglSwapBuffers");
}


