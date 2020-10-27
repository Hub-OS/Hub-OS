#pragma once
#include "bnSceneNode.h"
#include "bnUIComponent.h"
#include "bnField.h"

class DefenseAura;

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
class Aura : public Component
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
    Type type;
    int currHP{};
    Animation animation; /*!< Animation object */
    SpriteProxyNode* aura{ nullptr }; /*!< The scene node to attach to the entity's scene node */
    sf::Sprite auraSprite; /*!< the sprite drawn by SpriteSceneNode* aura */
    mutable Sprite font; /*!< Aura HP glyphs */
    std::shared_ptr<sf::Texture> fontTextureRef; /*!< reference to the texture set used */
    double timer{ 0 };
  public:
    VisualFX(Entity* owner, Aura::Type type);
    ~VisualFX();

  /**
   * @brief Injects into the scene so the animation updates regardless of battle paused.
   */
    void Inject(BattleSceneBase&) override;

    void OnUpdate(float _elapsed) override;

   /**
   * @brief Draws health using glyphs with correct margins
   * @param target
   * @param states
   */
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
  } *fx{ nullptr };
private:

  Type type; /*!< Type of aura */
  DefenseAura* defense{ nullptr }; /*!< Defense rule */
  double timer{ 0 }; /*!< Some aura types delete over time in seconds */
  int health{}; /*!< HP of aura */
  bool persist{ false }; /*!< Flag is Barrier goes away over time or if it lingers */
  bool isOver{ false }; /*!< Flag if the aura is fading out */
  int lastHP{}; /*!< HP last frame */
  int currHP{}; /*!< HP this frame */
  int startHP{}; /*!< HP at creation */

public:
  /**
   * @brief Create an aura with type and a Character owner
   * @param type of aura
   * @param Character ptr instead of Entity* ptr
   */
  Aura(Type type, Character* owner);
  
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
  void OnUpdate(float _elapsed) override;
  
  void Inject(BattleSceneBase&) override;

  /**
   * @brief Query the type of aura
   * @return const Aura::Type
   */
  const Type GetAuraType();
  
  /**
   * @brief Take damage 
   * @param damage amount of HP to lose
   */
  void TakeDamage(int damage);
  
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

