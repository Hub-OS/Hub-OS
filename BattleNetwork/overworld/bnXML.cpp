#include "bnXML.h"

#include <algorithm>
#include <cctype>
#include <string>
#include "../bnLogger.h"
#include "../stx/string.h"

bool XMLElement::HasAttribute(const std::string& name) const {
  return attributes.find(name) != attributes.end();
}

std::string XMLElement::GetAttribute(const std::string& name) const {
  return HasAttribute(name) ? attributes.at(name) : "";
}

int XMLElement::GetAttributeInt(const std::string& name) const {
  if (name.empty()) return 0;

  auto stringValue = GetAttribute(name);

  if (stringValue.empty()) {
    return 0;
  }

  auto result = stx::to_int(stringValue);

  if (result.is_error()) {
    Logger::Logf(LogLevel::warning, "Cannot convert xml attribute `%s` to int. Reason: %s", name.c_str(), result.error_cstr());
    return 0;
  }

  return result.value();
}

float XMLElement::GetAttributeFloat(const std::string& name) const {
  if (name.empty()) return 0.0;

  auto stringValue = GetAttribute(name);

  if (stringValue.empty()) {
    return 0.0;
  }

  auto result = stx::to_float(stringValue);

  if (result.is_error()) {
    Logger::Logf(LogLevel::warning, "Cannot convert xml attribute `%s` to float. Reason: %s", name.c_str(), result.error_cstr());
    return 0.0;
  }

  return result.value();
}

enum XMLTokenType {
  element,
  close,
  attributeName,
  attributeValue,
  text,
  none
};

struct XMLToken {
  XMLTokenType type;
  std::string text;
  size_t start;
  size_t end;
  size_t exit;
};

static size_t skipToAfter(const std::string& data, size_t index, const std::string& __s) {
  auto endIndex = data.find(__s, index);

  if (endIndex == std::string::npos) {
    return data.length();
  }

  return endIndex + __s.length();
}

static void trim(std::string& data) {
  auto start = std::find_if(data.begin(), data.end(), [](char c) { return !std::isspace(c); });
  data.erase(data.begin(), start);

  auto end = std::find_if(data.rbegin(), data.rend(), [](char c) { return !std::isspace(c); });
  data.erase(end.base(), data.end());
}


static std::string getTextTokenText(const std::string& data, const XMLToken& token) {
  auto text = data.substr(token.start, token.end - token.start);
  trim(text);
  return text;
}

static XMLToken nextToken(const std::string& data, XMLTokenType lastType, size_t start) {
  XMLToken token{
    XMLTokenType::none // type
  };

  auto index = start;
  auto len = data.length();
  char lastChar = '\0';
  bool inElement = lastType == XMLTokenType::element || lastType == XMLTokenType::attributeName || lastType == XMLTokenType::attributeValue;

  while (index < len) {
    unsigned char currentChar = data[index];

    switch (currentChar)
    {
    case '<':
      if (token.type == XMLTokenType::text) {
        token.end = index;
        token.text = getTextTokenText(data, token);
        token.exit = index;
        return token;
      }
      break;
    case '?':
      if (lastChar != '<') {
        break;
      }

      // index++ later, so subtract 1 for now
      index = skipToAfter(data, index, ">") - 1;
      currentChar = '\0';
      break;
    case '!':
      if (lastChar != '<') {
        break;
      }

      // index++ later, so subtract 1 for now
      index = skipToAfter(data, index, "-->") - 1;
      currentChar = '\0';
      break;
    case '/': {
      if (token.type == XMLTokenType::element) {
        token.end = index;
        token.text = data.substr(token.start, token.end - token.start);
        token.exit = index;
        return token;
      }

      if (lastChar != '<') {
        break;
      }

      token.type = XMLTokenType::close;
      token.start = index + 1;
      token.end = skipToAfter(data, index, ">") - 1;
      token.text = data.substr(token.start, token.end - token.start);
      trim(token.text);

      token.exit = token.end + 1;
      return token;
    }
    case '>':
      if (lastChar == '/') {
        token.type = XMLTokenType::close;
        token.start = index - 1;
        token.end = index;
        token.exit = index + 1;
        return token;
      }

      if (token.type == XMLTokenType::element) {
        token.end = index;
        token.text = data.substr(token.start, token.end - token.start);
        token.exit = index;
        return token;
      }

      inElement = false;
      token.start = index + 1;
      token.type = XMLTokenType::text;
      break;
    case '"':
      if (!inElement) {
        break;
      }

      if (token.type == XMLTokenType::attributeValue) {
        token.end = index;
        token.text = data.substr(token.start + 1, token.end - token.start - 1);

        token.exit = index + 1;
        return token;
      }

      token.type = XMLTokenType::attributeValue;
      token.start = index;
      break;
    default:
      if (isalpha(currentChar)) {
        if (lastChar == '<') {
          token.start = index;
          token.type = XMLTokenType::element;
        }
        else if (token.type == XMLTokenType::none && inElement) {
          token.start = index;
          token.type = XMLTokenType::attributeName;
        }
      }
      else if (token.type == XMLTokenType::element || token.type == XMLTokenType::attributeName) {
        token.end = index;
        token.text = data.substr(token.start, token.end - token.start);
        token.exit = index + 1;
        return token;
      }

      break;
    }

    lastChar = currentChar;
    index++;
  }

  token.end = index;
  token.exit = index;

  return token;
}

XMLElement parseXML(const std::string& data) {
  size_t index = 0;

  XMLToken token = nextToken(data, XMLTokenType::none, index);
  index = token.exit;

  XMLElement rootElement = {
    token.text // name
  };

  // error, exiting for safety
  if (token.type != XMLTokenType::element) {
    return rootElement;
  }

  std::vector<XMLElement*> elements;
  elements.push_back(&rootElement);

  XMLElement* currentElement = &rootElement;
  std::string attributeName;

  do {
    token = nextToken(data, token.type, index);
    index = token.exit;

    switch (token.type)
    {
    case XMLTokenType::element: {
      XMLElement child = {
        token.text // name
      };

      currentElement->children.push_back(child);

      auto& children = currentElement->children;
      currentElement = &children[children.size() - 1];
      elements.push_back(currentElement);
      break;
    }
    case XMLTokenType::close:
      if (currentElement->children.size() > 0) {
        currentElement->text = "";
      }

      elements.pop_back();

      if (elements.size() == 0) {
        // completed, exiting now for safety
        return rootElement;
      }

      currentElement = elements[elements.size() - 1];
      break;
    case XMLTokenType::attributeName:
      attributeName = token.text;
      break;
    case XMLTokenType::attributeValue:
      currentElement->attributes.emplace(attributeName, token.text);
      break;
    case XMLTokenType::text:
      currentElement->text = token.text;
      break;
    case XMLTokenType::none:
      return rootElement;
    }

  } while (token.type != XMLTokenType::none);


  return rootElement;
}
