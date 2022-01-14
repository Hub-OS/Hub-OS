#pragma once

#include <filesystem>

#include "result.h"

/* STD LIBRARY extensions */
namespace stx {
  result_t<bool> zip(const std::filesystem::path& target_path, const std::filesystem::path& destination_path);
  result_t<bool> unzip(const std::filesystem::path& target_path, const std::filesystem::path& destination_path);
}
