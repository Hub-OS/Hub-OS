#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

class Mail {
public:
  enum class Icons : char {
    announcement = 0,
    dm,
    dm_w_attachment,
    important,
    mission,
    size // For counting only!
  };

  struct Message {
    Icons icon{};
    std::string title;
    std::string from;
    std::string body;
    sf::Texture mugshot;
    bool read{};
  };

  void AddMessage(const Message& msg);
  void RemoveMessage(size_t index);
  void ReadMessage(size_t index, std::function<void(const Message& msg)> onRead);
  const Message& GetAt(size_t index) const;
  void Clear();
  size_t Size() const;
  bool Empty() const;
private:
  std::vector<Message> inbox;
};