#pragma once
#include "bnEngine.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

#include <Swoosh/Activity.h>

/**
 * @class FolderChangeNameScene
 * @author mav
 * @date 08/10/19
 * @brief Use controller to select letter from the table to spell out a folder name
 *
 * Modifies target folder in the input collection
 */
class FolderChangeNameScene : public swoosh::Activity {
private:
  sf::Sprite bg; /*!< Most of the elements on the screen are static */
  bool leave; /*!< Scene state coming/going flag */

public:
  FolderChangeNameScene(swoosh::ActivityController&);
  ~FolderChangeNameScene();

  void onStart();

  /**
   * @brief Poll for user input and edit folder name
   * @param elapsed in seconds
   */
  void onUpdate(double elapsed);

  void onLeave() { ; }
  void onExit() { ; }
  void onEnter() { ; }

  void onResume() { ; }

  /**
   * @brief Draws graphic
   * @param surface
   */
  void onDraw(sf::RenderTexture& surface);

  void onEnd() { ; }
};
