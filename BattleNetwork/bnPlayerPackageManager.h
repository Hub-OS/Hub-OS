/*! \file bnPlayerPackageManager.h */
#pragma once

#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>
#include "bnPackageManager.h"
#include "bnElements.h"
#include "bnPlayer.h"
#include "stx/result.h"
#include "stx/tuple.h"

class Player; // forward decl

/*! \brief Player package entry detail. Assign details to display them in specific screens. */
struct PlayerMeta final : public PackageManager<PlayerMeta>::Meta<Player>{
  std::string special; /*!< The net navi's special description */
  std::string overworldAnimationPath; /*!< The net navi's overworld animation */
  std::string overworldTexturePath; /*!< The path of the texture to load */
  std::string mugshotTexturePath;
  std::string mugshotAnimationPath;
  std::string emotionsTexturePath; /*!< The path of the texture containing all possible emotions, ordered by emotion enum */
  std::string name; /*!< The net navi's name */
  std::shared_ptr<sf::Texture> iconTexture; /*!< Icon on the top of the screen */
  std::shared_ptr<sf::Texture> previewTexture; /*!< Roster profile picture */
  unsigned atk; /*!< Attack level of the net navi */
  int chargedAtk; /*!< Charged attack level of the net navi */
  double speed; /*!< The speed of the navi */
  int hp; /*!< The health of the navi */
  bool isSword; /*!< Is buster or sword based navi */

  /**
   * @brief Sets every number to 1 and string to "None"
   */
  PlayerMeta();

  /**
   * @brief Delete navi pointer
   */
  ~PlayerMeta();

  void PreGetData() override;

  /**
   * @brief Sets special description information of the navi
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetSpecialDescription(const std::string&& special);

  /**
   * @brief Sets attack level to display
   * @param atk attack strength
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetAttack(const unsigned atk);

  /**
   * @brief Sets charged attack level to display
   * @param atk charge attack strength
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetChargedAttack(const int atk);

  /**
   * @brief Sets speed level to display
   * @param speed how fast the navi is expected to be
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetSpeed(const double speed);

  /**
   * @brief Sets the health to display
   * @param HP health
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetHP(const int HP);

  /**
   * @brief Toggles if a sword based navi or buster
   * @param enabled true if sword, default is false
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetIsSword(const bool enabled);

  /**
 * @brief Sets the mugshot animation path used in textboxes
 * @return PlayerMeta& to chain
 */
  PlayerMeta& SetMugshotAnimationPath(const std::string& path);

  /**
   * @brief Sets the texture of the mugshot animation
   * @param texture
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetMugshotTexturePath(const std::string& texture);

  /**
   * @brief Sets the emotions texture atlas
   * @param texture
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetEmotionsTexturePath(const std::string& texture);

  /**
   * @brief Sets the overworld animation path used in menu screen
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetOverworldAnimationPath(const std::string& path);

  /**
   * @brief Sets the texture of the overworld animation
   * @param texture
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetOverworldTexturePath(const std::string& texture);

  /**
   * @brief Sets the texture of the preview used in select screen
   * @param texture
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetPreviewTexture(const std::shared_ptr<Texture> texture);

  /**
   * @brief Device and face generally of the navi
   * @param icon texture
   * @return PlayerMeta& to chain
   */
  PlayerMeta& SetIconTexture(const std::shared_ptr<sf::Texture> icon);

  /**
   * @brief Gets the icon texture to draw
   * @return const sf::Texture&
   */
  const std::shared_ptr<sf::Texture> GetIconTexture() const;

  /**
   * @brief Gets the overworld texture path to load
   * @return const std::string&
   */
  const std::string& GetOverworldTexturePath() const;

  /**
   * @brief Gets the overworld animation path
   * @return const std::string&
   */
  const std::string& GetOverworldAnimationPath() const;

  /**
 * @brief Gets the mugshot texture path
 * @return const std::string&
 */
  const std::string& GetMugshotTexturePath() const;

  /**
 * @brief Gets the mugshot animation path
 * @return const std::string&
 */
  const std::string& GetMugshotAnimationPath() const;

  /**
 * @brief Gets the emotions texture path
 * @return const std::string&
 */
  const std::string& GetEmotionsTexturePath() const;

  /**
   * @brief Gets the preview texture to draw
   * @return const sf::Texture&
   */
  const std::shared_ptr<sf::Texture> GetPreviewTexture() const;

  /**
   * @brief Gets the net navi name
   * @return const std::string (copy)
   */
  const std::string GetName() const;

  /**
   * @brief Gets the navi HP
   * @return int
   */
  int GetHP() const;

  /**
   * @brief Gets the navi HP as a string to display
   * @return const std::string (copy)
   */
  const std::string GetHPString() const;

  /**
   * @brief Gets the navi speed as a string to display
   * @return const std::string (copy)
   */
  const std::string GetSpeedString() const;

  /**
   * @brief Get the attack strength as string to display
   * @return const std::string (copy)
   */
  const std::string GetAttackString() const;

  /**
   * @brief Get the special description string to display
   * @return const std::string
   */
  const std::string& GetSpecialDescriptionString() const;

  /**
   * @brief Gets the element of the navi
   * @return Element
   */
  const Element GetElement() const;
};

class PlayerPackageManager : public PackageManager<PlayerMeta> {};
