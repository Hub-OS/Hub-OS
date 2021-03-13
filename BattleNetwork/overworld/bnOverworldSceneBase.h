#pragma once

#include <SFML/Graphics.hpp>
#include <time.h>
#include <future>
#include <Swoosh/Activity.h>
#include <Swoosh/ActivityController.h>

#include "../bnScene.h"
#include "../bnCamera.h"
#include "../bnInputManager.h"
#include "../bnAudioResourceManager.h"
#include "../bnShaderResourceManager.h"
#include "../bnTextureResourceManager.h"
#include "../bnNaviRegistration.h"
#include "../bnPA.h"
#include "../bnDrawWindow.h"
#include "../bnAnimation.h"
#include "../bnCardFolderCollection.h"

// overworld
#include "bnOverworldSprite.h"
#include "bnOverworldActor.h"
#include "bnOverworldPlayerController.h"
#include "bnOverworldPathController.h"
#include "bnOverworldTeleportController.h"
#include "bnOverworldSpatialMap.h"
#include "bnOverworldMap.h"
#include "bnOverworldPersonalMenu.h"
#include "bnOverworldTextBox.h"
#include "bnEmotes.h"
#include "bnXML.h"

class Background; // forward decl

namespace Overworld {
  class SceneBase : public Scene {
  private:
    std::shared_ptr<Actor> playerActor;
    Overworld::EmoteWidget emote;
    Overworld::EmoteNode emoteNode;
    Overworld::TeleportController teleportController{};
    Overworld::PlayerController playerController{};
    Overworld::SpatialMap spatialMap{};
    std::vector<std::shared_ptr<Overworld::Actor>> actors;
    bool inputLocked{ false };

    struct QueuedCameraEvent {
      bool unlock;
      bool slide;
      sf::Vector2f position;
      sf::Time duration;
    };

    std::queue<QueuedCameraEvent> cameraQueue;
    Camera camera; /*!< camera in scene follows player */
    bool cameraLocked{ false };
    swoosh::Timer cameraTimer;
    bool clicked{ false }, scaledmap{ false };

    sf::Vector2f returnPoint{};
    bool teleportedOut{ false }; /*!< We may return to this area*/

    PersonalMenu personalMenu;

    SpriteProxyNode webAccountIcon; /*!< Status icon if connected to web server*/
    Animation webAccountAnimator; /*!< Use animator to represent different statuses */
    bool lastIsConnectedState; /*!< Set different animations if the connection has changed */

    // Bunch of sprites and their attachments
    Background* bg{ nullptr }; /*!< Background image pointer */
    Overworld::Map map; /*!< Overworld map */
    std::vector<std::shared_ptr<WorldSprite>> sprites;

    Overworld::TextBox textbox;

    double animElapsed{};

    /*!< Current navi selection index */
    SelectedNavi currentNavi{},
      lastSelectedNavi{ std::numeric_limits<SelectedNavi>::max() };

    bool gotoNextScene{ false }; /*!< If true, player cannot interact with screen yet */
    bool guestAccount{ false };

    CardFolderCollection folders; /*!< Collection of folders */
    PA programAdvance;

    std::future<WebAccounts::AccountState> accountCommandResponse; /*!< Response object that will wait for data from web server*/

    std::shared_ptr<Tileset> ParseTileset(const XMLElement& element, unsigned int firstgid);
    std::vector<std::shared_ptr<Overworld::TileMeta>> ParseTileMetas(const XMLElement& tilesetElement, const Overworld::Tileset& tileset);
    void HandleCamera(float elapsed);
    void HandleInput();
    void LoadBackground(const std::string& value);
    void DrawMap(sf::RenderTarget& target, sf::RenderStates states);
    void DrawTiles(sf::RenderTarget& target, sf::RenderStates states);
    void DrawSprites(sf::RenderTarget& target, sf::RenderStates states) const;

#ifdef __ANDROID__
    void StartupTouchControls();
    void ShutdownTouchControls();
#endif

  protected:
    virtual std::string GetText(const std::string& path);
    virtual std::shared_ptr<sf::Texture> GetTexture(const std::string& path);
    virtual std::shared_ptr<sf::SoundBuffer> GetAudio(const std::string& path);

  public:

    SceneBase() = delete;
    SceneBase(const SceneBase&) = delete;

    /**
     * @brief Loads the player's library data and loads graphics
     */
    SceneBase(swoosh::ActivityController&, bool guestAccount);

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
     * @brief Stops music, plays new track, reset the camera
     */
    virtual void onStart() override;

    /**
     * @brief Does nothing
     */
    virtual void onLeave() override;

    /**
     * @brief Does nothing
     */
    virtual void onExit() override;

    /**
     * @brief Checks the selected navi if changed and loads the new images
     */
    virtual void onEnter() override;

    /**
     * @brief Resets the camera
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

    /**
    * @brief Effectively sets the scene
    */
    void LoadMap(const std::string& data);

    void TeleportUponReturn(const sf::Vector2f& position);
    const bool HasTeleportedAway() const;

    void SetBackground(Background*);

    /**
     * @brief Add a sprite
     * @param sprite
     */
    void AddSprite(const std::shared_ptr<WorldSprite>& sprite);

    /**
     * @brief Remove a sprite
     * @param sprite
     */
    void RemoveSprite(const std::shared_ptr<WorldSprite>& sprite);

    /**
     * @brief Adds Actor for updates and rendering. (Calls AddSprite)
     * @param actor
     */
    void AddActor(const std::shared_ptr<Actor>& actor);


    /**
     * @brief Removes the actor for updates and rendering (Calls RemoveSprite)
     * @param actor
     */
    void RemoveActor(const std::shared_ptr<Actor>& actor);


    /**
     * @brief Block the player from moving and interacting
     * @param actor
     */
    void LockInput();


    /**
     * @brief Unblock the player from moving and interacting. Unlock is not guaranteed as the player may be in a menu
     * @param actor
     */
    void UnlockInput();


    /**
     * @brief Stops the camera from following the player
     * @param actor
     */
    void LockCamera();


    /**
     * @brief Locks the camera and queues PlaceCamera
     * @param position
     * @param holdTime
     */
    void QueuePlaceCamera(sf::Vector2f position, sf::Time holdTime = sf::Time::Zero);


    /**
     * @brief Locks the camera and queues MoveCamera
     * @param position
     * @param duration
     */
    void QueueMoveCamera(sf::Vector2f position, sf::Time duration);

    /**
     * @brief Unlocks the camera and clears the queue after completing previous camera events
     */
    void QueueUnlockCamera();

    /**
     * @brief Clears the camera queue and follow the player again
     */
    void UnlockCamera();

    //
    // Menu selection callbacks
    //

    void GotoChipFolder();
    void GotoNaviSelect();
    void GotoConfig();
    void GotoMobSelect();
    void GotoPVP();

    //
    // Getters
    //

    SpatialMap& GetSpatialMap();
    std::vector<std::shared_ptr<Actor>>& GetActors();
    Camera& GetCamera();
    Map& GetMap();
    std::shared_ptr<Actor> GetPlayer();
    PlayerController& GetPlayerController();
    TeleportController& GetTeleportController();
    SelectedNavi& GetCurrentNavi();
    Background* GetBackground();
    Overworld::TextBox& GetTextBox();
    bool IsInputLocked();
    bool IsCameraLocked();

    //
    // Helpers
    //
    std::pair<unsigned, unsigned> PixelToRowCol(const sf::Vector2i& px, const sf::RenderWindow& window) const;

    //
    // Optional events that can be decorated further
    //
    virtual void OnEmoteSelected(Emotes emote);

    //
    // Required implementations
    //
    virtual void OnTileCollision() = 0;
    virtual void OnInteract() = 0;
  };
}