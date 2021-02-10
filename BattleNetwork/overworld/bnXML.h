#include <string>
#include <vector>
#include <unordered_map>

struct XMLElement {
  std::string name;
  std::unordered_map<std::string, std::string> attributes;
  std::vector<XMLElement> children;
  std::string text;

  bool HasAttribute(const std::string& name) const;
  std::string GetAttribute(const std::string& name) const;
  int GetAttributeInt(const std::string& name) const;
  int GetAttributeFloat(const std::string& name) const;
};

// expects valid XML, does no error checking
XMLElement parseXML(const std::string& data);
