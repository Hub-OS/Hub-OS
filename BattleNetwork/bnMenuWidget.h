#pragma once
#include <Swoosh/Timer.h>
#include <Swoosh/Ease.h>

#include "bnSceneNode.h"
#include "bnSpriteProxyNode.h"
#include "bnAnimation.h"
#include "bnResourceHandle.h"
#include "bnFont.h"
#include "bnText.h"

/**
 * @class MenuWidget
 * @author mav
 * @date 11/04/20
 * @brief MenuWidget used in over-world hub. Can be interacted through public API.
 */
class MenuWidget : public SceneNode, public ResourceHandle {
public:
  enum class state : unsigned {
    closed = 0,
    opened,
    closing,
    opening
  };

  struct Options {
    std::string name;
    std::function<void()> onSelectFunc;
  };

  using OptionsList = std::vector<Options>;

private:
  int row{ 0 }; //!< Current row index
  int health{}, maxHealth{}; //!< Health displayed by main character
  int coins{}; //!< Coins held by main character
  float opacity{}; //!< Background darkens
  bool selectExit{ false }; //!< If exit option is selected
  bool extendedHold{ false }; //!< If player holds the arrow keys down
  state currState{}; //!< Track all open/close states. Default is closed
  std::string areaName; //!< Area name typed out
  Font font; //!< Used by text
  std::shared_ptr<sf::Texture> iconTexture; //!< If supplying an icon, use this one
  std::shared_ptr<sf::Texture> widgetTexture; //!< texture used by widget
  Text areaLabel; //!< Area name displayed in widget
  mutable Text infoText; //!< Text obj used for all other info
  swoosh::Timer easeInTimer; //!< Timer used for animations
  SpriteProxyNode banner;
  SpriteProxyNode symbol;
  SpriteProxyNode icon;
  SpriteProxyNode exit;
  SpriteProxyNode infoBox;
  SpriteProxyNode selectTextSpr;
  SpriteProxyNode placeTextSpr;
  OptionsList optionsList;
  std::vector<std::shared_ptr<SpriteProxyNode>> options;
  std::vector<std::shared_ptr<SpriteProxyNode>> optionIcons;
  Animation infoBoxAnim;
  Animation optionAnim;
  Animation exitAnim;

  void QueueAnimTasks(const state& state);
  void CreateOptions();

public:
  /**
   * @brief Constructs main menu widget UI. The programmer must set info params using the public API
   */
  MenuWidget(const std::string& area, const OptionsList& options);

  /**
   * @brief Deconstructor
   */
  ~MenuWidget();

   /**
   * @brief Animators cursors and all animations
   * @param elapsed in seconds
   */
  void Update(float elapsed);

  /**
   * @brief Draws GUI and all child graphics on it
   * @param target
   * @param states
   */
  void draw(sf::RenderTarget& target, sf::RenderStates states) const;

  /// Set data

  /**
  * @brief Set the area name to display
  * @param name String of the area name (limited to 12 chars)
  */
  void SetArea(const std::string& name);

  /**
  * @brief Set the numer of coins to display
  * @param coins Integer of coins
  */
  void SetCoins(int coins);

  /**
  * @brief Set the health to display e.g. (health / 100)
  * @param health Integer of health 
  */
  void SetHealth(int health);

  /**
  * @brief Set the max health to display e.g. ( 100 / maxHealth )
  * @param maxHealth Integer of max health to display
  */
  void SetMaxHealth(int maxHealth);

  void UseIconTexture(const std::shared_ptr<sf::Texture> icon);

  void ResetIconTexture();

  /// GUI OPS
  
  /**
  * @brief Execute the current selection (exit or row options)
  * @return true status if a selection was executed, false otherwise
  * 
  * NOTE: query which selection was made to determine how to respond to the return status
  */
  bool ExecuteSelection();

  /**
  * @brief Select the exit option
  * @return true if the exit option is selected, false if already selected
  */
  bool SelectExit();

  /**
  * @brief Select the options column and reset the row index to 0
  * @return true if the options column is selected, false if already selected
  */
  bool SelectOptions();

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

  /**
  * @brief Open the widget and begin the open animations
  * @return true if the widget has beguin opening, false if already opening or opened
  */
  bool Open();

  /**
  * @brief Close the widget and begin the close animations
  * @return true if the widget has beguin closing, false if already closing or closed
  */
  bool Close();

  /**
  * @brief Query if the widget is fully opened
  * @return true if the widget is fully opened, false if any other state
  */
  bool IsOpen() const;

  /**
  * @brief Query if the widget is fully closed
  * @return true if the widget is fully closed, false if any other state
  */
  bool IsClosed() const;
};