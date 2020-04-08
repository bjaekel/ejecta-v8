//
// Created by Martin Kleinhans on 18.04.17.
//

#ifndef __JNIOBJECT_H
#define __JNIOBJECT_H

#import <string>
#include <jni.h>
#include <mutex>
#include "JNIBase.h"

/**
 * Base class for all native classes associated with a java object
 * constructor should never be called manually; if you want to create a new instance
 * use JNIWrapper::createObject<Type>(env) instead
 */
class JNIObject : public JNIBase {
    friend class JNIWrapper;
    template <typename> friend class JNIRef;
public:
    JNIObject(jobject obj, JNIClassInfo *info);
    virtual ~JNIObject();

    /**
     * checks if the java object is currently
     */
    bool isRetained() const;

    /**
     * returns the referenced java object
     */
    const jobject getJObject() const;

    /**
     * calls the specified java object method
     */
    void callJavaVoidMethod(const char* name, ...);
    jlong callJavaLongMethod(const char* name, ...);
    jboolean callJavaBooleanMethod(const char* name, ...);
    jbyte callJavaByteMethod(const char* name, ...);
    jchar callJavaCharMethod(const char* name, ...);
    jdouble callJavaDoubleMethod(const char* name, ...);
    jfloat callJavaFloatMethod(const char* name, ...);
    jint callJavaIntMethod(const char* name, ...);
    jshort callJavaShortMethod(const char* name, ...);
    jobject callJavaObjectMethod(const char* name, ...);

protected:
    void retainJObject();
    void releaseJObject();

private:
    static void initializeJNIBindings(JNIClassInfo *info, bool isReload);
    static void jniRegisterClass(JNIEnv *env, jobject obj, jstring derivedClass, jstring baseClass);

    std::mutex _mutex;
    //pthread_mutex_t _mutex;
    jobject _jniObject;
    jweak _jniObjectWeak;
    std::atomic<uint8_t> _atomicJniObjectRefCount;
    std::weak_ptr<JNIObject> _weakPtr;
};

BGJS_JNI_LINK_DEF(JNIObject)

#endif //__OBJECT_H
