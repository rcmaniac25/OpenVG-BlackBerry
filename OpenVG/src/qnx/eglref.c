/*
 * eglref.c
 *
 *  Created on: Aug 19, 2012
 *      Author: Vincent Simonetti
 */

#include <EGL/egl.h>
#include "eglref.h"

EGLBoolean eglSwapBuffers_impl(EGLDisplay dpy, EGLSurface surface)
{
	return eglSwapBuffers(dpy, surface);
}

EGLBoolean eglMakeCurrent_impl(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	return eglMakeCurrent(dpy, draw, read, ctx);
}

EGLBoolean eglDestroySurface_impl(EGLDisplay dpy, EGLSurface surface)
{
	return eglDestroySurface(dpy, surface);
}

EGLBoolean eglDestroyContext_impl(EGLDisplay dpy, EGLContext ctx)
{
	return eglDestroyContext(dpy, ctx);
}

EGLBoolean eglTerminate_impl(EGLDisplay dpy)
{
	return eglTerminate(dpy);
}

EGLBoolean eglReleaseThread_impl()
{
	return eglReleaseThread();
}

#define NULL 0

EGLBoolean eglSetup(EGLDisplay* display, EGLSurface* surface, EGLContext* context, EGLNativeWindowType window)
{
	*display = EGL_NO_DISPLAY;
	*surface = EGL_NO_SURFACE;
	*context = EGL_NO_CONTEXT;

	EGLConfig eglConf;
	EGLint num_configs;
	EGLint interval;

	EGLint attrib_list[]= { EGL_RED_SIZE,        8,
							EGL_GREEN_SIZE,      8,
							EGL_BLUE_SIZE,       8,
							EGL_ALPHA_SIZE,      8,
							EGL_DEPTH_SIZE,     32,
							EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
							EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
							EGL_NONE};

	*display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (*display == EGL_NO_DISPLAY)
	{
		return EGL_FALSE;
	}

	EGLBoolean rc = eglInitialize(*display, NULL, NULL);
	if (rc != EGL_TRUE)
	{
		return EGL_FALSE;
	}

	rc = eglBindAPI(EGL_OPENGL_ES_API);
	if (rc != EGL_TRUE)
	{
		goto ERROR_DISPLAY;
	}

	if(!eglChooseConfig(*display, attrib_list, &eglConf, 1, &num_configs))
	{
		goto ERROR_DISPLAY;
	}

	*context = eglCreateContext(*display, eglConf, EGL_NO_CONTEXT, NULL);
	if (*context == EGL_NO_CONTEXT)
	{
		goto ERROR_DISPLAY;
	}

	*surface = eglCreateWindowSurface(*display, eglConf, window, NULL);
	if (*surface == EGL_NO_SURFACE)
	{
		goto ERROR_CONTEXT;
	}

	rc = eglMakeCurrent(*display, *surface, *surface, *context);
	if (rc != EGL_TRUE)
	{
		goto ERROR_SURFACE;
	}

	interval = 1;
	rc = eglSwapInterval(*display, interval);
	if (rc != EGL_TRUE)
	{
		goto ERROR_CURRENT;
	}
	return EGL_TRUE;

ERROR_CURRENT:
	eglMakeCurrent(*display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
ERROR_SURFACE:
	eglDestroySurface(*display, *surface);
	*surface = EGL_NO_SURFACE;
ERROR_CONTEXT:
	eglDestroyContext(*display, *context);
	*context = EGL_NO_CONTEXT;
ERROR_DISPLAY:
	eglTerminate(*display);
	*display = EGL_NO_DISPLAY;
	eglReleaseThread();
	return EGL_FALSE;
}
