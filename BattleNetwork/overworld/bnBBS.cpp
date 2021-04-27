#include "bnBBS.h"
#include "../bnTextureResourceManager.h"
#include "../bnAudioResourceManager.h"

float SCROLLBAR_X = 229;
float SCROLLBAR_Y = 10;
float SCROLLBAR_HEIGHT = 115;
float POST_HEIGHT = 16;
size_t PAGE_SIZE = 8;

static sf::Color lerp(sf::Color colorA, sf::Color colorB, float strength) {
  return sf::Color(
    (uint8_t)((float)colorA.r * (1 - strength) + (float)colorB.r * strength),
    (uint8_t)((float)colorA.g * (1 - strength) + (float)colorB.g * strength),
    (uint8_t)((float)colorA.b * (1 - strength) + (float)colorB.b * strength),
    (uint8_t)((float)colorA.a * (1 - strength) + (float)colorB.a * strength)
  );
}

static sf::Vector2f GetCursorTarget(size_t screenIndex) {
  return { 0, (float)screenIndex * POST_HEIGHT + POST_HEIGHT / 2.0f + 2.0f };
}

static float CalcScrollbarThumbY(size_t postCount, size_t topIndex) {
  if (postCount <= PAGE_SIZE) {
    return SCROLLBAR_Y;
  }

  auto scrollableCount = postCount - PAGE_SIZE;
  auto progress = (float)topIndex / (float)scrollableCount;
  return SCROLLBAR_Y + progress * SCROLLBAR_HEIGHT;
}

BBS::BBS(
  const std::string& topic,
  sf::Color color,
  const std::function<void(const std::string&)>& onSelect,
  const std::function<void()>& onClose
) :
  topicText(topic, Font::Style::thick),
  topicShadow(topic, Font::Style::thick),
  onSelect(onSelect),
  onClose(onClose)
{
  newAnimation.Load("resources/ui/new.animation");
  newAnimation << "FLICKER" << Animator::Mode::Loop;
  newNode.setTexture(Textures().LoadTextureFromFile("resources/ui/new.png"));

  shadows.setTexture(Textures().LoadTextureFromFile("resources/ui/bbs/bbs_shadows.png"));
  AddNode(&shadows);

  frame.setTexture(Textures().LoadTextureFromFile("resources/ui/bbs/bbs_frame.png"));
  frame.setPosition(2, 18);
  frame.SetLayer(-1);
  shadows.AddNode(&frame);

  postbg.setTexture(Textures().LoadTextureFromFile("resources/ui/bbs/bbs_post_bg.png"));
  postbg.setPosition(3, 3);
  postbg.SetLayer(-2);
  frame.AddNode(&postbg);

  scrollbarThumb.setTexture(Textures().LoadTextureFromFile("resources/ui/scrollbar.png"));
  scrollbarThumb.setOrigin(3, 4);
  scrollbarThumb.setPosition(SCROLLBAR_X, SCROLLBAR_Y);
  scrollbarThumb.SetLayer(-2);
  frame.AddNode(&scrollbarThumb);

  auto cursorTarget = GetCursorTarget(selectedIndex - topIndex);
  cursor.setPosition(cursorTarget);
  cursor.SetTarget(cursorTarget);
  cursor.SetLayer(-3);
  postbg.AddNode(&cursor);

  topicText.SetColor(sf::Color::White);
  topicText.setPosition(8, 3);
  topicShadow.setPosition(9, 4);
  AddNode(&topicShadow);
  AddNode(&topicText);

  SetTopic(topic);
  SetColor(color);
}

void BBS::SetTopic(const std::string& topic) {
  this->topic = topic;
}

void BBS::SetColor(sf::Color color) {
  shadows.setColor(color);
  frame.setColor(lerp(sf::Color::White, color, .025f));
  postbg.setColor(lerp(sf::Color::White, color, .5f));
  topicShadow.SetColor(lerp(sf::Color::Black, color, .7f));
}

void BBS::SetLastPageCallback(const std::function<void()>& callback) {
  onLastPage = callback;
}

void BBS::PrependPosts(const std::vector<BBS::Post>& newPosts) {
  posts.insert(posts.begin(), newPosts.begin(), newPosts.end());

  selectedIndex += newPosts.size();

  if (posts.size() > PAGE_SIZE) {
    topIndex += newPosts.size();
  }

  scrollbarThumb.setPosition(SCROLLBAR_X, CalcScrollbarThumbY(posts.size(), topIndex));
}

void BBS::PrependPosts(const std::string& id, const std::vector<BBS::Post>& newPosts) {
  auto iter = std::find_if(posts.begin(), posts.end(), [&](auto& post) { return post.id == id; });

  posts.insert(iter, newPosts.begin(), newPosts.end());

  auto insertIndex = iter - posts.begin();

  if ((size_t)insertIndex <= selectedIndex) {
    selectedIndex += newPosts.size();

    if (posts.size() > PAGE_SIZE) {
      topIndex += newPosts.size();
    }

    scrollbarThumb.setPosition(SCROLLBAR_X, CalcScrollbarThumbY(posts.size(), topIndex));
  }
}

void BBS::AppendPosts(const std::vector<BBS::Post>& newPosts) {
  posts.insert(posts.end(), newPosts.begin(), newPosts.end());
  reachedEnd = false;
}

void BBS::AppendPosts(const std::string& id, const std::vector<BBS::Post>& newPosts) {
  auto iter = std::find_if(posts.begin(), posts.end(), [&](auto& post) { return post.id == id; });

  if (iter == posts.end()) {
    iter -= 1;
  }

  posts.insert(iter + 1, newPosts.begin(), newPosts.end());

  auto insertIndex = iter - posts.begin();

  if ((size_t)insertIndex <= selectedIndex) {
    selectedIndex += newPosts.size();

    if (posts.size() > PAGE_SIZE) {
      topIndex += newPosts.size();
    }

    scrollbarThumb.setPosition(SCROLLBAR_X, CalcScrollbarThumbY(posts.size(), topIndex));
  }
  else {
    reachedEnd = false;
  }
}

void BBS::AppendPost(const std::string& id, bool read, const std::string& title, const std::string& author) {
  posts.push_back(Post{
    id,
    read,
    title,
    author
    });
}

void BBS::RemovePost(const std::string& id) {
  auto iter = std::find_if(posts.begin(), posts.end(), [&](auto& post) { return post.id == id; });

  if (iter != posts.end()) {
    posts.erase(iter);
  }

  auto removeIndex = iter - posts.begin();

  if ((size_t)removeIndex <= selectedIndex && selectedIndex > 0) {
    selectedIndex -= 1;

    if (topIndex > 0) {
      topIndex -= 1;
    }

    scrollbarThumb.setPosition(SCROLLBAR_X, CalcScrollbarThumbY(posts.size(), topIndex));
  }
}

void BBS::HandleInput(InputManager& input) {
  if (input.Has(InputEvents::pressed_confirm) && selectedIndex < posts.size()) {
    auto& post = posts[selectedIndex];
    post.read = true;
    onSelect(post.id);
    return;
  }

  if (input.Has(InputEvents::pressed_cancel)) {
    Audio().Play(AudioType::CHIP_DESC_CLOSE);
    onClose();
    return;
  }

  auto up = input.Has(InputEvents::held_ui_up);
  auto down = input.Has(InputEvents::held_ui_down);
  auto pageUp = input.Has(InputEvents::held_shoulder_left);
  auto pageDown = input.Has(InputEvents::held_shoulder_right);

  if (!up & !down & !pageUp && !pageDown) {
    nextCooldown = 0.2f;
    cooldown = 0;
    return;
  }
  else if (cooldown > 0) {
    return;
  }

  auto oldIndex = selectedIndex;

  if (up && selectedIndex > 0) {
    selectedIndex -= 1;
  }

  if (down && posts.size() > 0 && selectedIndex < posts.size() - 1) {
    selectedIndex += 1;
  }

  if (selectedIndex < topIndex) {
    topIndex = selectedIndex;
  }
  else if (selectedIndex >= topIndex + PAGE_SIZE) {
    topIndex += 1;
  }

  if (pageUp) {
    auto offsetIndex = selectedIndex - topIndex;

    if (topIndex > PAGE_SIZE) {
      topIndex -= PAGE_SIZE;
    }
    else {
      topIndex = 0;
    }

    selectedIndex = topIndex + offsetIndex;
  }

  if (pageDown && posts.size() > PAGE_SIZE) {
    auto offsetIndex = selectedIndex - topIndex;

    if (topIndex < posts.size() - (PAGE_SIZE * 2 - 1)) {
      topIndex += PAGE_SIZE;
    }
    else {
      topIndex = posts.size() - 8;
    }

    selectedIndex = std::min(topIndex + offsetIndex, posts.size() - 1);
  }

  if (oldIndex != selectedIndex) {
    Audio().Play(AudioType::CHIP_SELECT);
    cooldown = nextCooldown;
    nextCooldown = 0.05f;

    scrollbarThumb.setPosition(SCROLLBAR_X, CalcScrollbarThumbY(posts.size(), topIndex));
  }

  if (!reachedEnd && selectedIndex + PAGE_SIZE >= posts.size()) {
    reachedEnd = true;
    onLastPage ? onLastPage() : (void)0;
  }
}

void BBS::Update(float elapsed) {
  if (cooldown > 0) {
    cooldown -= elapsed;
  }

  cursor.SetTarget(GetCursorTarget(selectedIndex - topIndex));
  cursor.Update(elapsed);
  newAnimation.Update(elapsed, newNode.getSprite());
}

void BBS::draw(sf::RenderTarget& surface, sf::RenderStates states) const {
  // draw nodes
  states.transform *= getTransform();
  SceneNode::draw(surface, states);

  // calculate positions
  auto postBgPos = postbg.getPosition() + frame.getPosition();

  auto unreadLeft = postBgPos.x + 14;
  auto titleLeft = postBgPos.x + 34;
  auto authorLeft = postBgPos.x + 154;

  float startOffset = postBgPos.y;

  // drawing posts
  auto text = Text(Font::Style::thin);
  auto textColor = sf::Color(0x4A414AFF);
  auto shadowColor = sf::Color(0, 0, 0, 25);
  auto endIndex = std::min(topIndex + PAGE_SIZE, posts.size());

  auto newSprite = newNode.getSprite();

  for (auto i = topIndex; i < endIndex; i++) {
    auto& post = posts[i];

    auto y = startOffset + (i - topIndex) * POST_HEIGHT;

    if (!post.read) {
      newSprite.setPosition(unreadLeft, y + POST_HEIGHT * 0.5f + 2.0f);
      surface.draw(newSprite, states);
    }

    text.setPosition(titleLeft + 1, y + 1);
    text.SetColor(shadowColor);
    text.SetString(post.title);
    surface.draw(text, states);

    text.setPosition(titleLeft, y);
    text.SetColor(textColor);
    text.SetString(post.title);
    surface.draw(text, states);

    text.setPosition(authorLeft + 1, y + 1);
    text.SetColor(shadowColor);
    text.SetString(post.author);
    surface.draw(text, states);

    text.setPosition(authorLeft, y);
    text.SetColor(textColor);
    text.SetString(post.author);
    surface.draw(text, states);
  }
}
