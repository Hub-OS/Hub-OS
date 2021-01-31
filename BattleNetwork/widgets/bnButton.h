#include "../bnSpriteProxyNode.h"
#include "../bnText.h"
#include "../bnWidget.h"

class Button final : public Widget {
  mutable SpriteProxyNode img;
  mutable Text label;

public:
  Button(std::shared_ptr<Widget> parent, const std::string& labelStr);
  ~Button();

  const sf::FloatRect CalculateBounds() const override;
  void SetLabel(const std::string& labelStr);
  void SetImage(const std::string& path);
  void ClearImage();
};