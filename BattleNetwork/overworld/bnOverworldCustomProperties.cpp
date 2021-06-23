#include "bnOverworldCustomProperties.h"

#include <algorithm>

namespace Overworld {
  bool CustomProperties::HasProperty(const std::string& name) const {
    return properties.find(name) != properties.end();
  }

  std::string CustomProperties::GetProperty(const std::string& name) const {
    if (!HasProperty(name)) {
      return "";
    }

    return properties.at(name);
  }

  int CustomProperties::GetPropertyInt(const std::string& name) const {
    if (!HasProperty(name)) {
      return 0;
    }

    try {
      return stoi(properties.at(name));
    }
    catch (std::exception&) {
      // conversion failure, use default value (0)
    }

    return 0;
  }


  float CustomProperties::GetPropertyFloat(const std::string& name) const {
    if (!HasProperty(name)) {
      return 0.0;
    }

    try {
      return stof(properties.at(name));
    }
    catch (std::exception&) {
      // conversion failure, use default value (0)
    }

    return 0.0;
  }

  CustomProperties CustomProperties::From(const XMLElement& propertiesElement) {
    CustomProperties properties;
    properties.properties = propertiesElement.attributes;

    for (auto child : propertiesElement.children) {
      if(child.name != "property") {
        continue;
      }

      auto name = child.GetAttribute("name");

      std::transform(name.begin(), name.end(), name.begin(), ::tolower);

      properties.properties[name] = child.GetAttribute("value");
    }

    return properties;
  }
}
