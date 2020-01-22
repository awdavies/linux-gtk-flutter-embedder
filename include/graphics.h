// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef LINUX_INCLUDE_GRAPHICS_H_
#define LINUX_INCLUDE_GRAPHICS_H_
#include <assert.h>
#include <epoxy/gl.h>
#include <gtk/gtk.h>

static constexpr GLenum kDefaultTextureTargetBinding = GL_TEXTURE_BINDING_2D;
static constexpr GLenum kDefaultTextureTarget = GL_TEXTURE_2D;

// Saves the bound texture, render, and frame buffers for as long as this object
// is in scope, then restores them when released.
//
// The GDK GL context must not change (or if it does, it should be restored)
// across the lifetime of this object.
//
// Example:
//
// class Foo {
//  Foo::Bar* Baz() {
//    // Note that gdk_ctx must have been made current prior to this call.
//    SavedBufferContextRestorer buffer_context(gdk_ctx);
//    ...
//    /* Some texture/render buffer bindings happening here */
//    return bar;
//  }
// };
struct SavedBufferContextRestorer {
  GLint texture_binding;
  GLint render_buffer_binding;
  GLint frame_buffer_binding;
  SavedBufferContextRestorer()
      : texture_binding(0),
        render_buffer_binding(0),
        frame_buffer_binding(0),
        context_(gdk_gl_context_get_current()) {
    glGetIntegerv(kDefaultTextureTargetBinding, &texture_binding);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &render_buffer_binding);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &frame_buffer_binding);
  }
  ~SavedBufferContextRestorer() {
    assert(context_ == gdk_gl_context_get_current());
    if (texture_binding != 0) {
      glBindTexture(kDefaultTextureTarget, texture_binding);
    }
    if (render_buffer_binding != 0) {
      glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_binding);
    }
    if (frame_buffer_binding != 0) {
      glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_binding);
    }
  }

 private:
  GdkGLContext *context_;
};

// Allocates a texture to fit into the alloted window.
//
// Assumes contexts and bound textures have already been handled.
inline void AllocateTexture(GtkAllocation *allocation) {
  GLenum target = kDefaultTextureTarget;
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(target, 0, GL_RGBA8, allocation->width, allocation->height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

inline void DeleteTexture(GLuint texture) {
  if (texture != 0) {
    glDeleteTextures(1, &texture);
  }
}

inline void DeleteRenderbuffer(GLuint rb) {
  if (rb != 0) {
    glDeleteRenderbuffers(1, &rb);
  }
}

inline void DeleteFramebuffer(GLuint fbo) {
  if (fbo != 0) {
    glDeleteFramebuffers(1, &fbo);
  }
}
#endif  // LINUX_INCLUDE_GRAPHICS_H_
