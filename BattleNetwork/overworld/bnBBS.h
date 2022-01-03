#pragma once
#include "../bnResourceHandle.h"
#include "../bnSpriteProxyNode.h"
#include "../bnInputManager.h"
#include "../bnText.h"
#include "bnVerticalCursor.h"
#include <SFML/Graphics.hpp>
#include <memory>

class BBS : public ResourceHandle, public SceneNode {
public:
  struct Post {
    std::string id;
    bool read{};
    std::string title;
    std::string author;
  };

  BBS(const std::string& topic, sf::Color color, const std::function<void(const std::string&)>& onSelect, const std::function<void()>& onClose);

  void SetTopic(const std::string& topic);
  void SetColor(sf::Color color);
  void SetLastPageCallback(const std::function<void()>& callback);

  void PrependPosts(const std::vector<Post>& posts);
  void PrependPosts(const std::string& id, const std::vector<Post>& posts);
  void AppendPosts(const std::vector<Post>& posts);
  void AppendPosts(const std::string& id, const std::vector<Post>& posts);
  void AppendPost(const std::string& id, bool read, const std::string& title, const std::string& author);
  void RemovePost(const std::string& id);
  void Close();

  void HandleInput(InputManager& input);

  void Update(float elapsed);
  void draw(sf::RenderTarget& surface, sf::RenderStates states) const override final;
private:
  float cooldown{};
  float nextCooldown{};
  size_t topIndex{};
  size_t selectedIndex{};
  bool reachedEnd{};
  std::string topic;
  std::shared_ptr<Text> topicText, topicShadow;
  std::shared_ptr<SpriteProxyNode> shadows, frame, postbg, scrollbarThumb;
  SpriteProxyNode newNode;
  Animation newAnimation;
  std::vector<Post> posts;
  std::shared_ptr<VerticalCursor> cursor;
  std::function<void(const std::string&)> onSelect;
  std::function<void()> onClose;
  std::function<void()> onLastPage;
};
