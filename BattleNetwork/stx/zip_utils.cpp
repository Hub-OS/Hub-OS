#include "zip_utils.h"

#include <iostream>
#include <string>

#include "string.h"
#include "../zip/zip.h"

namespace {
  stx::result_t<bool> zip_walk(struct zip_t* zip, const std::filesystem::path& target_path, const std::filesystem::path& zip_root_path = {}) {
    using namespace std::string_literals;

    for(const auto& entry : std::filesystem::directory_iterator(target_path)) {
      if (entry.is_directory()) {
        if (stx::result_t<bool> r = zip_walk(zip, entry.path(), zip_root_path / entry.path().filename()); r.is_error()) {
          return r;
        }
        continue;
      }

      std::filesystem::path zip_entry_path = zip_root_path / entry.path().filename();
      std::string zip_entry_path_string = zip_entry_path.generic_u8string();
      if (int r = zip_entry_open(zip, zip_entry_path_string.c_str()); r != 0) {
        return stx::error<bool>("zip_entry_open for "s + zip_entry_path_string + ": " + zip_strerror(r));
      }

      std::string entry_path_string = entry.path().u8string();
      if (int r = zip_entry_fwrite(zip, entry_path_string.c_str()); r != 0) {
        return stx::error<bool>("zip_entry_fwrite for "s + entry_path_string + ", compressing to " + zip_entry_path_string + ": " + zip_strerror(r));
      }

      if (int r = zip_entry_close(zip); r != 0) {
        return stx::error<bool>("zip_entry_close for "s + zip_entry_path_string + ": " + zip_strerror(r));
      }
    }
    return stx::ok();
  }

  stx::result_t<uint64_t> unzip_walk(struct zip_t* zip, const std::filesystem::path& destination_path, const std::filesystem::path& root_path, int size_limit = 100 * 1024 * 1024 /* 100MB */) {
    using namespace std::string_literals;

    uint64_t current_size = 0;

    int i, n = static_cast<int>(zip_entries_total(zip));
    if (n < 0) {
      return stx::error<uint64_t>("zip_entries_total: "s + zip_strerror(n));
    }
    for (i = 0; i < n; ++i) {
      if (int r = zip_entry_openbyindex(zip, i); r != 0) {
        return stx::error<uint64_t>("zip_entry_openbyindex for "s + std::to_string(i) + ": " + zip_strerror(r));
      }

      std::filesystem::path filename = std::filesystem::u8path(zip_entry_name(zip));
      std::filesystem::path entry_destination_path = (destination_path / filename).lexically_normal();
      std::string entry_destination_path_string = entry_destination_path.u8string();
      if (auto [_, end] = std::mismatch(entry_destination_path.begin(), entry_destination_path.end(), root_path.begin(), root_path.end()); end != root_path.end()) {
        return stx::error<uint64_t>("extracting "s + filename.u8string() + " will extract outside of root path to " + entry_destination_path_string);
      }

      current_size += zip_entry_size(zip);
      if (size_limit > 0 && current_size > size_limit) {
        return stx::error<uint64_t>("extracting "s + filename.u8string() + " exceeds size limit: current = " + std::to_string(current_size) + "bytes, limit = " + std::to_string(size_limit) + " bytes");
      }

      if (!zip_entry_isdir(zip)) {
        try {
          std::filesystem::create_directories(entry_destination_path.parent_path());  // Result ignored, folder may already exist.
        } catch(std::exception& e) {
          return stx::error<uint64_t>(e.what());
        }

        if (int r = zip_entry_fread(zip, entry_destination_path_string.c_str()); r != 0) {
          return stx::error<uint64_t>("zip_entry_fread for "s + filename.u8string() + ", extracting to " + entry_destination_path_string + ": " + zip_strerror(r));
        }
      }

      if (int r = zip_entry_close(zip); r != 0) {
        return stx::error<uint64_t>("zip_entry_close for "s + filename.u8string() + ": " + zip_strerror(r));
      }
    }
    return stx::ok(current_size);
  }
}

/* STD LIBRARY extensions */
namespace stx {
  result_t<bool> zip(const std::filesystem::path& target_path, const std::filesystem::path& destination_path) {
    std::string destination_path_string = destination_path.u8string();
    std::unique_ptr<zip_t, decltype(&zip_close)> zip{zip_open(destination_path_string.c_str(), 9, 'w'), zip_close};
    if (!zip) {
      return error<bool>("failed to open zip file");
    }
    return zip_walk(zip.get(), target_path.lexically_normal());
  }

  result_t<bool> unzip(const std::filesystem::path& target_path, const std::filesystem::path& destination_path) {
    std::string target_path_string = target_path.u8string();
    std::unique_ptr<zip_t, decltype(&zip_close)> zip{zip_open(target_path_string.c_str(), -1, 'r'), zip_close};
    if (!zip) {
      return error<bool>("failed to open zip file");
    }
    std::filesystem::path normalized_destination_path = destination_path.lexically_normal();
    if (result_t<uint64_t> r = unzip_walk(zip.get(), normalized_destination_path, normalized_destination_path); r.is_error()) {
      return error<bool>(std::string(r.error_cstr()));
    }
    return ok();
  }
}
