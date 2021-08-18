#pragma once
#include <string>
#include <fstream>
#include "result.h"
#include "../zip/zip.h"

// TODO: mac os < 10.5 file system support...
#ifndef __APPLE__
#include <filesystem>
#endif 

/* STD LIBRARY extensions */
namespace stx {

  namespace detail {
    static void zip_walk(struct zip_t* zip, const char* target_path, const char* parent_dir = "") {
      for(const auto& entry : std::filesystem::directory_iterator(target_path)) {
        std::string entry_path_string = entry.path().generic_string();
        const char* entry_path = entry_path_string.c_str();
        std::string filename = entry.path().filename().string();

        std::string parent = std::string(parent_dir);

        if (parent.size()) {
          filename = parent + "/" + filename;
        }

        if (entry.is_directory()) {
          zip_walk(zip, entry_path, filename.c_str());
        }
        else {
          zip_entry_open(zip, filename.c_str());
          zip_entry_fwrite(zip, entry_path);
          zip_entry_close(zip);
        }
      }
    }

    static void unzip_walk(struct zip_t* zip, const char* target_path) {
      int i, n = static_cast<int>(zip_entries_total(zip));
      for (i = 0; i < n; ++i) {
        zip_entry_openbyindex(zip, i);
        {
          const char* name = zip_entry_name(zip);
          int isdir = zip_entry_isdir(zip);

          if (isdir) {
            std::string next_path = std::string(target_path) + name + "\\";
            std::filesystem::create_directory(std::filesystem::path(next_path));
            unzip_walk(zip, target_path);
          }
          else {
            std::string filename = std::string(target_path) + "\\" + name;
            zip_entry_fread(zip, filename.c_str());
          }
        }
        zip_entry_close(zip);
      }
    }
  }

  static result_t<bool> zip(const std::string& target_path, const std::string& destination_path) {
#ifndef __APPLE__
    struct zip_t* zip = zip_open(destination_path.c_str(), 9, 'w');
    detail::zip_walk(zip, target_path.c_str());
    zip_close(zip);

    return ok();

#elif
    return error<bool>("System filedirectory utils not supported on APPLE");
#endif
  }

  static result_t<bool> unzip(const std::string& target_path, const std::string& destination_path) {
    if(zip_extract(target_path.c_str(), destination_path.c_str(), nullptr, nullptr) == 0)
      return ok();

    return error<bool>(std::string("Unable to unzip ") + target_path);
  }
}