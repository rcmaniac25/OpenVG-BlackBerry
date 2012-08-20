/*
 * eglref.h
 *
 *  Created on: Aug 19, 2012
 *      Author: Vincent Simonetti
 */

#ifndef EGLREF_H_
#define EGLREF_H_

//Main purpose for this is so that the "real" EGL can be used when we otherwise wouldn't be able to do anything

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __egl_h_
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLNativeWindowType;
#endif
#ifndef EGLBoolean
typedef unsigned int EGLBoolean;
#endif

EGLBoolean eglSwapBuffers_impl(EGLDisplay dpy, EGLSurface surface);
EGLBoolean eglMakeCurrent_impl(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLBoolean eglDestroySurface_impl(EGLDisplay dpy, EGLSurface surface);
EGLBoolean eglDestroyContext_impl(EGLDisplay dpy, EGLContext ctx);
EGLBoolean eglTerminate_impl(EGLDisplay dpy);
EGLBoolean eglReleaseThread_impl();
EGLBoolean eglSetup(EGLDisplay* display, EGLSurface* surface, EGLContext* context, EGLNativeWindowType window);
void eglCleanup();

#ifdef __cplusplus
}
#endif

#endif /* EGLREF_H_ */
