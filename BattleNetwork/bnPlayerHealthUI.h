/*! \brief Displays player health in a box for the main player 
 * 
 * UIComponent is drawn on top of everything else in the scene.
 * The health uses glyphs from a text file to look identical to
 * the source material. The glyphs use the green or orange rows
 * when the player is healing or taking damage respectively.
 * 
 * When player health drops low, a beep plays with HIGH priority
 * 
 * When the battle is over, the BattleOverTrigger is activated
 * and sets the internal flag to false which stops beeping
 */

#pragma once
#include <SFML/Graphics.hpp>
#include <sstream>
#include <vector>

#include "bnBattleOverTrigger.h"
#include "bnPlayer.h"
#include "bnUIComponent.h"

class Entity;
class Player;

using sf::Font;
using sf::Text;
using sf::Sprite;
using sf::Texture;
using sf::Drawable;
using std::vector;
using std::ostringstream;

class PlayerHealthUI : virtual public UIComponent, virtual public BattleOverTrigger<Player> {
public:
<<<<<<< HEAD
  PlayerHealthUI(Player* _player);
  ~PlayerHealthUI();

  virtual void Inject(BattleScene& scene) { ; }

  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;
  void Update(float elapsed);

private:
  int lastHP;
  int currHP;
  int startHP;
  Player* player;
  mutable Sprite glyphs;
  Sprite sprite;
  Texture* texture;

=======
  /**
   * \brief Sets the player owner. Sets hp tracker to current health.
   */
  PlayerHealthUI(Player* _player);
  
  /**
   * @brief No memory needs to be freed
   */
  ~PlayerHealthUI();

  /**
   * @brief This component does not need to be injected into the scene
   * @param scene
   */
  virtual void Inject(BattleScene& scene) { ; }

  /**
   * @brief Uses bitmap glyphs for each number in the health
   * @param target
   * @param states
   */
  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const;
  
  /**
   * @brief Quickly depletes health based on game rules and beeps if health is low
   * @param elapsed in seconds
   */
  void Update(float elapsed);

private:
  int lastHP; /*!< HP of target last frame */
  int currHP; /*!< HP of target current frame */
  int startHP; /*!< HP of target when this component was attached */
  Player* player; /*!< target entity of type Player */
  mutable Sprite glyphs; /*!< bitmap image object to draw */
  Sprite sprite; /*!< the box surrounding the health */
  Texture* texture; /*!< the texture of the box */

  /**
   * @class Color
   * @brief strong type for glyph colors
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  enum class Color : int {
    NORMAL,
    ORANGE,
    GREEN
<<<<<<< HEAD
  } color;

  bool loaded;
  bool isBattleOver;
  double cooldown;
=======
  } color; /*!< color of the glyphs */

  bool isBattleOver; /*!< flag when battle scene ends to stop beeping */
  double cooldown; /*!< timer to colorize the health. Set to 0.5 seconds */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};
