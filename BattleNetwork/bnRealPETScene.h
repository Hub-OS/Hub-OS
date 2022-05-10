#pragma once
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Buffer.h>
#include "overworld/bnOverworldPlayerSession.h"
#include "overworld/bnOverworldPollingPacketProcessor.h"
#include "overworld/bnOverworldMenuSystem.h"
#include "bnKeyItemScene.h"
#include "bnInbox.h"
#include "bnScene.h"
#include "bnPA.h"
#include "bnCardFolderCollection.h"

using Overworld::PollingPacketProcessor;
using Overworld::ServerStatus;
using Overworld::PlayerSession;
using Overworld::MenuSystem;

namespace RealPET {
  class Homepage final : public Scene {
  private:
    double animElapsed{};
    bool lastIsConnectedState; /*!< Set different animations if the connection has changed */
    Poco::Net::SocketAddress remoteAddress; //!< server
    std::string host; // need to store host string to retain domain names
    std::shared_ptr<PollingPacketProcessor> packetProcessor;
    std::vector<KeyItemScene::Item> items;
    uint16_t maxPayloadSize{};
    ServerStatus serverStatus{ ServerStatus::offline };

    /*!< Current player package selection */
    std::shared_ptr<PlayerSession> playerSession;
    std::string currentNaviId, lastSelectedNaviId;

    MenuSystem menuSystem; // TODO: remove. we don't use all of these widgets...

    CardFolderCollection* folders{ nullptr }; /*!< Collection of folders */
    PA programAdvance;

    void UpdateServerStatus(ServerStatus status, uint16_t serverMaxPayloadSize);

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