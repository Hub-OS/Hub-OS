#pragma once

#include <unordered_map>

#include "bnXML.h"

namespace Overworld {
  class CustomProperties {
  public:
    static CustomProperties From(const XMLElement& propertiesElement);

    bool HasProperty(const std::string& name) const;
    std::string GetProperty(const std::string& name) const;
    int GetPropertyInt(const std::string& name) const;
    float GetPropertyFloat(const std::string& name) const;

  private:
    std::unordered_map<std::string, std::string> properties;
  };
}