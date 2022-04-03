#include "filesystem.h"

namespace stx {
  namespace filesystem {
    namespace details {
      TrashCollector::~TrashCollector() {
        for (std::filesystem::path& file : tempFiles) {
          std::filesystem::remove_all(file);
        }
      }

      TrashCollector& TrashCollector::Instance() {
        static TrashCollector singleton;
        return singleton;
      }

      void TrashCollector::Add(const std::filesystem::path& file) {
        tempFiles.push_back(file);
      }
    }

    void trash_queue(const std::filesystem::path& file) {
      details::TrashCollector::Instance().Add(file);
    }

    void trash(const std::filesystem::path& file) {
      std::filesystem::remove_all(file);
    }

    // Generates a random filename at cache path
    std::filesystem::path generate_temp_filename(const std::filesystem::path& path) {
      return path / std::filesystem::path(stx::rand_alphanum(12));
    }

    // Tries to replace a file. If there are any errors, revert safely
    result_t<bool> replace(const std::filesystem::path& srcFile,
      const std::filesystem::path& dstFile,
      const std::filesystem::path& tempDir) {
      if (!std::filesystem::is_directory(tempDir)) {
        return stx::error<bool>("path \"" + tempDir.generic_string() + "\" is not a directory!");
      }

      std::filesystem::path tempPath = generate_temp_filename(tempDir);

      // First, copy the contents of the file at destination to a temp safe location
      std::error_code err;
      if (!std::filesystem::copy_file(dstFile, tempPath, err)) {
        return stx::error<bool>(err.message());
      }

      trash(dstFile);

      // Second, move the source file to the destination location
      new (&err) std::error_code{};
      if (!std::filesystem::copy_file(srcFile, dstFile, err)) {
        // Restore the original file.
        // If the original file copy fails, it should at least
        // be in the specified `tempDir`
        std::filesystem::copy_file(tempPath, dstFile);
        return stx::error<bool>(err.message());
      }

      // Third, if successfull thus far, delete the old files moved to temp location
      trash(tempPath);
      trash(srcFile);

      // Successfully replaced dstFile with srcFile!
      return true;
    }
  }
}