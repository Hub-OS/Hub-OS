
/*! \file bnMobPackageManager.h */

#pragma once

#include <filesystem>
#include <map>
#include <vector>
#include <functional>
#include <atomic>
#include <SFML/Graphics.hpp>

#include "bnPackageManager.h"
#include "bnTextureResourceManager.h"
#include "bnField.h"
#include "bnMobFactory.h"
#include "bnResourceHandle.h"
#include "stx/tuple.h"

class Mob;

typedef int SelectedMob;

struct MobMeta final : public PackageManager<MobMeta>::Meta<MobFactory> {
  sf::Sprite placeholder; /*!< Preview image for the mob in select screen */
  std::string name;       /*!< Name of the mob */
  std::string description;/*!< Description of mob that shows up in the text box */
  std::filesystem::path placeholderPath; /*!< Path to the preview image */
  std::shared_ptr<sf::Texture> placeholderTexture; /*!< Texture of the preview image */
  int atk; /*!< Strength of mob to display */
  double speed; /*!< Speed of mob to display */
  int hp; /*!< Total health of mob to display */

  /**
    * @brief Sets mob to temp data
    */
  MobMeta();

  /**
    * @brief deconstructor
    */
  ~MobMeta();

  void OnMetaParsed() override;

  /**
    * @brief Sets the preview path to load
    * @param path path to preview texture
    * @return MobMeta& to chain
    */
  MobMeta& SetPlaceholderTexturePath(const std::filesystem::path& path);

  /**
    * @brief Sets the description that shows up in text box
    * @param message
    * @return MobMeta& to chain
    */
  MobMeta& SetDescription(const std::string& message);

  /**
    * @brief Sets the attack to display on the select screen
    * @param atk strength of the mob
    * @return MobMeta& to chain
    */
  MobMeta& SetAttack(const int atk);

  /**
    * @brief Sets the speed to display on the select screen
    * @param speed of the mob
    * @return MobMeta& to chain
    */
  MobMeta& SetSpeed(const double speed);

  /**
    * @brief Sets the total HP of the mob
    * @param HP health of the mob to display
    * @return MobMeta& to chain
    */
  MobMeta& SetHP(const int HP);

  /**
    * @brief Clever title of the mob
    * @param name name of the mob to display
    * @return MobMeta& to chain
    */
  MobMeta& SetName(const std::string& name);

  /**
    * @brief Gets the preview texture
    * @return const std::shared_ptr<sf::Texture>
    */
  const std::shared_ptr<sf::Texture> GetPlaceholderTexture() const;

  /**
    * @brief Gets the preview texture path
    * @return const std::string
    */
  const std::filesystem::path GetPlaceholderTexturePath() const;

  /**
    * @brief Gets the name of the mob
    * @return const std::string
    */
  const std::string GetName() const;

  /**
    * @brief Gets the health of the mob as a string
    * @return const std::string
    */
  const std::string GetHPString() const;

  /**
    * @brief Gets the speed of the mob as a string
    * @return const std::string
    */
  const std::string GetSpeedString() const;

  /**
    * @brief Gets the attack of the mob as a string
    * @return const std::string
    */
  const std::string GetAttackString() const;

  /**
    * @brief Gets the mob description to show up in message box
    * @return const std::string
    */
  const std::string GetDescriptionString() const;
};

class MobPackageManager : public PackageManager<MobMeta> {
public:
  MobPackageManager(const std::string& ns) : PackageManager<MobMeta>(ns) {}
};
class MobPackagePartitioner : public PackagePartitioner<MobPackageManager> {};
