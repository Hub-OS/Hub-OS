#include <string>
#include <vector>
#include <unordered_map>

struct XMLElement {
  std::string name;
  std::unordered_map<std::string, std::string> attributes;
  std::vector<XMLElement> children;
  std::string text;
};

// expects valid XML, does no error checking
XMLElement parseXML(const std::string& data);
