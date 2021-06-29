#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

class Inbox {
public:
  enum class Icons : char {
    announcement = 0,
    dm,
    dm_w_attachment,
    important,
    mission,
    size // For counting only!
  };

  struct Mail {
    Icons icon{};
    std::string title;
    std::string from;
    std::string body;
    sf::Texture mugshot;
    bool read{};
  };

  void AddMail(const Mail& msg);
  void RemoveMail(size_t index);
  void ReadMail(size_t index, std::function<void(const Mail& msg)> onRead);
  const Mail& GetAt(size_t index) const;
  void Clear();
  size_t Size() const;
  bool Empty() const;
private:
  std::vector<Mail> mailList;
};