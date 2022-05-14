#pragma once
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include "../overworld/bnOverworldPlayerSession.h"
#include "../overworld/bnOverworldPollingPacketProcessor.h"
#include "../overworld/bnOverworldMenuSystem.h"
#include "../bnKeyItemScene.h"
#include "../bnInbox.h"
#include "../bnScene.h"
#include "../bnPA.h"
#include "../bnCardFolderCollection.h"
#include "../bnInputRepeater.h"
#include "bnRevolvingMenuWidget.h"

using Overworld::PollingPacketProcessor;
using Overworld::ServerStatus;
using Overworld::PlayerSession;
using Overworld::MenuSystem;

template<typename T>
T rand_val(const T& min, const T& max) {
  int sample = rand() % RAND_MAX;
  double frac = static_cast<double>(sample) / static_cast<double>(RAND_MAX);
  
  return static_cast<T>(((1.0 - frac) * min) + (frac * max));
}

static int rand_val(const int& min, const int& max) {
  return (rand() % (max-min+1)) + (min);
}

static bool rand_val() {
  return rand() % RAND_MAX == 0;
}

static sf::Vector2f rand_val(const sf::Vector2f& min, const sf::Vector2f& max) {
  int sample = rand() % RAND_MAX;
  double frac = static_cast<double>(sample) / static_cast<double>(RAND_MAX);

  sf::Vector2f result{};
  result.x = (((1.0 - frac) * min.x) + (frac * max.x));

  // resample for y
  sample = rand() % RAND_MAX;
  frac = static_cast<double>(sample) / static_cast<double>(RAND_MAX);

  result.y = (((1.0 - frac) * min.y) + (frac * max.y));

  return result;
}

namespace RealPET {
  struct Particle {
    double lifetime{};
    double max_lifetime{};
    sf::Vector2f acceleration;
    sf::Vector2f velocity;
    sf::Vector2f friction;
    sf::Vector2f position;
    bool scaleIn{}, scaleOut{};
    int type{};
    // below is contextual and mostly unused
    std::optional<sf::Vector2f> click{};
  };

  struct StaticParticle {
    double lifetime{};
    double max_lifetime{};
    sf::Vector2f pos;
    // below is contextual and mostly unused
    double startup_delay{};
  };

  class Homepage final : public Scene {
  private:
    double animElapsed{};
    bool lastIsConnectedState; /*!< Set different animations if the connection has changed */
    bool playJackin{};
    bool mouseClicked{};
    size_t mouseBufferIdx{};
    std::array<sf::Vector2f, 5> lastMousef;
    sf::Sprite bgSprite, jackinSprite;
    std::shared_ptr<sf::Texture> bgTexture, folderTexture, windowTexture, jackinTexture, menuTexture, miscMenuTexture;
    Animation folderAnim, windowAnim, jackinAnim, menuAnim, miscMenuAnim;
    
    std::shared_ptr<sf::SoundBuffer> jackinsfx;

    size_t maxPoolSize{};
    std::vector<Particle> pool;

    size_t maxStaticPoolSize{};
    std::vector<StaticParticle> staticPool;

    Poco::Net::SocketAddress remoteAddress; //!< server
    std::string host; // need to store host string to retain domain names
    std::shared_ptr<PollingPacketProcessor> packetProcessor;
    std::vector<KeyItemScene::Item> items;
    uint16_t maxPayloadSize{};
    ServerStatus serverStatus{ ServerStatus::offline };

    /*!< Current player package selection */
    std::shared_ptr<PlayerSession> playerSession;
    std::string currentNaviId, lastSelectedNaviId;

    RevolvingMenuWidget menuWidget;
    InputRepeater repeater;

    CardFolderCollection* folders{ nullptr }; /*!< Collection of folders */
    PA programAdvance;

    void UpdateServerStatus(ServerStatus status, uint16_t serverMaxPayloadSize);
    void InitializeFolderParticles();
    void InitializeWindowParticles();
    void UpdateFolderParticles(double elapsed);
    void UpdateWindowParticles(double elapsed);
    void DrawFolderParticles(sf::RenderTexture& surface);
    void DrawWindowParticles(sf::RenderTexture& surface);

  public:

    /**
     * @brief Loads the player's library data and loads graphics
     */
    Homepage(swoosh::ActivityController&);
    ~Homepage();

    /**
     * @brief Checks input events and listens for select buttons. Segues to new screens.
     * @param elapsed in seconds
     */
    virtual void onUpdate(double elapsed) override;

    /**
     * @brief Draws the UI
     * @param surface
     */
    virtual void onDraw(sf::RenderTexture& surface) override;

    /**
     * @brief Stops music, plays new track
     */
    virtual void onStart() override;

    /**
     * @brief Does nothing
     */
    virtual void onLeave() override;

    /**
     * @brief Drops packet processor
     */
    virtual void onExit() override;

    /**
     * @brief Checks the selected navi if changed and loads the new images
     */
    virtual void onEnter() override;

    /**
     * @brief Resumes scene's music
     */
    virtual void onResume() override;

    /**
     * @brief Stops the music
     */
    virtual void onEnd() override;

    /**
    * @brief Update's the player sprites according to the most recent selection
    */
    void RefreshNaviSprite();

    /**
    * @brief Equip a folder for the navi that was last used when playing as them
    */
    void NaviEquipSelectedFolder();

    //
    // Menu selection callbacks
    //

    void GotoChipFolder();
    void GotoNaviSelect();
    void GotoConfig();
    void GotoMobSelect();
    void GotoPVP();
    void GotoMail();
    void GotoKeyItems();
    // void GotoPlayerCust();

    std::string& GetCurrentNaviID();
    PA& GetProgramAdvance();
    std::optional<CardFolder*> GetSelectedFolder();
    bool IsInFocus();
  };
}