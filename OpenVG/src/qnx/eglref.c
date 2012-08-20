/*
 * eglref.c
 *
 *  Created on: Aug 19, 2012
 *      Author: Vincent Simonetti
 */

#include <dlfcn.h>

#include <EGL/egl.h>
#include "eglref.h"

//Define
#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//Macros
#define EGLRUN(x) if(eglInit()) \
{\
	return (x);\
}\
	return 0;

#define EGLINIT_GETPTR(x) (x##Ptr) = (x##Function)dlsym(library, #x);\
	if(!(x##Ptr))\
	{\
		goto INIT_ERROR;\
	}

#define EGLINIT_DEF_FIELD(x) x##Function x##Ptr = NULL;

//Function pointers
typedef EGLBoolean (*eglSwapBuffersFunction)(EGLDisplay dpy, EGLSurface surface);
typedef EGLBoolean (*eglMakeCurrentFunction)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
typedef EGLBoolean (*eglDestroySurfaceFunction)(EGLDisplay dpy, EGLSurface surface);
typedef EGLBoolean (*eglDestroyContextFunction)(EGLDisplay dpy, EGLContext ctx);
typedef EGLBoolean (*eglTerminateFunction)(EGLDisplay dpy);
typedef EGLBoolean (*eglReleaseThreadFunction)();
typedef EGLDisplay (*eglGetDisplayFunction)(EGLNativeDisplayType display_id);
typedef EGLBoolean (*eglInitializeFunction)(EGLDisplay dpy, EGLint *major, EGLint *minor);
typedef EGLBoolean (*eglBindAPIFunction)(EGLenum api);
typedef EGLBoolean (*eglChooseConfigFunction)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
typedef EGLContext (*eglCreateContextFunction)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
typedef EGLSurface (*eglCreateWindowSurfaceFunction)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
typedef EGLBoolean (*eglSwapIntervalFunction)(EGLDisplay dpy, EGLint interval);

//Variables
void* library = NULL;

EGLINIT_DEF_FIELD(eglSwapBuffers)
EGLINIT_DEF_FIELD(eglMakeCurrent)
EGLINIT_DEF_FIELD(eglDestroySurface)
EGLINIT_DEF_FIELD(eglDestroyContext)
EGLINIT_DEF_FIELD(eglTerminate)
EGLINIT_DEF_FIELD(eglReleaseThread)
EGLINIT_DEF_FIELD(eglGetDisplay)
EGLINIT_DEF_FIELD(eglInitialize)
EGLINIT_DEF_FIELD(eglBindAPI)
EGLINIT_DEF_FIELD(eglChooseConfig)
EGLINIT_DEF_FIELD(eglCreateContext)
EGLINIT_DEF_FIELD(eglCreateWindowSurface)
EGLINIT_DEF_FIELD(eglSwapInterval)

//Functions
int eglInit()
{
	if(library)
	{
		return TRUE;
	}
	library = dlopen("libEGL.so", RTLD_LAZY);
	if(library)
	{
		EGLINIT_GETPTR(eglSwapBuffers)
		EGLINIT_GETPTR(eglMakeCurrent)
		EGLINIT_GETPTR(eglDestroySurface)
		EGLINIT_GETPTR(eglDestroyContext)
		EGLINIT_GETPTR(eglTerminate)
		EGLINIT_GETPTR(eglReleaseThread)
		EGLINIT_GETPTR(eglGetDisplay)
		EGLINIT_GETPTR(eglInitialize)
		EGLINIT_GETPTR(eglBindAPI)
		EGLINIT_GETPTR(eglChooseConfig)
		EGLINIT_GETPTR(eglCreateContext)
		EGLINIT_GETPTR(eglCreateWindowSurface)
		EGLINIT_GETPTR(eglSwapInterval)
		return TRUE;

	INIT_ERROR:
		dlclose(library);
		library = NULL;
	}
	return FALSE;
}

EGLBoolean eglSwapBuffers_impl(EGLDisplay dpy, EGLSurface surface)
{
	EGLRUN(eglSwapBuffersPtr(dpy, surface))
}

EGLBoolean eglMakeCurrent_impl(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
	EGLRUN(eglMakeCurrentPtr(dpy, draw, read, ctx))
}

EGLBoolean eglDestroySurface_impl(EGLDisplay dpy, EGLSurface surface)
{
	EGLRUN(eglDestroySurfacePtr(dpy, surface))
}

EGLBoolean eglDestroyContext_impl(EGLDisplay dpy, EGLContext ctx)
{
	EGLRUN(eglDestroyContextPtr(dpy, ctx))
}

EGLBoolean eglTerminate_impl(EGLDisplay dpy)
{
	EGLRUN(eglTerminatePtr(dpy))
}

EGLBoolean eglReleaseThread_impl()
{
	EGLRUN(eglReleaseThreadPtr())
}

EGLBoolean eglSetup(EGLDisplay* display, EGLSurface* surface, EGLContext* context, EGLNativeWindowType window)
{
	if(eglInit())
	{
		*display = EGL_NO_DISPLAY;
		*surface = EGL_NO_SURFACE;
		*context = EGL_NO_CONTEXT;

		EGLConfig eglConf = NULL;
		EGLint num_configs;
		EGLint interval;

		EGLint attrib_list[]= { EGL_RED_SIZE,        8,
								EGL_GREEN_SIZE,      8,
								EGL_BLUE_SIZE,       8,
								EGL_ALPHA_SIZE,      8,
								//EGL_DEPTH_SIZE,     32,
								EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
								EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
								EGL_NONE};

		*display = eglGetDisplayPtr(EGL_DEFAULT_DISPLAY);
		if (*display == EGL_NO_DISPLAY)
		{
			return EGL_FALSE;
		}

		EGLBoolean rc = eglInitializePtr(*display, NULL, NULL);
		if (rc != EGL_TRUE)
		{
			return EGL_FALSE;
		}

		rc = eglBindAPIPtr(EGL_OPENGL_ES_API);
		if (rc != EGL_TRUE)
		{
			goto ERROR_DISPLAY;
		}

		if(!eglChooseConfigPtr(*display, attrib_list, &eglConf, 1, &num_configs))
		{
			goto ERROR_DISPLAY;
		}

		*context = eglCreateContextPtr(*display, eglConf, EGL_NO_CONTEXT, NULL);
		if (*context == EGL_NO_CONTEXT)
		{
			goto ERROR_DISPLAY;
		}

		*surface = eglCreateWindowSurfacePtr(*display, eglConf, window, NULL);
		if (*surface == EGL_NO_SURFACE)
		{
			goto ERROR_CONTEXT;
		}

		rc = eglMakeCurrentPtr(*display, *surface, *surface, *context);
		if (rc != EGL_TRUE)
		{
			goto ERROR_SURFACE;
		}

		interval = 1;
		rc = eglSwapIntervalPtr(*display, interval);
		if (rc != EGL_TRUE)
		{
			goto ERROR_CURRENT;
		}
		return EGL_TRUE;

	ERROR_CURRENT:
		eglMakeCurrentPtr(*display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	ERROR_SURFACE:
		eglDestroySurfacePtr(*display, *surface);
		*surface = EGL_NO_SURFACE;
	ERROR_CONTEXT:
		eglDestroyContextPtr(*display, *context);
		*context = EGL_NO_CONTEXT;
	ERROR_DISPLAY:
		eglTerminatePtr(*display);
		*display = EGL_NO_DISPLAY;
		eglReleaseThreadPtr();
		eglCleanup();
	}
	return EGL_FALSE;
}

void eglCleanup()
{
	if(library)
	{
		dlclose(library);
		library = NULL;
	}
}
