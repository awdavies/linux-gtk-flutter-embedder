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
#ifndef LINUX_INCLUDE_FLUTTER_EMBEDDER_H_
#define LINUX_INCLUDE_FLUTTER_EMBEDDER_H_
#include <gtk/gtk.h>

G_BEGIN_DECLS

// To be called before anything else happens in your main function.
void flutter_embedder_init();

GtkWidget *flutter_embedder_new(const char *main_path, const char *assets_path,
                                const char *packages_path,
                                const char *icu_data_path, int argc,
                                const char **argv);

GtkWidget *flutter_embedder_snapshot_mode_new(const char *assets_path,
                                              const char *icu_data_path,
                                              int argc, const char **argv);

G_END_DECLS

#endif  // LINUX_INCLUDE_FLUTTER_EMBEDDER_H_
