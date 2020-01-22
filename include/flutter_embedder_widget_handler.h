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
#ifndef LINUX_INCLUDE_FLUTTER_EMBEDDER_WIDGET_HANDLER_H_
#define LINUX_INCLUDE_FLUTTER_EMBEDDER_WIDGET_HANDLER_H_
#include "embedder.h"

#include <epoxy/gl.h>
#include <gtk/gtk.h>

#include <condition_variable>
#include <mutex>
#include <string>

#include "flutter_engine_params_inline.h"

// Handles the drawing backend and Flutter API calls for the parent GTK widget.
class FlutterEmbedderWidgetHandler {
 public:
  FlutterEmbedderWidgetHandler(std::string main_path, std::string assets_path,
                               std::string packages_path,
                               std::string icu_data_path, int argc,
                               const char **argv, GtkGLArea *gl_area);
  ~FlutterEmbedderWidgetHandler();

  // Launches the Flutter Engine.
  //
  // Returns true if launched successfully. False if: |gtk_context| or
  // |allocation| are null, or if the engine failed to launch.
  bool InitFlutterEngine(GdkGLContext *gtk_context, GtkAllocation *allocation);

  // Renders the GTK widget area.
  //
  // Note this is run from the GTK thread (and may not be the same thread across
  // calls, so the surrounding context is not guaranteed to be the same).
  //
  // This must be called from the GTK widget graphics context.
  bool RenderGtkWidget(GtkAllocation *allocation);

  // Resizes the Flutter Drawing area, and tells the engine to redraw.
  void HandleResizeEvent(GtkAllocation *allocation);

  // Sends pointer events to the Flutter Engine.
  //
  // The pointer event is marked with the timestamp of when this function was
  // called, so it must be called synchronously with the pointer event.
  void SendFlutterPointerEventWithPhase(FlutterPointerPhase phase,
                                        GdkEvent *event);

 protected:
  // Allocates space in VRAM for the rendering buffers (different from
  // ResizeFlutterBuffers, as that function does not generate any of the
  // textures, or bind anything to any framebuffers).
  void AllocateFlutterBuffers(GtkAllocation *allocation);

  // Reallocates the Flutter rendering buffers to fit to the size of the new GTK
  // window allocation.
  void ResizeFlutterBuffers(GtkAllocation *allocation);

  // Deletes all buffers (textures/fbo/renderbuffers).
  void ReleaseRenderBuffers();

  // Sends a resize event to the Flutter Engine.
  void SendFlutterEngineResizeEvent(GtkAllocation *allocation);

  /////---- Flutter Engine Callbacks ----//////
  static bool FlutterMakeCurrent(void *user_data);
  static bool FlutterClearCurrent(void *user_data);
  static bool FlutterPresent(void *user_data);
  static uint32_t FlutterGetFbo(void *user_data);
  static void OnFlutterPlatformMessage(
      const FlutterPlatformMessage *platform_message, void *user_data);

 private:
  FlutterEngineParams engine_params_;
  FlutterEngine flutter_engine_;
  GLuint flutter_engine_fbo_;

  GLuint front_buffer_tx_;

  // Default texture/renderbuffer to use with the Flutter Engine framebuffer.
  GLuint flutter_tx_;
  GLuint flutter_rb_;

  // Respective OpenGL contexts (not owned).
  GdkGLContext *flutter_gl_context_;

  // Instance of the GL area for pushing frames (not owned).
  GtkGLArea *gl_area_;

  std::mutex frame_ready_m_;
  std::condition_variable frame_ready_cv_;
  GLsync frame_ready_;
};
#endif  // LINUX_INCLUDE_FLUTTER_EMBEDDER_WIDGET_HANDLER_H_
