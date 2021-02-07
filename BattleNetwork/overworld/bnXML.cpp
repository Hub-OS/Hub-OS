#include "bnXML.h"
#include <algorithm>
#include <cctype>

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
  auto end = std::find_if(data.rbegin(), data.rend(), [](char c) { return !std::isspace(c); });

  data.erase(data.begin(), start);
  data.erase(end.base(), data.end());
}


static std::string getTextTokenText(const std::string& data, const XMLToken& token) {
  auto text = data.substr(token.start, token.end - token.start);
  trim(text);
  return text;
}

static XMLToken nextToken(const std::string& data, XMLTokenType lastType, size_t start) {
  XMLToken token{
    .type = XMLTokenType::none,
  };

  auto index = start;
  auto len = data.length();
  char lastChar = '\0';
  bool inElement = lastType == XMLTokenType::element || lastType == XMLTokenType::attributeName || lastType == XMLTokenType::attributeValue;
  bool opened = false;
  bool containsText = false;

  while (index < len) {
    char currentChar = data[index];

    switch (currentChar)
    {
    case '<':
      if (token.type == XMLTokenType::text) {
        token.end = index - 1;
        token.text = getTextTokenText(data, token);
        token.exit = index;
        return token;
      }
      break;
    case '?':
      if (lastChar != '<') {
        containsText = true;
        break;
      }

      // index++ later, so subtract 1 for now
      index = skipToAfter(data, index, ">") - 1;
      currentChar = '\0';
      break;
    case '!':
      if (lastChar != '<') {
        containsText = true;
        break;
      }

      // index++ later, so subtract 1 for now
      index = skipToAfter(data, index, "-->") - 1;
      currentChar = '\0';
      break;
    case '/': {
      if (lastChar != '<') {
        containsText = true;
        break;
      }

      token.type = XMLTokenType::close;
      token.start = index + 1;
      token.end = skipToAfter(data, index, ">") - 1;
      token.text = data.substr(token.start, token.end - token.start);
      trim(token.text);

      token.exit = token.end + 2;
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
    .name = token.text
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
        .name = token.text,
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
