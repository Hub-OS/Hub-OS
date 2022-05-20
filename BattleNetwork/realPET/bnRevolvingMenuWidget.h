#pragma once

#include "../bnMenu.h"
#include "../overworld/bnOverworldPlayerSession.h"
#include "../bnInputManager.h"
#include "../bnSpriteProxyNode.h"
#include "../bnAnimation.h"
#include "../bnResourceHandle.h"
#include "../bnFont.h"
#include "../bnText.h"
#include "../bnPlayerHealthUI.h"
#include <Swoosh/Timer.h>
#include <Swoosh/Ease.h>

using Overworld::PlayerSession;

namespace RealPET {
  class RevolvingMenuWidget : public Menu, public ResourceHandle {
  private:
    std::shared_ptr<PlayerSession> session;
    frame_time_t frameTick;
    int row{ 0 }; //!< Current row index
    float opacity{}; //!< Background darkens
    double elapsedThisFrame{};
    std::shared_ptr<sf::Texture> widgetTexture; //!< texture used by widget
    mutable Text infoText; //!< Text obj used for all other info
    mutable Text time; //!< Text obj displaying the time both inside and outside of the widget
    swoosh::Timer easeInTimer; //!< Timer used for animations
    OptionsList optionsList;
    std::vector<std::shared_ptr<SpriteProxyNode>> options;
    std::vector<std::shared_ptr<SpriteProxyNode>> optionIcons;
    Animation infoBoxAnim;
    Animation optionAnim;

    struct barrel {
      double startAngle{}; // which way out the barrel is facing
      double phaseWidth{}; // the total surface we want to map the surface to
      double radius{}; // of the barrel

      sf::Vector2f calculate_point(unsigned int idx, unsigned int max) {
        double idxRadians = startAngle;
        idxRadians -= (phaseWidth / 2.0);
        idxRadians += (idx*(phaseWidth)) / static_cast<double>(max);
        return sf::Vector2f(radius * std::cosf(idxRadians), radius * std::sinf(idxRadians));
      }
    };

    const double defaultPhaseWidth = swoosh::ease::pi;
    double phaseWidth = defaultPhaseWidth;

    void CreateOptions();
    void DrawTime(sf::RenderTarget& target) const;

  public:
    /**
     * @brief Constructs main menu widget UI. The programmer must set info params using the public API
     */
    RevolvingMenuWidget(const std::shared_ptr<PlayerSession>& session, const Menu::OptionsList& options);

    /**
     * @brief Deconstructor
     */
    ~RevolvingMenuWidget();

    /**
    * @brief Animators cursors and all animations
    * @param elapsed in seconds
    */
    void Update(double elapsed) override;

    /**
    * @brief handles specific key presses to interact with this widget
    * @param 
    */
    void HandleInput(InputManager&, sf::Vector2f mousePos) override;

    /**
     * @brief Draws GUI and all child graphics on it
     * @param target
     * @param states
     */
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    /// GUI OPS

    /**
    * @brief Execute the current selection (exit or row options)
    * @return true status if a selection was executed, false otherwise
    *
    * NOTE: query which selection was made to determine how to respond to the return status
    */
    bool ExecuteSelection();

    /**
    * @brief Move the cursor up one
    * @return true if the options column is able to move up, false if it cannot move up
    */
    bool CursorMoveUp();

    /**
    * @brief Move the cursor down one
    * @return true if the options column is able to move down, false if it cannot move down
    */
    bool CursorMoveDown();
  };
}
