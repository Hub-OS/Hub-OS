#include "../bnSpriteProxyNode.h"
#include "../bnText.h"
#include "../bnWidget.h"

class Button final : public Widget, std::enable_shared_from_this<Button> {
  mutable std::shared_ptr<SpriteProxyNode> img;
  mutable std::shared_ptr<Text> label;
  Widget::Layout* btnLayout{ nullptr };

  class ButtonLayout final : public Widget::Layout {
    Button* btn{ nullptr };

  public:
    ButtonLayout(Button* btn);
    const sf::FloatRect CalculateBounds() const;
  };

public:
  Button(const std::string& labelStr);
  ~Button();

  void SetLabel(const std::string& labelStr);
  void SetImage(const std::string& path);
  void ClearImage();
};