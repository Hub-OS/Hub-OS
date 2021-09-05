#pragma once
#include "bnSceneNode.h"
#include "bnUIComponent.h"
#include "bnField.h"
#include "bnDefenseAura.h"
#include "bnInputHandle.h"

/**
 * @class Aura
 * @author mav
 * @date 14/05/19
 * @brief Aura is a scene node and a Component that protects entity from an amount of damage
 * 
 * Aura adds a DefenseRule to the entities damage resolution steps. When the total damage recieved
 * is greater or equal to the aura's health then the aura deletes the rule and removes itself from
 * the entity and its draw nodes.
 * 
 * The other type of Aura is a Barrier. Barrier is similar but each damage recieved takes away from the
 * Barrier's internal HP. When the HP is below or at zero, the Barrier deletes itself.
 */
class Aura : public DefenseAura, public Component, public InputHandle
{
public:
  /**
 * @brief Aura can be weak or strong Auras as well as Barriers */
  enum class Type : int {
    AURA_100,
    AURA_200,
    AURA_1000,
    BARRIER_10,
    BARRIER_200,
    BARRIER_500
  };

  // The Aura draws a barrier or aura graphic and text on the UI pass
  class VisualFX : public UIComponent {
    friend class Aura;
    bool showHP{ false };
    int currHP{};
    double timer{ 0 };
    Type type;
    Animation animation; /*!< Animation object */
    SpriteProxyNode* aura{ nullptr }; /*!< The scene node to attach to the entity's scene node */
    sf::Sprite auraSprite; /*!< the sprite drawn by SpriteSceneNode* aura */
    mutable Sprite font; /*!< Aura HP glyphs */
    std::shared_ptr<sf::Texture> fontTextureRef; /*!< reference to the texture set used */
    sf::Vector2f flySpeed{}, flyAccel{};
    Battle::Tile* flyStartTile{ nullptr };
  public:
    VisualFX(std::weak_ptr<Entity> owner, Aura::Type type);
    ~VisualFX();

  /**
   * @brief Injects into the scene so the animation updates regardless of battle paused.
   */
    void Inject(BattleSceneBase&) override;

    void OnUpdate(double _elapsed) override;

    void ShowHP(bool visible);

   /**
   * @brief Draws health using glyphs with correct margins
   * @param target
   * @param states
   */
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  };
  
  std::shared_ptr<VisualFX> fx{ nullptr };
private:

  Type type; /*!< Type of aura */
  double timer{ 0 }; /*!< Some aura types delete over time in seconds */
  int health{}; /*!< HP of aura */
  bool persist{ false }; /*!< Flag is Barrier goes away over time or if it lingers */
  bool isOver{ false }; /*!< Flag if the aura is fading out */
  bool defenseRuleRemoved{};
  int lastHP{}; /*!< HP last frame */
  int currHP{}; /*!< HP this frame */
  int startHP{}; /*!< HP at creation */

  void OnHitCallback(Spell& in, Character& owner, bool windRemove);
  void RemoveDefenseRule();
public:


  /**
   * @brief Create an aura with type and a Character owner
   * @param type of aura
   * @param Character ptr instead of std::shared_ptr<Entity> ptr
   */
  Aura(Type type, std::weak_ptr<Character> owner);
  
  /**
   * @brief Dtor. Deletes aura SceneNodeSprite and defense rule.
   */
  ~Aura();
  
  /**
   * @brief If timer or HP is over, remove the components and nodes in this recipe
   * @param _elapsed in seconds
   * 
   * First the defense rule is removed
   * Then the aura sprite scene node is removed from this scene node
   * Then this scene node removes itself from the entities scene node
   * Then the entity removes this component
   * Finally the component commits suicide
   * 
   * (scene nodes: Entity -> Aura Comp -> Aura Sprite)
   * 
   * If the player is off of the field, toggle Hide
   * If the plaer is back on the field, Show
   */
  void OnUpdate(double _elapsed) override;
  
  void Inject(BattleSceneBase&) override;
  void OnReplace() override;

  /**
   * @brief Query the type of aura
   * @return const Aura::Type
   */
  const Type GetAuraType();
  
  /**
   * @brief Take damage 
   * @param hitbox properties
   */
  void TakeDamage(Character& owner, const Hit::Properties& props);
  
  /**
   * @brief Get remaining HP
   * @return const int
   */
  const int GetHealth() const;
  
  /**
   * @brief Whether or not to use a timer on the aura
   * @param enable if true, aura goes away when HP is zero, if false aura will also go away when time runs out.
   */
  void Persist(bool enable);
  
  /**
   * @brief Query if the aura is persistent or not
   * @return true if persistence is enabled, false otherwise
   */
  const bool IsPersistent() const;
};

