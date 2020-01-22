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
#include "include/flutter_embedder_widget_handler.h"

#include <gtk/gtk.h>
#include <time.h>

#include <chrono>
#include <iostream>

#include "include/graphics.h"

FlutterEmbedderWidgetHandler::FlutterEmbedderWidgetHandler(
    std::string main_path, std::string assets_path, std::string packages_path,
    std::string icu_data_path, int argc, const char **argv, GtkGLArea *gl_area)
    : engine_params_(main_path, assets_path, packages_path, icu_data_path, argc,
                     argv),
      gl_area_(gl_area),
      frame_ready_(0) {}

FlutterEmbedderWidgetHandler::~FlutterEmbedderWidgetHandler() {
  ReleaseRenderBuffers();
  FlutterEngineShutdown(flutter_engine_);
}

void FlutterEmbedderWidgetHandler::ReleaseRenderBuffers() {
  DeleteTexture(front_buffer_tx_);

  DeleteFramebuffer(flutter_engine_fbo_);
  DeleteTexture(flutter_tx_);
  DeleteRenderbuffer(flutter_rb_);
}

void FlutterEmbedderWidgetHandler::SendFlutterPointerEventWithPhase(
    FlutterPointerPhase phase, GdkEvent *event) {
  FlutterPointerEvent pointer_event = {};
  pointer_event.struct_size = sizeof(pointer_event);
  pointer_event.phase = phase;
  switch (phase) {
    case FlutterPointerPhase::kDown:
    case FlutterPointerPhase::kUp: {
      auto button_event = reinterpret_cast<GdkEventButton *>(event);
      pointer_event.x = button_event->x;
      pointer_event.y = button_event->y;
      break;
    }
    case FlutterPointerPhase::kMove: {
      auto move_event = reinterpret_cast<GdkEventMotion *>(event);
      pointer_event.x = move_event->x;
      pointer_event.y = move_event->y;
      break;
    }
    default:  // kCancel
      break;
  }
  pointer_event.timestamp =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  FlutterEngineSendPointerEvent(flutter_engine_, &pointer_event, 1);
}

void FlutterEmbedderWidgetHandler::HandleResizeEvent(
    GtkAllocation *allocation) {
  {
    std::unique_lock<std::mutex> lock(frame_ready_m_);
    // Ensure all previously queue-up work has completed.
    glFinish();

    // Note: this function runs in the GTK thread.
    ResizeFlutterBuffers(allocation);
    glDeleteSync(frame_ready_);
    // It is imperative that this is called here, as the notification will cause
    // a new frame to be rendered in what could be an incorrectly sized texture
    // if this isn't guaranteed to complete synchronously.
    glFinish();
    frame_ready_ = nullptr;
    frame_ready_cv_.notify_all();
  }
  SendFlutterEngineResizeEvent(allocation);
}

void FlutterEmbedderWidgetHandler::ResizeFlutterBuffers(
    GtkAllocation *allocation) {
  SavedBufferContextRestorer prev_ctx;

  glBindTexture(kDefaultTextureTarget, flutter_tx_);
  AllocateTexture(allocation);
  // TODO(awdavies):
  // From https://www.khronos.org/opengl/wiki/Renderbuffer_Object :
  // It is strongly advised to delete and reallocate render buffers to ensure
  // fbo completeness.
  glBindRenderbuffer(GL_RENDERBUFFER, flutter_rb_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                        allocation->width, allocation->height);

  glBindTexture(kDefaultTextureTarget, front_buffer_tx_);
  AllocateTexture(allocation);
}

void FlutterEmbedderWidgetHandler::SendFlutterEngineResizeEvent(
    GtkAllocation *allocation) {
  FlutterWindowMetricsEvent window_metrics_event = {};
  window_metrics_event.struct_size = sizeof(window_metrics_event);
  window_metrics_event.width = allocation->width;
  window_metrics_event.height = allocation->height;
  window_metrics_event.pixel_ratio = 1.0;
  FlutterEngineSendWindowMetricsEvent(flutter_engine_, &window_metrics_event);
}

bool FlutterEmbedderWidgetHandler::RenderGtkWidget(GtkAllocation *allocation) {
  // Note: this function runs in the GTK thread.
  GLint fbo = 0;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &fbo);
  if (fbo == 0) {
    // The return value here is ignored.
    return true;
  }
  std::unique_lock<std::mutex> lock(frame_ready_m_);
  frame_ready_cv_.wait(lock, [this] { return frame_ready_ != nullptr; });
  // Ensure all previously queued-up work has completed.
  glFinish();
  SavedBufferContextRestorer prev_ctx;

  // Draws the front buffer to the GTK widget area. While performance isn't a
  // factor right now, it may be faster to use a shader instead.
  glBindTexture(kDefaultTextureTarget, front_buffer_tx_);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(-1.0f, 1.0f, 0.0);
  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(-1.0f, -1.0f, 0.0);
  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(1.0f, -1.0f, 0.0);
  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(1.0f, 1.0f, 0.0);
  glEnd();

  glFinish();
  return true;
}

bool FlutterEmbedderWidgetHandler::InitFlutterEngine(
    GdkGLContext *gtk_context, GtkAllocation *allocation) {
  if (gtk_context == nullptr || allocation == nullptr) {
    return false;
  }
  auto gdk_window = gdk_gl_context_get_window(gtk_context);
  GError *error = nullptr;
  flutter_gl_context_ = gdk_window_create_gl_context(gdk_window, &error);
  if (flutter_gl_context_ == nullptr) {
    return false;
  }
  gdk_gl_context_set_use_es(flutter_gl_context_, TRUE);
  AllocateFlutterBuffers(allocation);

  auto project_args = engine_params_.GetProjectArgs();
  const FlutterOpenGLRendererConfig renderer_config = {
    struct_size : sizeof(FlutterOpenGLRendererConfig),
    make_current : FlutterEmbedderWidgetHandler::FlutterMakeCurrent,
    clear_current : FlutterEmbedderWidgetHandler::FlutterClearCurrent,
    present : FlutterEmbedderWidgetHandler::FlutterPresent,
    fbo_callback : FlutterEmbedderWidgetHandler::FlutterGetFbo
  };
  FlutterRendererConfig config = {};
  config.type = kOpenGL;
  config.open_gl = renderer_config;
  bool status =
      FlutterEngineRun(FLUTTER_ENGINE_VERSION, &config, project_args.get(),
                       reinterpret_cast<void *>(this), &this->flutter_engine_);
  if (status != kSuccess) {
    std::cerr << "Unable to start flutter engine." << std::endl;
    return status;
  }
  SendFlutterEngineResizeEvent(allocation);
  return status == kSuccess;
}

void FlutterEmbedderWidgetHandler::AllocateFlutterBuffers(
    GtkAllocation *allocation) {
  // This should be the only place where this is explicitly made current since
  // this is called in the main thread. All other function calls here should be
  // made from the Flutter graphics thread.
  gdk_gl_context_make_current(flutter_gl_context_);
  SavedBufferContextRestorer prev_ctx;

  glGenTextures(1, &flutter_tx_);
  glGenTextures(1, &front_buffer_tx_);

  glBindTexture(kDefaultTextureTarget, flutter_tx_);
  AllocateTexture(allocation);
  glGenRenderbuffers(1, &flutter_rb_);
  glBindRenderbuffer(GL_RENDERBUFFER, flutter_rb_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                        allocation->width, allocation->height);

  glGenFramebuffers(1, &flutter_engine_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, flutter_engine_fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         kDefaultTextureTarget, flutter_tx_, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, flutter_rb_);

  // Allocate intermediate buffering textures.
  glBindTexture(kDefaultTextureTarget, front_buffer_tx_);
  AllocateTexture(allocation);
}

bool FlutterEmbedderWidgetHandler::FlutterMakeCurrent(void *user_data) {
  auto handler = reinterpret_cast<FlutterEmbedderWidgetHandler *>(user_data);
  gdk_gl_context_make_current(handler->flutter_gl_context_);
  return true;
}

bool FlutterEmbedderWidgetHandler::FlutterClearCurrent(void *user_data) {
  // Do nothing here.
  //
  // The flutter rendering thread and the GTK rendering thread
  // have completely separate contexts, and calling the GDK context-clearing
  // function will clear ALL contexts regardless of thread, which causes both
  // sides of the application to get confused and/or crash.
  return true;
}

bool FlutterEmbedderWidgetHandler::FlutterPresent(void *user_data) {
  auto handler = reinterpret_cast<FlutterEmbedderWidgetHandler *>(user_data);
  std::unique_lock<std::mutex> lock(handler->frame_ready_m_);
  // Make sure all previous events (texture reads, etc) complete.
  glFinish();

  GtkAllocation allocation;
  gtk_widget_get_allocation(GTK_WIDGET(handler->gl_area_), &allocation);
  SavedBufferContextRestorer prev_ctx;
  glBindFramebuffer(GL_FRAMEBUFFER, handler->flutter_engine_fbo_);

  // TODO: If we are animating and resizing at the same time, it is possible a
  // frame could be pushed that is the wrong size.

  glBindTexture(kDefaultTextureTarget, handler->front_buffer_tx_);
  // TODO: this causes errors when resizing while an animation is running.
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, allocation.width,
                      allocation.height);
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    std::cerr << "Flutter Present Callback Error: " << err << std::endl;
  }

  if (handler->frame_ready_ != nullptr) {
    glDeleteSync(handler->frame_ready_);
  }
  GLsync frame_ready_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  // Ensure the sync is attached and copying to the texture
  // actually occurs properly.
  glFinish();
  handler->frame_ready_ = frame_ready_sync;
  handler->frame_ready_cv_.notify_all();
  gtk_gl_area_queue_render(handler->gl_area_);
  return true;
}

uint32_t FlutterEmbedderWidgetHandler::FlutterGetFbo(void *user_data) {
  auto handler = reinterpret_cast<FlutterEmbedderWidgetHandler *>(user_data);
  return handler->flutter_engine_fbo_;
}

void FlutterEmbedderWidgetHandler::OnFlutterPlatformMessage(
    const FlutterPlatformMessage *platform_message, void *user_data) {
  // TODO: Implement this along with the plugin framework.
}
