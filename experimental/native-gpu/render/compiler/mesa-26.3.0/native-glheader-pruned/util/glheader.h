/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2022  Yongang Luo       All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file glheader.h
 * Wrapper for GL/gl*.h and GLES[3|2|]/gl*.h
 */


#ifndef GLHEADER_H
#define GLHEADER_H


#define GL_GLEXT_PROTOTYPES
#define GL_GLEXT_LEGACY
#if defined(_WIN32) && !defined(__CYGWIN__)
/* Prevent glheader.h from including <windows.h> by defining APIENTRY */
#pragma push_macro("APIENTRY")
#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif
#include "GL/gl.h"
#pragma pop_macro("APIENTRY")
#else /* !(defined(_WIN32) && !defined(__CYGWIN__)) */
#include "GL/gl.h"
#endif /* defined(_WIN32) && !defined(__CYGWIN__) */
#include "gl_ext_constants.h"

/**
 * Define GL_API, GL_APICALL and GL_APIENTRY to avoid MSVC/MinGW warnings
 * about different dllimport attributes for prototypes between
 * GL/gl*.h and GLES[|3|2]/gl*.h
 */
#define GL_API GLAPI
#define GL_APICALL GLAPI
#define GL_APIENTRY GLAPIENTRY

/**
 * The order for including GLES[|3|2]/gl*.h headers are from newest to oldest.
 * As the newer header contains extra symbols that are not present in the
 * older header, some extra symbols can be visible only when you include the
 * newer header first; otherwise, if the older header is included first, some
 * extra symbols will be hidden by the older header.
 * For example, suppose we move the inclusion of GLES/gl*.h to the front,
 * then GL_SAMPLER_EXTERNAL_OES will not be present and cause compiling error.
 */


/* Native Vulkan compiler needs these two GLES enum values, not
 * the full GLES declarations. Values: Khronos GLES2/gl2ext.h. */
#define GL_SAMPLER_EXTERNAL_OES 0x8D66
#define GL_SAMPLER_EXTERNAL_2D_Y2Y_EXT 0x8BE7

#ifdef __cplusplus
extern "C" {
#endif


/* Custom Mesa types to save space. */
typedef unsigned char GLenum8; /* only for primitive modes */
typedef unsigned short GLenum16;
typedef unsigned char GLbitfield8;
typedef unsigned short GLbitfield16;
typedef unsigned long long GLuint64;
typedef GLuint64 GLbitfield64;

/* This is for packing the GL index type. The reason it's a structure is that
 * we don't want it to be used as integer accidentally.
 */
typedef struct {
   unsigned char value;
} GLindextype;

/* There is no formal spec for the following extension. */
#ifndef GL_ATI_texture_compression_3dc
#define GL_ATI_texture_compression_3dc                          1
#define GL_COMPRESSED_LUMINANCE_ALPHA_3DC_ATI                   0x8837
#endif

/**
 * Internal token to represent a GLSL shader program (a collection of
 * one or more shaders that get linked together).  Note that GLSL
 * shaders and shader programs share one name space (one hash table)
 * so we need a value that's different from any of the
 * GL_VERTEX/FRAGMENT/GEOMETRY_PROGRAM tokens.
 */
#define GL_SHADER_PROGRAM_MESA                                  0x9999

#ifdef __cplusplus
}
#endif

#endif /* GLHEADER_H */
