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
#ifndef LINUX_INCLUDE_FLUTTER_ENGINE_PARAMS_INLINE_H_
#define LINUX_INCLUDE_FLUTTER_ENGINE_PARAMS_INLINE_H_
#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "embedder.h"

class FlutterEngineParams {
 public:
  FlutterEngineParams(std::string main_path, std::string assets_path,
                      std::string packages_path, std::string icu_data_path,
                      int argc, const char **argv)
      : main_path_(main_path),
        assets_path_(assets_path),
        packages_path_(packages_path),
        icu_data_path_(icu_data_path),
        argc_(argc) {
    if (argv) {
      char **argv_copy =
          reinterpret_cast<char **>(malloc((argc + 1) * sizeof(char *)));
      int i = 0;
      for (; i < argc; ++i) {
        assert(argv[i] != nullptr);
        size_t length = strlen(argv[i]) + 1;
        argv_copy[i] = reinterpret_cast<char *>(malloc(length));
        memmove(argv_copy[i], argv[i], length);
      }
      argv_copy[i] = nullptr;
      argv_ = argv_copy;
    }
  }
  ~FlutterEngineParams() {
    if (argv_) {
      for (int i = 0; i < argc_; ++i) {
        free(argv_[i]);
      }
      free(argv_);
    }
  }

  // Creates a FlutterProjectArgs struct.
  //
  // The lifetime of this pointer must not be longer than this object, as all
  // strings are tied to this instance.
  std::unique_ptr<FlutterProjectArgs> GetProjectArgs() {
    auto args = std::make_unique<FlutterProjectArgs>();
    args->struct_size = sizeof(FlutterProjectArgs);
    args->assets_path = assets_path_.c_str();
    args->main_path = main_path_.c_str();
    args->packages_path = packages_path_.c_str();
    args->icu_data_path = icu_data_path_.c_str();
    args->command_line_argc = argc_;
    args->command_line_argv = argv_;
    return std::move(args);
  }

 private:
  std::string main_path_;
  std::string assets_path_;
  std::string packages_path_;
  std::string icu_data_path_;
  int argc_;
  char **argv_;
};
#endif  // LINUX_INCLUDE_FLUTTER_ENGINE_PARAMS_INLINE_H_
