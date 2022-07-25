/*! \brief Player controlled net navi prefab 
 * 
 * This class is the foundation for all player controlled characters
 * To write a new netnavi inherit from this class
 * And simply load new sprites and animation file
 * And change some properties
 */

#pragma once

#include "bnCharacter.h"
#include "bnEmotions.h"
#include "bnPlayerState.h"
#include "bnResourcePaths.h"
#include "bnChargeEffectSceneNode.h"
#include "bnCardPackageManager.h"
#include "bnCardBuilderTrait.h"
#include "bnAnimationComponent.h"
#include "bnAI.h"
#include "bnPlayerSpecialButton.h"
#include "bnPlayerControlledState.h"
#include "bnPlayerIdleState.h"
#include "bnPlayerHitState.h"
#include "bnPlayerChangeFormState.h"
#include "bnPlayerForm.h"
#include "bnDefenseSuperArmor.h"
#include "bnSyncNode.h"

#include <array>

using sf::IntRect;

struct PlayerStats {
  static constexpr unsigned MAX_CHARGE_LEVEL = 5u;
  static constexpr unsigned MAX_ATTACK_LEVEL = 5u;
  static constexpr unsigned MAX_SPEED_LEVEL = 5u;

  unsigned charge{ 1 }, attack{ 1 }, speed{ 1 };
  int moddedHP{};
  Element element{Element::none};
};

struct BusterEvent {
  frame_time_t deltaFrames{}; //!< e.g. how long it animates
  frame_time_t endlagFrames{}; //!< Wait period after completition

  // Default is false which is shoot-then-move
  bool blocking{}; //!< If true, blocks incoming move events for auto-fire behavior
  std::shared_ptr<CardAction> action{ nullptr };
};

struct PlayerCardMeta final : public CardBuilderTrait {
  Battle::Card::Properties properties;
  std::shared_ptr<sf::Texture> iconTexture; /*!< Icon used in hand */
  std::shared_ptr<sf::Texture> previewTexture; /*!< Picture used in select widget */
  char code{ 'A' }; // only 1 code
  ~PlayerCardMeta() {};
};

class Player : public Character, public AI<Player> {
private:
  friend class PlayerControlledState;
  friend class PlayerNetworkState;
  friend class PlayerIdleState;
  friend class PlayerHitState;
  friend class PlayerChangeFormState;
  
  void SaveStats();
  void RevertStats();
  void CreateMoveAnimHash();
  void CreateRecoilAnimHash();
  void TagBaseNodes();

public:
  using DefaultState = PlayerControlledState;
  static constexpr size_t MAX_FORM_SIZE = 5;
  static constexpr const char* BASE_NODE_TAG = "Base Node";
  static constexpr const char* FORM_NODE_TAG = "Form Node";

   /**
   * @brief Loads graphics and adds a charge component
   */
  Player();
  
  /**
   * @brief deconstructor
   */
  virtual ~Player();

  void Init() override;

  /**
   * @brief Polls for interrupted states and fires delete state when deleted
   * @param _elapsed for secs
   */
  virtual void OnUpdate(double _elapsed);

  void MakeActionable() override final;
  bool IsActionable() const override final;

  /**
   * @brief Fires a buster
   */
  void Attack();

  void UseSpecial();

  void HandleBusterEvent(const BusterEvent& event, const ActionQueue::ExecutionType& exec);

  /**
   * @brief when player is deleted, changes state to delete state and hide charge component
   */
  void OnDelete() override;

  const float GetHeight() const override;

  /**
   * @brief Get how many times the player has moved across the grid
   * @return int
   */
  int GetMoveCount() const;

  /**
   * @brief Toggles the charge component
   * @param state
   */
  void Charge(bool state);

  void SetAttackLevel(unsigned lvl);
  const unsigned GetAttackLevel();

  void SetChargeLevel(unsigned lvl);
  const unsigned GetChargeLevel();

  void SetSpeedLevel(unsigned lvl);
  const unsigned GetSpeedLevel();

  void ModMaxHealth(int mod);
  const int GetMaxHealthMod();
  
  /** Sets the player's emotion. @note setting an emotion (e.g. full_synchro) does not trigger its effects, it merely
   * tracks state */
  void SetEmotion(Emotion emotion);
  Emotion GetEmotion() const;

  /**
   * @brief Set the animation and on finish callback
   * @param _state name of the animation
   */
  void SetAnimation(string _state, std::function<void()> onFinish = nullptr);

  void SetSpecialButton1(const std::shared_ptr<PlayerSpecialButton>& button);
  void SetSpecialButton2(const std::shared_ptr<PlayerSpecialButton>& button);

  const std::string GetMoveAnimHash();
  const std::string GetRecoilAnimHash();

  void SlideWhenMoving(bool enable = true, const frame_time_t& = frames(1));

  virtual std::shared_ptr<CardAction> OnExecuteBusterAction() = 0; // custom
  virtual std::shared_ptr<CardAction> OnExecuteChargedBusterAction() = 0; // custom
  virtual std::shared_ptr<CardAction> OnExecuteSpecialAction(); // can be replaced by other mods
  virtual std::shared_ptr<PlayerCardMeta> BuildSpecialCard(); // can be replaced
  virtual frame_time_t CalculateChargeTime(const unsigned chargeLevel); // can be replaced

  std::shared_ptr<CardAction> ExecuteBuster();
  std::shared_ptr<CardAction> ExecuteChargedBuster();
  std::shared_ptr<CardAction> ExecuteSpecial();

  void ActivateFormAt(int index); 
  void DeactivateForm();
  const bool IsInForm() const;

  const std::vector<PlayerFormMeta*> GetForms();
  std::shared_ptr<PlayerSpecialButton> GetSpecialButton1();
  std::shared_ptr<PlayerSpecialButton> GetSpecialButton2();

  ChargeEffectSceneNode& GetChargeComponent();

  void OverrideSpecialAbility(const std::function<std::shared_ptr<CardAction> ()>& func);

  std::shared_ptr<SyncNode> AddSyncNode(const std::string& point);
  void RemoveSyncNode(std::shared_ptr<SyncNode> syncNode);

protected:
  // functions
  void FinishConstructor();
  bool RegisterForm(PlayerFormMeta* meta);
  void RefreshEmotionColors(Emotion emotion);

  // add form from class type
  template<typename T>
  PlayerFormMeta* AddForm();

  // add form from existing meta
  PlayerFormMeta* AddForm(PlayerFormMeta* meta);

  // provide to card select GUI if there are special GUI behaviors
  std::shared_ptr<PlayerSpecialButton> specialButton1, specialButton2;

  // member vars
  string state; /*!< Animation state name */
  string moveAnimHash, recoilAnimHash; //!< Enforce frame times for players
  frame_time_t slideFrames{ frames(1) };
  bool playerControllerSlide{};
  bool fullyCharged{}; //!< Per-frame value of the charge
  std::shared_ptr<AnimationComponent> animationComponent;
  std::shared_ptr<ChargeEffectSceneNode> chargeEffect; /*!< Handles charge effect */

  std::vector<PlayerFormMeta*> forms;
  PlayerForm* activeForm{ nullptr };
  Emotion emotion{ Emotion::normal };
  PlayerStats stats{}, savedStats{};
  std::function<std::shared_ptr<CardAction>()> specialOverride{};
  std::shared_ptr<DefenseSuperArmor> superArmor{ nullptr };
  SyncNodeContainer syncNodeContainer;
};

template<typename T>
PlayerFormMeta* Player::AddForm() {
  PlayerFormMeta* meta = new PlayerFormMeta(forms.size()+1u);
  meta->SetFormClass<T>();
  
  if (!RegisterForm(meta)) {
    delete meta;
    meta = nullptr;
  }

  return meta;
}