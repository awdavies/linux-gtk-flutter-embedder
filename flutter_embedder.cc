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
#include "include/flutter_embedder.h"

#include <X11/Xlib.h>
#include <atomic>
#include <iostream>

#include "include/flutter_embedder_widget_handler.h"

static constexpr char kFlutterDataPrivate[] = "flutter_embedder_internal_";

static std::atomic<bool> pointer_down(false);

// Returns an instance of the stored FlutterEmbedderWidgetHandler.
static FlutterEmbedderWidgetHandler *get_widget_handler(GtkWidget *widget) {
  return reinterpret_cast<FlutterEmbedderWidgetHandler *>(
      g_object_get_data(G_OBJECT(widget), kFlutterDataPrivate));
}

// Initializes widget state.
//
// Called after a widget has been mapped to a GdkWindow.
static gboolean gl_area_realize(GtkWidget *area) {
  GtkAllocation allocation;
  gtk_widget_get_allocation(area, &allocation);
  GtkGLArea *gl_area = GTK_GL_AREA(area);
  auto gtk_gl_context = gtk_gl_area_get_context(gl_area);
  return get_widget_handler(area)->InitFlutterEngine(gtk_gl_context,
                                                     &allocation);
}

// Resizes the widget.
static void gl_area_resize(GtkWidget *area) {
  GtkAllocation allocation;
  gtk_widget_get_allocation(area, &allocation);
  get_widget_handler(area)->HandleResizeEvent(&allocation);
}

// Handles rendering the widget.
static gboolean gl_area_render(GtkWidget *area) {
  GtkAllocation allocation;
  gtk_widget_get_allocation(area, &allocation);
  return get_widget_handler(area)->RenderGtkWidget(&allocation);
}

static void gl_area_destroy(GtkWidget *area) {
  delete get_widget_handler(area);
}

// Sends mouse button press events to the Flutter Engine.
static void button_press_handler(GtkWidget *widget, GdkEvent *event) {
  // Occasionally too many consecutive pointer events are sent, causing the
  // engine to raise exceptoins.
  if (pointer_down) {
    return;
  }
  // The container class GtkEventBox is a subclass of GtkBin, a container with a
  // single child.
  GtkWidget *gl_area = gtk_bin_get_child(GTK_BIN(widget));
  get_widget_handler(gl_area)->SendFlutterPointerEventWithPhase(
      FlutterPointerPhase::kDown, event);
  pointer_down = true;
}

// Sends mouse button release events to the Flutter Engine.
static void button_release_handler(GtkWidget *widget, GdkEvent *event) {
  // See `button_press_handler` for more on this.
  if (!pointer_down) {
    return;
  }
  GtkWidget *gl_area = gtk_bin_get_child(GTK_BIN(widget));
  get_widget_handler(gl_area)->SendFlutterPointerEventWithPhase(
      FlutterPointerPhase::kUp, event);
  pointer_down = false;
}

// Sends motion data to the Flutter Engine.
//
// Only sends motion data if the mouse has been pressed and not released.
static void pointer_motion_handler(GtkWidget *widget, GdkEvent *event) {
  GtkWidget *gl_area = gtk_bin_get_child(GTK_BIN(widget));
  if (!pointer_down) {
    return;
  }
  get_widget_handler(gl_area)->SendFlutterPointerEventWithPhase(
      FlutterPointerPhase::kMove, event);
}

GtkWidget *flutter_embedder_new(const char *main_path, const char *assets_path,
                                const char *packages_path,
                                const char *icu_data_path, int argc,
                                const char **argv) {
  // The container both adds a single layer of opacity to the implementation of
  // the widget and also allows for events to be properly sent to the child
  // widget.
  GtkWidget *container = gtk_event_box_new();
  GtkWidget *gl_area = gtk_gl_area_new();
  gtk_gl_area_set_use_es(GTK_GL_AREA(gl_area), TRUE);
  gtk_gl_area_set_has_alpha(GTK_GL_AREA(gl_area), TRUE);
  g_object_set_data(G_OBJECT(gl_area), kFlutterDataPrivate,
                    reinterpret_cast<void *>(new FlutterEmbedderWidgetHandler(
                        main_path, assets_path, packages_path, icu_data_path,
                        argc, argv, GTK_GL_AREA(gl_area))));
  g_signal_connect(gl_area, "render", G_CALLBACK(gl_area_render), NULL);
  g_signal_connect(gl_area, "realize", G_CALLBACK(gl_area_realize), NULL);
  g_signal_connect(gl_area, "resize", G_CALLBACK(gl_area_resize), NULL);
  g_signal_connect(gl_area, "destroy", G_CALLBACK(gl_area_destroy), NULL);

  gtk_widget_add_events(container,
                        GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK);
  g_signal_connect(container, "button-press-event",
                   G_CALLBACK(button_press_handler), NULL);
  g_signal_connect(container, "button-release-event",
                   G_CALLBACK(button_release_handler), NULL);
  g_signal_connect(container, "motion-notify-event",
                   G_CALLBACK(pointer_motion_handler), NULL);
  gtk_container_add(GTK_CONTAINER(container), gl_area);
  return container;
}

GtkWidget *flutter_embedder_snapshot_mode_new(const char *assets_path,
                                              const char *icu_data_path,
                                              int argc, const char **argv) {
  return flutter_embedder_new("", assets_path, "", icu_data_path, argc, argv);
}

void flutter_embedder_init() { XInitThreads(); }
