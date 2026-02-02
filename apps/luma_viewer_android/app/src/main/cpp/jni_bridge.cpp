// LUMA Viewer Android - JNI Bridge
// Connects Kotlin/Java code to native Vulkan renderer

#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include "vulkan_renderer.h"

#define LOG_TAG "LumaJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

// Initialize Vulkan renderer with native window
JNIEXPORT jlong JNICALL
Java_com_luma_viewer_MainActivity_nativeInit(JNIEnv *env, jobject thiz, jobject surface, jint width, jint height) {
    LOGI("nativeInit called: %dx%d", width, height);
    
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window from surface");
        return 0;
    }
    
    auto* renderer = new luma::VulkanRenderer();
    if (!renderer->initialize(window, width, height)) {
        LOGE("Failed to initialize Vulkan renderer");
        delete renderer;
        ANativeWindow_release(window);
        return 0;
    }
    
    LOGI("Vulkan renderer initialized successfully");
    return reinterpret_cast<jlong>(renderer);
}

// Destroy renderer
JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeDestroy(JNIEnv *env, jobject thiz, jlong handle) {
    LOGI("nativeDestroy called");
    
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->shutdown();
        delete renderer;
    }
}

// Resize
JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeResize(JNIEnv *env, jobject thiz, jlong handle, jint width, jint height) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->resize(width, height);
    }
}

// Render frame
JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeRender(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->render();
    }
}

// Camera controls
JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeOrbit(JNIEnv *env, jobject thiz, jlong handle, jfloat deltaX, jfloat deltaY) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->orbit(deltaX, deltaY);
    }
}

JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativePan(JNIEnv *env, jobject thiz, jlong handle, jfloat deltaX, jfloat deltaY) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->pan(deltaX, deltaY);
    }
}

JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeZoom(JNIEnv *env, jobject thiz, jlong handle, jfloat scaleFactor) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->zoom(scaleFactor);
    }
}

JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeResetCamera(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->resetCamera();
    }
}

// Model loading
JNIEXPORT jboolean JNICALL
Java_com_luma_viewer_MainActivity_nativeLoadModel(JNIEnv *env, jobject thiz, jlong handle, jstring path) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        const char* pathStr = env->GetStringUTFChars(path, nullptr);
        bool result = renderer->loadModel(pathStr);
        env->ReleaseStringUTFChars(path, pathStr);
        return result ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

// Settings
JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeToggleGrid(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->toggleGrid();
    }
}

JNIEXPORT void JNICALL
Java_com_luma_viewer_MainActivity_nativeToggleAutoRotate(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        renderer->toggleAutoRotate();
    }
}

// Info
JNIEXPORT jstring JNICALL
Java_com_luma_viewer_MainActivity_nativeGetInfo(JNIEnv *env, jobject thiz, jlong handle) {
    if (handle != 0) {
        auto* renderer = reinterpret_cast<luma::VulkanRenderer*>(handle);
        std::string info = renderer->getInfo();
        return env->NewStringUTF(info.c_str());
    }
    return env->NewStringUTF("No renderer");
}

} // extern "C"
