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

#include <cstdint>
#include <cstdlib>

static constexpr uint32_t kDefaultWindowWidth = 800;
static constexpr uint32_t kDefaultWindowHeight = 600;

static void app_activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window = gtk_application_window_new(app);
  int argc = 2;
  const char *argv[] = {"", "--dart-non-checked-mode", NULL};
#define HOME_PATH "/usr/local/google/home/awdavies/"
#define FLUTTER_PATH HOME_PATH "proj/flutter/examples/flutter_gallery/"
  GtkWidget *flutter_embedder = flutter_embedder_new(
      FLUTTER_PATH "lib/main.dart", FLUTTER_PATH "build/flutter_assets",
      FLUTTER_PATH ".packages",
      HOME_PATH "proj/engine/src/out/host_debug_unopt/icudtl.dat", argc, argv);
  gtk_window_set_title(GTK_WINDOW(window), "Flutter");
  gtk_window_set_default_size(GTK_WINDOW(window), kDefaultWindowWidth,
                              kDefaultWindowHeight);
  gtk_container_add(GTK_CONTAINER(window), flutter_embedder);
  gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
  flutter_embedder_init();
  GtkApplication *app =
      gtk_application_new("flutter.linux", G_APPLICATION_FLAGS_NONE);
  g_signal_connect(app, "activate", G_CALLBACK(app_activate), NULL);
  g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return EXIT_SUCCESS;
}
