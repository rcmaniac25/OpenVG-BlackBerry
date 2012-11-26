/*------------------------------------------------------------------------
 *
 * EGL 1.3
 * -------
 *
 * Copyright (c) 2007 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and /or associated documentation files
 * (the "Materials "), to deal in the Materials without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Materials,
 * and to permit persons to whom the Materials are furnished to do so,
 * subject to the following conditions: 
 *
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Materials. 
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR
 * THE USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 *//**
 * \file
 * \brief	QNX specific EGL functionality
 * \note
  *//*-------------------------------------------------------------------*/

#include "../riImage.h"
#include <EGLimp/egl.h>
#if defined(GL_BACKEND)
#include <math.h>
#include "eglref.h"
#else
#include <stdlib.h>
#endif
#include <screen/screen.h>
#include <pthread.h>
#include <errno.h>

namespace OpenVGRI
{

/*-------------------------------------------------------------------*//*!
* \brief	
* \param	
* \return	
* \note		
*//*-------------------------------------------------------------------*/

void* OSGetCurrentThreadID(void)
{
	return (void*)pthread_self();   //TODO this is not safe
}

/*-------------------------------------------------------------------*//*!
* \brief	
* \param	
* \return	
* \note		
*//*-------------------------------------------------------------------*/

static pthread_mutex_t mutex;
static int mutexRefCount = 0;
static bool mutexInitialized = false;
//acquired mutex cannot be deinited
void OSDeinitMutex(void)
{
	RI_ASSERT(mutexInitialized);
	RI_ASSERT(mutexRefCount == 0);
	int ret = pthread_mutex_destroy(&mutex);
	RI_ASSERT(ret != EINVAL);	//assert that the mutex has been initialized
	RI_ASSERT(ret != EAGAIN);	//assert that the maximum number of recursive locks hasn't been exceeded
	RI_ASSERT(!ret);	//check that there aren't other errors
	RI_UNREF(ret);
}
void OSAcquireMutex(void)
{
	if(!mutexInitialized)
    {
        int ret;
        pthread_mutexattr_t attr;
        ret = pthread_mutexattr_init(&attr);	//initially not locked
        RI_ASSERT(!ret);	//check that there aren't any errors
        ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);	//count the number of recursive locks
        RI_ASSERT(!ret);	//check that there aren't any errors
        ret = pthread_mutex_init(&mutex, &attr);
        pthread_mutexattr_destroy(&attr);
        RI_ASSERT(!ret);	//check that there aren't more errors
        RI_UNREF(ret);
        mutexInitialized = true;
    }
	int ret = pthread_mutex_lock(&mutex);
	RI_ASSERT(ret != EINVAL);	//assert that the mutex has been initialized
	RI_ASSERT(ret != EAGAIN);	//assert that the maximum number of recursive locks hasn't been exceeded
	RI_ASSERT(ret != EDEADLK);	//recursive mutexes shouldn't return this
	RI_ASSERT(!ret);	//check that there aren't other errors
	RI_UNREF(ret);
	mutexRefCount++;
}
void OSReleaseMutex(void)
{
	RI_ASSERT(mutexInitialized);
	mutexRefCount--;
	RI_ASSERT(mutexRefCount >= 0);
	int ret = pthread_mutex_unlock(&mutex);
	RI_ASSERT(ret != EPERM);	//assert that the current thread owns the mutex
	RI_ASSERT(!ret);	//check that there aren't more errors
	RI_UNREF(ret);
}

/*-------------------------------------------------------------------*//*!
* \brief	
* \param	
* \return	
* \note		
*//*-------------------------------------------------------------------*/

static bool isBigEndian()
{
	static const RIuint32 v = 0x12345678u;
	const RIuint8* p = (const RIuint8*)&v;
	RI_ASSERT (*p == (RIuint8)0x12u || *p == (RIuint8)0x78u);
	return (*p == (RIuint8)(0x12)) ? true : false;
}


/*-------------------------------------------------------------------*//*!
* \brief	
* \param	
* \return	
* \note		
*//*-------------------------------------------------------------------*/

#if defined(GL_BACKEND)
#include <GLES/gl.h>
//#include <GLES/glext.h>
#endif

struct OSWindowContext
{
#if defined(GL_BACKEND)
	EGLDisplay 			eglDisp;
	EGLSurface			eglSurf;
	EGLContext			eglCtx;

	GLuint				tex;
#endif

	screen_window_t		window;
#if defined(GL_BACKEND)
    unsigned int*		tmp;
    int					tmpWidth;
    int					tmpHeight;
#else
    screen_buffer_t		pixelbuffer;
    int					pixelBufferStride;
    int					rect[4];
#endif
};

void* OSCreateWindowContext(EGLNativeWindowType window)
{
    OSWindowContext* ctx = NULL;
    try
    {
        ctx = RI_NEW(OSWindowContext, ());
    }
	catch(std::bad_alloc&)
	{
		return NULL;
	}

#if defined(GL_BACKEND)
	if(!eglSetup(&(ctx->eglDisp), &(ctx->eglSurf), &(ctx->eglCtx), window))
	{
		RI_DELETE(ctx);
		return NULL;
	}
#endif

    ctx->window = (screen_window_t)window;
#if defined(GL_BACKEND)
    ctx->tmp = NULL;
    ctx->tmpWidth = 0;
    ctx->tmpHeight = 0;
    ctx->tex = 0;
#else
    screen_context_t context = NULL;
    if(screen_get_window_property_pv(ctx->window, SCREEN_PROPERTY_CONTEXT, (void**)&context) != 0)
    {
    	RI_DELETE(ctx);
		return NULL;
    }
    ctx->rect[0] = 0;
    ctx->rect[1] = 0;
    if(screen_get_window_property_iv(ctx->window, SCREEN_PROPERTY_BUFFER_SIZE, ctx->rect + 2) != 0)
    {
    	RI_DELETE(ctx);
		return NULL;
    }

    //Create the pixmap
    screen_pixmap_t pixmap;
    if (screen_create_pixmap(&pixmap, context) != 0)
    {
    	RI_DELETE(ctx);
		return NULL;
	}

	// A combination of write and native usage is necessary to blit the pixmap to screen.
	int usage = SCREEN_USAGE_WRITE | SCREEN_USAGE_NATIVE;
	if(screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_USAGE, &usage) != 0)
	{
		screen_destroy_pixmap(pixmap);
		RI_DELETE(ctx);
		return NULL;
	}

	int format = SCREEN_FORMAT_RGBX8888;
	if (screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_BUFFER_SIZE, ctx->rect + 2) != 0 || screen_set_pixmap_property_iv(pixmap, SCREEN_PROPERTY_FORMAT, &format) != 0)
	{
		screen_destroy_pixmap(pixmap);
		RI_DELETE(ctx);
		return NULL;
	}

	if (screen_create_pixmap_buffer(pixmap) != 0)
	{
		screen_destroy_pixmap(pixmap);
		RI_DELETE(ctx);
		return NULL;
	}
	if (screen_get_pixmap_property_pv(pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)&(ctx->pixelbuffer)) != 0)
	{
		screen_destroy_pixmap(pixmap);
		RI_DELETE(ctx);
		return NULL;
	}

	// We get the stride (the number of bytes between pixels on different rows), its used
	// later on when we perform the rendering to the pixmap buffer.
	if (screen_get_buffer_property_iv(ctx->pixelbuffer, SCREEN_PROPERTY_STRIDE, &(ctx->pixelBufferStride)) != 0)
	{
		screen_destroy_pixmap(pixmap);
		RI_DELETE(ctx);
		return NULL;
	}
#endif
    return ctx;
}

void OSDestroyWindowContext(void* context)
{
    OSWindowContext* ctx = (OSWindowContext*)context;
    if(ctx)
    {
#if defined(GL_BACKEND)
    	glDeleteTextures(1, &(ctx->tex));
    	if (ctx->eglDisp != EGL_NO_DISPLAY)
    	{
			eglMakeCurrent_impl(ctx->eglDisp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if (ctx->eglSurf != EGL_NO_SURFACE)
			{
				eglDestroySurface_impl(ctx->eglDisp, ctx->eglSurf);
				ctx->eglSurf = EGL_NO_SURFACE;
			}
			if (ctx->eglCtx != EGL_NO_CONTEXT)
			{
				eglDestroyContext_impl(ctx->eglDisp, ctx->eglCtx);
				ctx->eglCtx = EGL_NO_CONTEXT;
			}
			eglTerminate_impl(ctx->eglDisp);
			ctx->eglDisp = EGL_NO_DISPLAY;
		}
		eglReleaseThread_impl();
		eglCleanup();
        RI_DELETE_ARRAY(ctx->tmp);
#else
        //Everything gets cleaned up when the screen context is destroyed
#endif
        RI_DELETE(ctx);
    }
}

bool OSIsWindow(const void* context)
{
    OSWindowContext* ctx = (OSWindowContext*)context;
    int tmp;
    if(ctx)
    {
        if(ctx->window)
        	return screen_get_window_property_iv(ctx->window, SCREEN_PROPERTY_VISIBLE, &tmp) == 0;
    }
    return false;
}

void OSGetWindowSize(const void* context, int& width, int& height)
{
    OSWindowContext* ctx = (OSWindowContext*)context;
    if(ctx)
    {
    	int size[2];
    	screen_get_window_property_iv(ctx->window, SCREEN_PROPERTY_BUFFER_SIZE, size);
    	width = size[0];
    	height = size[1];
    }
    else
    {
        width = 0;
        height = 0;
    }
}

void OSBlitToWindow(void* context, const Drawable* drawable)
{
    OSWindowContext* ctx = (OSWindowContext*)context;
    if(ctx)
    {
        int w = drawable->getWidth();
        int h = drawable->getHeight();

        //NOTE: we assume here that the display is always in sRGB color space
		VGImageFormat f = VG_sXBGR_8888;
		if(isBigEndian())
			f = VG_sRGBX_8888;

#if defined(GL_BACKEND)
        eglMakeCurrent_impl(ctx->eglDisp, ctx->eglSurf, ctx->eglSurf, ctx->eglCtx);

#if !defined(GL_OES_texture_npot)
		int ow = w;
		int oh = h;

		//Calculate next power of 2 (from Clemens: http://acius2.blogspot.com/2007/11/calculating-next-power-of-2.html, I'm pretty sure there is a way to do this without converting to floating point)
		w = h = 1 << (int)ceil(log2(fmax(w, h)));
#endif
        if(!ctx->tmp || ctx->tmpWidth != w || ctx->tmpHeight != h)
        {
            RI_DELETE_ARRAY(ctx->tmp);
            ctx->tmp = NULL;
            try
            {
                ctx->tmp = RI_NEW_ARRAY(unsigned int, w*h);	//throws bad_alloc
                ctx->tmpWidth = w;
                ctx->tmpHeight = h;
            }
            catch(std::bad_alloc&)
            {
                //do nothing
            }
        }

        if(ctx->tmp)
        {
        	glViewport(0, 0, w, h);
            glDisable(GL_DEPTH_TEST);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            vgReadPixels(ctx->tmp, w*sizeof(unsigned int), f, 0, 0,
#if defined(GL_OES_texture_npot)
				w, h);
#else
            	ow, oh);
#endif

			if(ctx->tex == 0)
			{
				//Generate texture
				glGenTextures(1, &(ctx->tex));
			}
            glBindTexture(GL_TEXTURE_2D, ctx->tex);

            //Allocate the texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ctx->tmp);
            if(glGetError() == GL_NO_ERROR)
            {
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				//Setup GLES
				static const GLfloat plane[12] = {-1.0f, -1.0f,  1.0f,
						 1.0f, -1.0f,  1.0f,
						-1.0f,  1.0f,  1.0f,
						 1.0f,  1.0f,  1.0f};
				static const GLfloat planeUV[8] = {0.0f, 0.0f,
						 1.0f, 0.0f,
						 0.0f, 1.0f,
						 1.0f, 1.0f};

				glEnable(GL_TEXTURE_2D);
				glVertexPointer(3, GL_FLOAT, 0, plane);
				glTexCoordPointer(2, GL_FLOAT, 0, planeUV);
				glEnableClientState(GL_VERTEX_ARRAY);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);

				glShadeModel(GL_SMOOTH);

				//Draw texture
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				glFinish();

				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisableClientState(GL_VERTEX_ARRAY);
				glDisable(GL_TEXTURE_2D);
            }
        }

        eglSwapBuffers_impl(ctx->eglDisp, ctx->eglSurf);
#else
        //XXX Format is BGR and is flipped vertically.

        //Prepare for blit and get pointer
        int noiseBlitParameters[] = { SCREEN_BLIT_SOURCE_WIDTH, ctx->rect[2], SCREEN_BLIT_SOURCE_HEIGHT, ctx->rect[3], SCREEN_BLIT_END };
        void* ptr = NULL;
        screen_context_t context = NULL;
		if(screen_get_window_property_pv(ctx->window, SCREEN_PROPERTY_CONTEXT, (void**)&context) == 0 &&
				screen_get_buffer_property_pv(ctx->pixelbuffer, SCREEN_PROPERTY_POINTER, (void **)&ptr) == 0)
		{
			//Get buffers
			int count = 0;
			screen_buffer_t* screen_bufs = NULL;
			if(screen_get_window_property_iv(ctx->window, SCREEN_PROPERTY_BUFFER_COUNT, &count) == 0 && count > 0)
			{
				screen_bufs = (screen_buffer_t*)calloc(count, sizeof(screen_buffer_t));
				if(screen_bufs)
				{
					if(screen_get_window_property_pv(ctx->window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)screen_bufs) == 0)
					{
						//Get data
						vgReadPixels(ptr, ctx->pixelBufferStride, f, 0, 0, w, h);

						//Blit
						screen_blit(context, screen_bufs[0], ctx->pixelbuffer, noiseBlitParameters);
						screen_post_window(ctx->window, screen_bufs[0], 1, ctx->rect, 0);
					}
					free(screen_bufs);
				}
			}
		}
#endif
    }
}

EGLDisplay OSGetDisplay(EGLNativeDisplayType display_id)
{
    RI_UNREF(display_id);
    return (EGLDisplay)1;    //support only a single display
}

}   //namespace OpenVGRI
