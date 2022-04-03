#pragma once
#include <charconv>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include "string.h"
#include "result.h"

namespace stx {
  namespace filesystem {
    namespace details {
      // hidden singleton that tracks and deletes files at the end of application lifetime
      class TrashCollector {
      private:
          std::vector<std::filesystem::path> tempFiles;
                
          ~TrashCollector();

      public:
          static TrashCollector& Instance();
          void Add(const std::filesystem::path& file);
      };
    }

    // Deferred trash at end of application lifetime to remove off disc
    void trash_queue(const std::filesystem::path& file);

    // Immediately deletes file from disc
    void trash(const std::filesystem::path& file);

    // Generates a random filename at cache path
    std::filesystem::path generate_temp_filename(const std::filesystem::path& path);

    // Tries to replace a file. If there are any errors, revert safely
    result_t<bool> replace(const std::filesystem::path& srcFile, const std::filesystem::path& dstFile, const std::filesystem::path& tempDir = ".");
  }
}