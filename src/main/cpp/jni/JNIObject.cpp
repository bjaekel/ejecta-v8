//
// Created by Martin Kleinhans on 18.04.17.
//

#include "jni_assert.h"

#include "JNIObject.h"
#include "JNIWrapper.h"

BGJS_JNI_LINK(JNIObject, "ag/boersego/bgjs/JNIObject");

JNIObject::JNIObject(jobject obj, JNIClassInfo *info) : JNIBase(info) {
    // handling of shared_ptr/weak_ptr as well as the jobject references has to be threadsafe
    // we do NOT need a recursive mutex, we prefer the possible small performance advantage
    // this is not a problem, because a deadlock is impossible the way the mutex is used
    _mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&_mutex, &Attr);

    JNIEnv* env = JNIWrapper::getEnvironment();
    if(info->type == JNIObjectType::kPersistent) {
        // persistent objects are owned by the java side: they are destroyed once the java side is garbage collected
        // => as long as there are no references to the c object, the java reference is weak.
        _jniObjectWeak = env->NewWeakGlobalRef(obj);
        _jniObject = nullptr;
    } else {
        // non-persistent objects are owned by the c side. they do not exist in this form on the java side
        // => as long as the object exists, the java reference should always be strong
        // theoretically we could use the same logic here, and make it non-weak on demand, but it simply is not necessary
        _jniObject = env->NewGlobalRef(obj);
        _jniObjectWeak = nullptr;
    }
    _atomicJniObjectRefCount = 0;

    // store pointer to native instance in "nativeHandle" field
    // actually type will never be kAbstract here, because JNIClassInfo will be provided for the subclass!
    // however, this gets rid of the "never used" warning for the constant, and works just fine as well.
    if(info->type != JNIObjectType::kTemporary) {
        JNIClassInfo *info = _jniClassInfo;
        while(info && info->baseClassInfo) info = info->baseClassInfo;
        auto it = info->fieldMap.at("nativeHandle");
        env->SetLongField(getJObject(), it.id, reinterpret_cast<jlong>(this));
    }
}

JNIObject::~JNIObject() {
    JNI_ASSERT(_atomicJniObjectRefCount==0, "JNIObject was deleted while retaining java object");
    if(_jniObject) {
        // this should/can never happen for persistent objects
        // if there is a strong ref to the JObject, then the native object must not be deleted!
        // it can however happen for non-persistent objects!
        JNIWrapper::getEnvironment()->DeleteGlobalRef(_jniObject);
    } else if(_jniObjectWeak) {
        JNIWrapper::getEnvironment()->DeleteWeakGlobalRef(_jniObjectWeak);
    }
    _jniObjectWeak = _jniObject = nullptr;
    pthread_mutex_destroy(&_mutex);
}

void JNIObject::initializeJNIBindings(JNIClassInfo *info, bool isReload) {
    info->registerNativeMethod("RegisterClass", "(Ljava/lang/String;Ljava/lang/String;)V", (void*)JNIObject::jniRegisterClass);
}

const jobject JNIObject::getJObject() {
    jobject obj;
    JNIEnv *env = JNIWrapper::getEnvironment();

    // this method is only ever called from a shared_ptr, so we can access the _jni* members without
    // mutexes, because we know they will not change as long as the shared_ptr exists,
    // because the shared_ptr holds a reference to them via retainJObject

    // we always need to convert the object reference to local (both weak and global)
    // because the global reference could be deleted when the last shared ptr to this instance is released
    // e.g. "return instance->getJObject();" would then yield an invalid global ref!
    if(_jniObject) {
        obj = env->NewLocalRef(_jniObject);
    } else {
        obj = env->NewLocalRef(_jniObjectWeak);
    }

    return obj;
}

std::shared_ptr<JNIObject> JNIObject::getSharedPtr() {
    std::shared_ptr<JNIObject> ptr;

    // a private internal weak ptr is used as a basis for "synchronizing" creation of shared_ptr
    // all shared_ptr created from the weak_ptr use the same counter and deallocator!
    // theoretically we could use multiple separate instance with the retain/release logic, but this way we stay more flexible

    // the lock method atomically checks if weakPtr is still valid, and if yes returns a shared_ptr
    ptr = _weakPtr.lock();
    if(ptr) return ptr;

    // if _weakPtr is empty, we need to create a new one in a thread-safe way
    pthread_mutex_lock(&_mutex);

    // if another thread was waiting for the mutex while the ptr was being created, it might be available now!
    // we still need to use .lock here, because we don't know how long the shared_ptr lives..
    ptr = _weakPtr.lock();

    if(!ptr) {
        if (isPersistent()) {
            // we are handing out a reference to the native object here
            // as long as that reference is alive, the java object must not be garbabe collected
            retainJObject();
        }
        ptr = std::shared_ptr<JNIObject>(this, [=](JNIObject *cls) {
            if (!cls->isPersistent()) {
                // non persistent objects need to be deleted once they are not referenced anymore
                // wrapping an object again will return a new native instance!
                delete cls;
            } else {
                // ownership of persistent objects is held by the java side
                // object is only deleted if the java object is garbage collected
                // when there are no more references from C we make the reference to the java object weak again!

                // we are using a mutex here so that the release call does not interfere with possible
                // parallel creation of a new shared_ptr
                // it does NOT matter if we release before we retain in this case, because either way in the end the counter
                // will be one (1 -> 0 -> 1; 1 -> 2 -> 1)
                pthread_mutex_lock(&cls->_mutex);
                cls->releaseJObject();
                pthread_mutex_unlock(&cls->_mutex);
            }
        });
        _weakPtr = ptr;
    }

    pthread_mutex_unlock(&_mutex);

    return ptr;
}

void JNIObject::retainJObject() {
    // this is always called from a shared_ptr, which ensures that the ref count is at least 1
    // except for the initial incrementation which is protected via mutex in getSharedPtr
    // additional calls are made thread-safe by the atomic counter
    JNI_ASSERT(isPersistent(), "Attempt to retain non-persistent native object");
    if(_atomicJniObjectRefCount++ == 0) {
        JNIEnv *env = JNIWrapper::getEnvironment();
        _jniObject = env->NewGlobalRef(_jniObjectWeak);
    }
}

void JNIObject::releaseJObject() {
    // this is always called from a shared_ptr, which ensures that the ref count is at least 1
    // except for the final decrementation which is protected via mutex in getSharedPtr
    // additional calls are made thread-safe by the atomic counter
    JNI_ASSERT(isPersistent(), "Attempt to release non-persistent native object");
    if(--_atomicJniObjectRefCount == 0) {
        JNIEnv *env = JNIWrapper::getEnvironment();

        // this method might be executed automatically if a shared_ptr falls of the stack
        // if an exception was thrown after the shared_ptr was created, we can
        // not call any JNI functions without clearing the exception first and then rethrowing it
        // In most cases this could be done in the method itself, but it is tedious and likely to be forgotten
        // also, there are cases
        // e.g. when an exception is thrown in a method that is not immediately called from Java/JNI, but the shared_ptr is used in a method further upp the call stack
        // when it is very hard or even impossible to do this
        // => handle this here!
        jthrowable exc = nullptr;
        if(env->ExceptionCheck()) {
            exc = env->ExceptionOccurred();
            env->ExceptionClear();
        }

        env->DeleteGlobalRef(_jniObject);
        _jniObject = nullptr;

        if(exc) env->Throw(exc);
    }
}

//--------------------------------------------------------------------------------------------------
// Methods
//--------------------------------------------------------------------------------------------------
#define METHOD(TypeName, JNITypeName) \
JNITypeName JNIObject::callJava##TypeName##Method(const char* name, ...) {\
    JNIEnv* env = JNIWrapper::getEnvironment();\
    auto it = _jniClassInfo->methodMap.find(name);\
    JNI_ASSERTF(it != _jniClassInfo->methodMap.end(), "Attempt to call unregistered method '%s'", name);\
    JNI_ASSERTF(!it->second.isStatic, "Attempt to call non-static method '%s' as static", name);\
    va_list args;\
    JNITypeName res;\
    va_start(args, name);\
    res = env->Call##TypeName##MethodV(getJObject(), it->second.id, args);\
    va_end(args);\
    return res;\
}

void JNIObject::callJavaVoidMethod(const char* name, ...) {
    JNIEnv* env = JNIWrapper::getEnvironment();
    auto it = _jniClassInfo->methodMap.find(name);
    JNI_ASSERTF(it != _jniClassInfo->methodMap.end(), "Attempt to call unregistered method '%s'", name);
    JNI_ASSERTF(!it->second.isStatic, "Attempt to call non-static method '%s' as static", name);
    va_list args;
    va_start(args, name);\
    env->CallVoidMethodV(getJObject(), it->second.id, args);
    va_end(args);
}

METHOD(Long, jlong)
METHOD(Boolean, jboolean)
METHOD(Byte, jbyte)
METHOD(Char, jchar)
METHOD(Double, jdouble)
METHOD(Float, jfloat)
METHOD(Int, jint)
METHOD(Short, jshort)
METHOD(Object, jobject)

void JNIObject::jniRegisterClass(JNIEnv *env, jobject obj, jstring derivedClass, jstring baseClass) {
    std::string strDerivedClass = JNIWrapper::jstring2string(derivedClass);
    std::replace(strDerivedClass.begin(), strDerivedClass.end(), '.', '/');

    std::string strBaseClass = JNIWrapper::jstring2string(baseClass);
    std::replace(strBaseClass.begin(), strBaseClass.end(), '.', '/');

    JNIWrapper::registerJavaObject(strDerivedClass, strBaseClass);
}

//--------------------------------------------------------------------------------------------------
// Exports
//--------------------------------------------------------------------------------------------------
extern "C" {
    JNIEXPORT void JNICALL Java_ag_boersego_bgjs_JNIObject_initNative(JNIEnv *env, jobject obj, jstring canonicalName) {
        JNIWrapper::initializeNativeObject(obj, canonicalName);
    }

    JNIEXPORT bool JNICALL Java_ag_boersego_bgjs_JNIObjectReference_disposeNative(JNIEnv *env, jobject obj, jlong nativeHandle) {
        JNIObject *jniObject = reinterpret_cast<JNIObject*>(nativeHandle);
        delete jniObject;
        return true;
    }
}

