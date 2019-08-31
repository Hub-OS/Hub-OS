#pragma once
#include "bnEngine.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"

#include "bnAnimation.h"
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
  sf::Font *font;
  sf::Text *nameLabel;
  std::string name;
  int letterPos; /*!< Where the name will begin writing to*/
  std::vector<std::vector<std::vector<std::string>>> table; /*!< 3 tables with rows and columns */
  int cursorPosX; /*!< x location in column */
  int cursorPosY; /*!< y location in column */
  int currTable;  /*!< which table we're on */
  sf::Sprite cursor;
  Animation animator;
  float elapsed;
  std::string& folderName;

  /*!\brief performs the A button action depending on which table it resides*/
  void ExecuteAction(size_t table, size_t x, size_t y);

  void DoBACK();
  void DoNEXT();
  void DoOK();
  void DoEND();
public:
  FolderChangeNameScene(swoosh::ActivityController&, std::string& folderName);
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
