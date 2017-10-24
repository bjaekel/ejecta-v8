#ifndef __CLIENTANDROID_H
#define __CLIENTANDROID_H	1

#include <jni.h>
#include "ClientAbstract.h"

/**
 * ClientAndroid
 * The bridge between v8 and Android.
 *
 * Copyright 2014 Kevin Read <me@kevin-read.com> and BörseGo AG (https://github.com/godmodelabs/ejecta-v8/)
 *
 */

class ClientAndroid : public ClientAbstract {
public:
	JNIEnv* envCache;
	JavaVM *cachedJVM;
	jobject assetManager;
	jclass bgjsPushHelper;
	jclass bgjsWebPushHelper;
	jclass chartingV8Engine;
	jmethodID bgjsPushUnsubscribeMethod, bgjsPushSubscribeMethod;
    jmethodID bgjsWebPushSubscribeMethod;
    jmethodID bgjsWebPushSubUnsubscribeMethod;
	jmethodID v8EnginegetIAPState;

	const char* loadFile (const char* path, unsigned int* length = NULL);
	void on (const char* event, void* cbPtr, void *thisObjPtr);
	~ClientAndroid();

	unsigned char *_magnifierImage;
	unsigned int _magnifierWidth;
	unsigned int _magnifierHeight;
	unsigned int _magnifierTexWidth;
	unsigned int _magnifierTexHeight;
	unsigned char *_maskImage;
};

// http://stackoverflow.com/questions/5991615/unable-to-get-jnienv-value-in-arbitrary-context
JNIEnv* JNU_GetEnv();

#endif
