/// \brief Thunk to populate menu options to callbacks
// namespace {
//   auto MakeOptions = [](Overworld::SceneBase* scene) -> Overworld::PersonalMenu::OptionsList {
//     return {
//       { "chip_folder", std::bind(&GotoChipFolder, scene) },
//       { "navi",        std::bind(&GotoNaviSelect, scene) },
//       { "mail",        std::bind(&GotoMail, scene) },
//       { "key_items",   std::bind(&GotoKeyItems, scene) },
//       { "mob_select",  std::bind(&GotoMobSelect, scene) },
//       { "config",      std::bind(&GotoConfig, scene) },
//       { "sync",        std::bind(&GotoPVP, scene) }
//     };
//   };
// }

// SceneBase(swoosh::ActivityController& controller) :
//   personalMenu(std::make_shared<PersonalMenu>(playerSession, "Overworld", ::MakeOptions(this))),
//   minimap(std::make_shared<Minimap>()),
//   camera(controller.getWindow().getView()),
//   Scene(controller),
//   map(0, 0, 0, 0),
//   playerActor(std::make_shared<Overworld::Actor>("You"))
// {

//   // Draws the scrolling background
//   SetBackground(std::make_shared<LanBackground>());

//   // set the missing texture for all actor objects
//   auto missingTexture = Textures().LoadFromFile("resources/ow/missing.png");
//   Overworld::Actor::SetMissingTexture(missingTexture);

//   personalMenu->setScale(2.f, 2.f);

//   auto& session = getController().Session();
//   bool loaded = session.LoadSession("profile.bin");

//   // folders may be blank if session was unable to load a collection
//   folders = &session.GetCardFolderCollection();

//   if (loaded) {
//     NaviEquipSelectedFolder();
//   }

//   // Spawn overworld player
//   playerActor->SetCollisionRadius(4);

//   AddActor(playerActor);
//   AddSprite(teleportController.GetBeam());

//   worldTransform.setScale(2.f, 2.f);

//   menuSystem.BindMenu(InputEvents::pressed_pause, personalMenu);
//   menuSystem.BindMenu(InputEvents::pressed_map, minimap);
//   minimap->setScale(2.f, 2.f);

//   cameraPanUITexture = Textures().LoadFromFile(TexturePaths::CAMERA_PAN_UI);
//   cameraPanUIAnimation = Animation(AnimationPaths::CAMERA_PAN_UI) << "DEFAULT" << Animator::Mode::Loop;
//   cameraPanUI.setTexture(*cameraPanUITexture, false);
//   cameraPanUI.setScale(2.f, 2.f);
//   cameraPanUIAnimation.Refresh(cameraPanUI);
// }

// void onUpdate(double elapsed) {
//   playerController.ListenToInputEvents(!IsInputLocked());

//   if (IsInFocus()) {
//     HandleInput();
//   }

//   // handle custom tile collision
//   if (!HasTeleportedAway() && teleportController.IsComplete()) {
//     this->OnTileCollision();
//   }

//   /**
//   * update all overworld objects and animations
//   */

//   // animations
//   animElapsed += elapsed;

//   // expecting glitches, manually update when actors move?
//   spatialMap.Update();

//   // update tile animations
//   map.Update(*this, animElapsed);

//   if (IsInFocus()) {
//     playerController.Update(elapsed);
//     teleportController.Update(elapsed);

//     // factor in player's position for the minimap
//     sf::Vector2f screenPos = map.WorldToScreen(playerActor->getPosition());
//     screenPos.y -= playerActor->GetElevation() * map.GetTileSize().y * 0.5f;
//     screenPos.x = std::floor(screenPos.x);
//     screenPos.y = std::floor(screenPos.y);

//     // update minimap
//     auto playerTilePos = map.WorldToTileSpace(playerActor->getPosition());
//     bool isConcealed = map.IsConcealed(sf::Vector2i(playerTilePos), playerActor->GetLayer());
//     minimap->SetPlayerPosition(screenPos, isConcealed);
//   }

//   for (auto& actor : actors) {
//     actor->Update((float)elapsed, map, spatialMap);
//   }

//   auto layerCount = map.GetLayerCount() + 1;

//   if (spriteLayers.size() != layerCount) {
//     spriteLayers.resize(layerCount);
//   }

//   for (auto i = 0; i < layerCount; i++) {
//     auto& spriteLayer = spriteLayers[i];
//     auto elevation = (float)i;
//     bool isTopLayer = elevation + 1 == layerCount;
//     bool isBottomLayer = elevation == 0;

//     spriteLayer.clear();

//     // match sprites to layer
//     for (auto& sprite : sprites) {
//       // use ceil(elevation) + 1 instead of GetLayer to prevent sorting issues with stairs
//       auto spriteElevation = std::ceil(sprite->GetElevation()) + 1;
//       if (spriteElevation == elevation || (isTopLayer && spriteElevation >= layerCount) || (isBottomLayer && spriteElevation < 0)) {
//         spriteLayer.push_back(sprite);
//       }
//     }

//     // sort sprites within the layer
//     std::sort(spriteLayer.begin(), spriteLayer.end(),
//       [](const std::shared_ptr<WorldSprite>& A, const std::shared_ptr<WorldSprite>& B) {
//       auto& A_pos = A->getPosition();
//       auto& B_pos = B->getPosition();
//       auto A_compare = A_pos.x + A_pos.y;
//       auto B_compare = B_pos.x + B_pos.y;

//       return A_compare < B_compare;
//     });
//   }

//   // grabbing the camera pos for parallax
//   const sf::Vector2f& cameraPos = camera.GetView().getCenter();

//   // Update bg
//   if (bg) {
//     bg->Update((float)elapsed);

//     // Apply parallax
//     bg->SetOffset(bg->GetOffset() + Floor(cameraPos * backgroundParallaxFactor));
//   }

//   // Update the textbox
//   menuSystem.Update((float)elapsed);

//   // camera pan ui animates
//   cameraPanUIAnimation.Update(elapsed, cameraPanUI);

//   HandleCamera((float)elapsed);

//   // Update foreground
//   if (fg) {
//     fg->Update((float)elapsed);

//     // Apply parallax
//     fg->SetOffset(fg->GetOffset() + Floor(cameraPos * foregroundParallaxFactor));
//   }
// }

// void HandleCamera(float elapsed) {
//   if (!cameraLocked) {
//     // Follow the navi
//     sf::Vector2f pos = playerActor->getPosition();

//     if (teleportedOut) {
//       pos = { returnPoint.x, returnPoint.y };
//     }

//     pos = map.WorldToScreen(pos);
//     pos.y -= playerActor->GetElevation() * map.GetTileSize().y * 0.5f;

//     teleportedOut ? camera.MoveCamera(pos, sf::seconds(0.5)) : camera.PlaceCamera(pos);
//     camera.Update(elapsed);
//     return;
//   }

//   camera.Update(elapsed);
// }

// void HandleInput() {
//   auto& window = getController().getWindow();
//   auto& input = Input();
//   auto mousei = sf::Mouse::getPosition(window);
//   auto mousef = window.mapPixelToCoords(mousei);

//   if (cameraControlsEnabled) {
//     LockCamera();

//     sf::Vector2f panningOffset;
//     float fast = 1.f;

//     if (input.Has(InputEvents::held_cancel)) {
//       fast = 3.f;
//     }

//     if (input.Has(InputEvents::held_ui_left)) {
//       panningOffset.x -= 2.f;
//     }

//     if (input.Has(InputEvents::held_ui_right)) {
//       panningOffset.x += 2.f;
//     }

//     if (input.Has(InputEvents::held_ui_up)) {
//       panningOffset.y -= 2.f;
//     }

//     if (input.Has(InputEvents::held_ui_down)) {
//       panningOffset.y += 2.f;
//     }

//     if (input.GetConfigSettings().GetInvertMinimap()) {
//       panningOffset.x *= -2.f;
//       panningOffset.y *= -2.f;
//     }

//     if (input.Has(InputEvents::pressed_shoulder_left)) {
//       cameraControlsEnabled = false;
//       UnlockInput();
//       UnlockCamera();
//     }

//     sf::Vector2f newPos = camera.GetView().getCenter() + (panningOffset * fast);
//     newPos.x = std::clamp(newPos.x, cameraControlsStart.x - cameraControlsRange.x, cameraControlsStart.x + cameraControlsRange.x);
//     newPos.y = std::clamp(newPos.y, cameraControlsStart.y - cameraControlsRange.y, cameraControlsStart.y + cameraControlsRange.y);
//     camera.PlaceCamera(newPos);
//     return;
//   }

//   auto menuSystemOpen = !menuSystem.IsClosed();

//   menuSystem.HandleInput(input, mousef);

//   if (menuSystemOpen) {
//     return;
//   }

//   if (cameraZoomEnabled) {
//     if (sf::Keyboard::isKeyPressed(sf::Keyboard::Home)) {
//       worldTransform.setScale(2.f, 2.f);
//     }
//     else if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp) && !cameraZoomKeyPressed) {
//       worldTransform.setScale(worldTransform.getScale() * 2.f);
//       cameraZoomKeyPressed = true;
//     }
//     else if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown) && !cameraZoomKeyPressed) {
//       worldTransform.setScale(worldTransform.getScale() * 0.5f);
//       cameraZoomKeyPressed = true;
//     }
//   }

//   if (cameraZoomKeyPressed
//     && !sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp)
//     && !sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown)) {
//     cameraZoomKeyPressed = false;
//   }

//   // check to see if talk button was pressed
//   if (!IsInputLocked()) {
//     if (input.Has(InputEvents::pressed_interact)) {
//       OnInteract(Interaction::action);
//     }

//     if (input.Has(InputEvents::pressed_shoulder_left)) {
//       OnInteract(Interaction::inspect);
//     }
//   }
// }

// void onEnter()
// {
//   RefreshNaviSprite();

//   if (!this->map.GetSongPath().empty()) {
//     Audio().Stream(GetPath(map.GetSongPath()), true);
//   }
// }

// void onResume() {
//   getController().Session().SaveSession("profile.bin");

//   // if we left this scene for a new OW scene... return to our warp area
//   if (teleportedOut) {
//     playerController.ReleaseActor();

//     auto& command = teleportController.TeleportIn(
//       playerActor,
//       returnPoint,
//       Reverse(playerActor->GetHeading())
//     );

//     teleportedOut = false;
//     command.onFinish.Slot([=] {
//       playerController.ControlActor(playerActor);
//     });
//   }

//   if (!map.GetSongPath().empty()) {
//     Audio().Stream(GetPath(map.GetSongPath()), true);
//   }
// }

// void onDraw(sf::RenderTexture& surface) {
//   if (menuSystem.IsFullscreen()) {
//     surface.draw(menuSystem);
//     return;
//   }

//   if (bg) {
//     surface.draw(*bg);
//   }

//   DrawWorld(surface, sf::RenderStates::Default);

//   if (fg) {
//     surface.draw(*fg);
//   }

//   if (personalMenu->IsClosed() && !cameraControlsEnabled) {
//     // menuSystem will not draw personal menu if it's closed
//     // might make sense to extract some parts of menu system as the closed UI has different requirements
//     personalMenu->draw(surface, sf::RenderStates::Default);
//   }

//   // This will mask everything before this line with camera fx
//   surface.draw(camera.GetLens());

//   // camera pan ui on top
//   if (cameraControlsEnabled) {
//     surface.draw(cameraPanUI);
//   }

//   // always see menus
//   surface.draw(menuSystem);
// }

// void DrawWorld(sf::RenderTarget& target, sf::RenderStates states) {
//   const sf::Vector2f& mapScale = worldTransform.getScale();
//   sf::Vector2f cameraCenter = camera.GetView().getCenter();
//   cameraCenter.x = std::floor(cameraCenter.x) * mapScale.x;
//   cameraCenter.y = std::floor(cameraCenter.y) * mapScale.y;

//   sf::Vector2f offset = getView().getCenter() - cameraCenter;

//   // prevents stitching artifacts between tiles
//   offset.x = std::floor(offset.x);
//   offset.y = std::floor(offset.y);

//   states.transform.translate(offset);
//   states.transform *= worldTransform.getTransform();

//   sf::Vector2i tileSize = map.GetTileSize();
//   size_t mapLayerCount = map.GetLayerCount();

//   // there should be mapLayerCount + 1 sprite layers
//   for (size_t i = 0; i < mapLayerCount + 1; i++) {

//     // loop is based on expected sprite layers
//     // make sure we dont try to draw an extra map layer and segfault
//     if (i < mapLayerCount) {
//       DrawMapLayer(target, states, i, mapLayerCount);
//     }

//     // save from possible map layer count change after OverworldSceneBase::Update
//     if (i < spriteLayers.size()) {
//       DrawSpriteLayer(target, states, i);
//     }

//     // translate next layer
//     states.transform.translate(0.f, -tileSize.y * 0.5f);
//   }
// }

// void DrawMapLayer(sf::RenderTarget& target, sf::RenderStates states, size_t index, size_t maxLayers) {
//   Map::Layer& layer = map.GetLayer(index);

//   if (!layer.IsVisible()) {
//     return;
//   }

//   unsigned int rows = map.GetRows();
//   unsigned int cols = map.GetCols();
//   sf::Vector2i tileSize = map.GetTileSize();

//   const int TILE_PADDING = 3;
//   sf::Vector2f screenSize = sf::Vector2f(target.getSize()) / worldTransform.getScale().x;

//   int verticalTileCount = (int)std::ceil((screenSize.y / (float)tileSize.y) * 2.0f);
//   int horizontalTileCount = (int)std::ceil(screenSize.x / (float)tileSize.x);

//   sf::Vector2f screenTopLeft = camera.GetView().getCenter() - screenSize * 0.5f;
//   sf::Vector2i tileSpaceStart = sf::Vector2i(map.WorldToTileSpace(map.ScreenToWorld(screenTopLeft)));
//   int verticalOffset = (int)index;

//   for (int i = -TILE_PADDING; i < verticalTileCount + TILE_PADDING; i++) {
//     int verticalStart = (verticalOffset + i) / 2;
//     int rowOffset = (verticalOffset + i) % 2;

//     for (int j = -TILE_PADDING; j < horizontalTileCount + rowOffset + TILE_PADDING; j++) {
//       int row = tileSpaceStart.y + verticalStart - j + rowOffset;
//       int col = tileSpaceStart.x + verticalStart + j;

//       auto tile = layer.GetTile(col, row);
//       if (!tile || tile->gid == 0) continue;

//       TileMetaPtr tileMeta = map.GetTileMeta(tile->gid);

//       // failed to load tile
//       if (tileMeta == nullptr) continue;

//       // invisible tile
//       if (tileMeta->type == TileType::invisible) continue;

//       sf::Sprite& tileSprite = tileMeta->sprite;
//       sf::FloatRect spriteBounds = tileSprite.getLocalBounds();

//       const sf::Vector2f& originalOrigin = tileSprite.getOrigin();
//       tileSprite.setOrigin(sf::Vector2f(sf::Vector2i(
//         (int)spriteBounds.width / 2,
//         tileSize.y / 2
//       )));

//       sf::Vector2i pos((col * tileSize.x) / 2, row * tileSize.y);
//       sf::Vector2f ortho = map.WorldToScreen(sf::Vector2f(pos));
//       sf::Vector2f tileOffset = sf::Vector2f(sf::Vector2i(
//         -tileSize.x / 2 + (int)spriteBounds.width / 2,
//         tileSize.y + tileSize.y / 2 - (int)spriteBounds.height
//       ));

//       tileSprite.setPosition(ortho + tileMeta->drawingOffset + tileOffset);
//       tileSprite.setRotation(tile->rotated ? 90.0f : 0.0f);
//       tileSprite.setScale(
//         tile->flippedHorizontal ? -1.0f : 1.0f,
//         tile->flippedVertical ? -1.0f : 1.0f
//       );

//       sf::Color originalColor = tileSprite.getColor();
//       if (map.HasShadow({ col, row }, (int)index)) {
//         sf::Uint8 r, g, b;
//         r = sf::Uint8(originalColor.r * 0.65);
//         b = sf::Uint8(originalColor.b * 0.65);
//         g = sf::Uint8(originalColor.g * 0.65);

//         tileSprite.setColor(sf::Color(r, g, b, originalColor.a));
//       }
//       target.draw(tileSprite, states);
//       tileSprite.setColor(originalColor);

//       tileSprite.setOrigin(originalOrigin);
//     }
//   }
// }


// void DrawSpriteLayer(sf::RenderTarget& target, sf::RenderStates states, size_t index) {
//   unsigned int rows = map.GetRows();
//   unsigned int cols = map.GetCols();
//   sf::Vector2i tileSize = map.GetTileSize();
//   size_t mapLayerCount = map.GetLayerCount();
//   float layerElevation = (float)index;

//   for (WorldSpritePtr& sprite : spriteLayers[index]) {
//     const sf::Vector2f worldPos = sprite->getPosition();
//     sf::Vector2f screenPos = map.WorldToScreen(worldPos);
//     screenPos.y -= (sprite->GetElevation() - layerElevation) * tileSize.y * 0.5f;

//     // prevents blurring and camera jittering with the player
//     screenPos.x = std::floor(screenPos.x);
//     screenPos.y = std::floor(screenPos.y);

//     sprite->setPosition(screenPos);

//     sf::Vector2i gridPos = sf::Vector2i(map.WorldToTileSpace(worldPos));

//     if (/*cam && cam->IsInView(sprite->getSprite())*/ true) {
//       sf::Color originalColor = sprite->getColor();
//       // round to the closest layer for handling online players on stairs
//       int layer = (int)std::roundf(sprite->GetElevation());

//       if (map.HasShadow(gridPos, layer)) {
//         sf::Uint8 r, g, b;
//         r = sf::Uint8(originalColor.r * 0.5);
//         b = sf::Uint8(originalColor.b * 0.5);
//         g = sf::Uint8(originalColor.g * 0.5);
//         sprite->setColor(sf::Color(r, g, b, originalColor.a));
//       }

//       target.draw(*sprite, states);
//       sprite->setColor(originalColor);
//     }

//     sprite->setPosition(worldPos);
//   }
// }

// void RefreshNaviSprite()
// {
//   // Only refresh all data and graphics if this is a new navi
//   if (lastSelectedNaviId == currentNaviId && !lastSelectedNaviId.empty()) return;

//   PlayerPackageManager& packageManager = getController().PlayerPackagePartitioner().GetPartition(Game::LocalPartition);
//   if (!packageManager.HasPackage(currentNaviId)) {
//     currentNaviId = packageManager.FirstValidPackage();
//     getController().Session().SetKeyValue("SelectedNavi", currentNaviId);
//   }

//   lastSelectedNaviId = currentNaviId;

//   auto& meta = packageManager.FindPackageByID(currentNaviId);

//   // refresh menu widget too
//   playerSession->health = meta.GetHP();
//   playerSession->maxHealth = meta.GetHP();

//   // If coming back from navi select, the navi has changed, update it
//   const std::filesystem::path& owPath = meta.GetOverworldAnimationPath();

//   if (!owPath.empty()) {
//     if (auto tex = Textures().LoadFromFile(meta.GetOverworldTexturePath())) {
//       playerActor->setTexture(tex);
//     }
//     playerActor->LoadAnimations(owPath);

//     auto iconTexture = meta.GetIconTexture();

//     if (iconTexture) {
//       personalMenu->UseIconTexture(iconTexture);
//     }
//     else {
//       personalMenu->ResetIconTexture();
//     }
//   }
//   else {
//     Logger::Logf(LogLevel::critical, "Overworld animation not found for navi package %s", currentNaviId.c_str());
//   }
// }

// void NaviEquipSelectedFolder()
// {
//   GameSession& session = getController().Session();
//   std::string naviId = session.GetKeyValue("SelectedNavi");
//   if (!naviId.empty()) {
//     currentNaviId = naviId;
//     RefreshNaviSprite();

//     std::string folderStr = session.GetKeyValue("FolderFor:" + naviId);
//     if (!folderStr.empty()) {
//       // preserve our selected folder
//       if (int index = folders->FindFolder(folderStr); index >= 0) {
//         folders->SwapOrder(index, 0); // Select this folder again
//       }
//     }
//   }
//   else {
//     currentNaviId = getController().PlayerPackagePartitioner().GetPartition(Game::LocalPartition).FirstValidPackage();
//     session.SetKeyValue("SelectedNavi", currentNaviId);
//   }
// }

// void LoadBackground(const Map& map, const std::string& value)
// {
//   float parallax = map.GetBackgroundParallax();

//     const auto& texture = GetTexture(map.GetBackgroundCustomTexturePath());
//     const auto& animationData = GetText(map.GetBackgroundCustomAnimationPath());
//     const auto& velocity = map.GetBackgroundCustomVelocity();

//     Animation animation;
//     animation.LoadWithData(animationData);

//     SetBackground(std::make_shared<CustomBackground>(texture, animation, velocity), parallax);
// }

// void LoadForeground(const Map& map)
// {
//   auto texturePath = map.GetForegroundTexturePath();

//   if (texturePath.empty()) {
//     fg.reset();
//     return;
//   }

//   const auto& texture = GetTexture(texturePath);
//   const auto& animationData = GetText(map.GetForegroundAnimationPath());
//   const auto& velocity = map.GetForegroundVelocity();
//   auto parallaxFactor = map.GetForegroundParallax();

//   Animation animation;
//   animation.LoadWithData(animationData);

//   SetForeground(std::make_shared<CustomBackground>(texture, animation, velocity), parallaxFactor);
// }

// std::filesystem::path GetPath(const std::string& path) {
//   return std::filesystem::u8path(path);
// }

// std::string GetText(const std::string& path) {
//   return FileUtil::Read(std::filesystem::u8path(path));
// }

// std::shared_ptr<sf::Texture> GetTexture(const std::string& path) {
//   return Textures().LoadFromFile(std::filesystem::u8path(path));
// }

// std::shared_ptr<sf::SoundBuffer> GetAudio(const std::string& path) {
//   return Audio().LoadFromFile(std::filesystem::u8path(path));
// }


// void LoadMap(const std::string& data)
// {
//   auto optionalMap = LoadTiledMap(*this, data);

//   if (!optionalMap) {
//     Logger::Log(LogLevel::critical, "Failed to load map");
//     return;
//   }

//   auto map = std::move(optionalMap.value());

//   bool backgroundDiffers = map.GetBackgroundName() != this->map.GetBackgroundName() ||
//     map.GetBackgroundCustomTexturePath() != this->map.GetBackgroundCustomTexturePath() ||
//     map.GetBackgroundCustomAnimationPath() != this->map.GetBackgroundCustomAnimationPath() ||
//     map.GetBackgroundCustomVelocity() != this->map.GetBackgroundCustomVelocity();

//   if (backgroundDiffers) {
//     LoadBackground(map, map.GetBackgroundName());
//   } else {
//     backgroundParallaxFactor = map.GetBackgroundParallax();
//   }

//   bool foregroundDiffers = map.GetForegroundTexturePath() != this->map.GetForegroundTexturePath() ||
//     map.GetForegroundAnimationPath() != this->map.GetForegroundAnimationPath() ||
//     map.GetForegroundVelocity() != this->map.GetForegroundVelocity();

//   if (foregroundDiffers) {
//     LoadForeground(map);
//   } else {
//     foregroundParallaxFactor = map.GetForegroundParallax();
//   }

//   if (this->map.GetSongPath() != map.GetSongPath()) {
//     Audio().Stream(GetPath(map.GetSongPath()), true);
//   }

//   personalMenu->SetArea(map.GetName());

//   // cleanup data from the previous map
//   this->map.RemoveSprites(*this);

//   // replace the previous map
//   this->map = std::move(map);

//   // update map to trigger recalculating shadows for minimap
//   this->map.Update(*this, 0.0f);
//   minimap->Update(this->map.GetName(), this->map);
// }

// void TeleportUponReturn(const sf::Vector3f& position)
// {
//   teleportedOut = true;
//   returnPoint = position;
// }

// const bool HasTeleportedAway() const
// {
//   return teleportedOut;
// }

// void SetBackground(const std::shared_ptr<Background>& background, float parallax)
// {
//   this->bg = background;
//   backgroundParallaxFactor = parallax;
// }

// void SetForeground(const std::shared_ptr<Background>& foreground, float parallax)
// {
//   this->fg = foreground;
//   foregroundParallaxFactor = parallax;
// }

// void AddSprite(const std::shared_ptr<WorldSprite>& sprite)
// {
//   sprites.push_back(sprite);
// }

// void RemoveSprite(const std::shared_ptr<WorldSprite>& sprite) {
//   auto pos = std::find(sprites.begin(), sprites.end(), sprite);

//   if (pos != sprites.end())
//     sprites.erase(pos);
// }

// void AddActor(const std::shared_ptr<Actor>& actor) {
//   actors.push_back(actor);
//   spatialMap.AddActor(actor);
//   AddSprite(actor);
// }

// void RemoveActor(const std::shared_ptr<Actor>& actor) {
//   spatialMap.RemoveActor(actor);
//   RemoveSprite(actor);

//   auto pos = std::find(actors.begin(), actors.end(), actor);

//   if (pos != actors.end())
//     actors.erase(pos);
// }

// bool IsInputLocked() {
//   return
//     inputLocked 
//     || !IsInFocus() 
//     || menuSystem.ShouldLockInput() 
//     || !teleportController.IsComplete() 
//     || teleportController.TeleportedOut() 
//     || cameraControlsEnabled;
// }

// bool IsCameraControlsEnabled()
// {
//   return cameraControlsEnabled;
// }

// void LockInput() {
//   inputLocked = true;
// }

// void UnlockInput() {
//   inputLocked = false;
// }

// void LockCamera() {
//   cameraLocked = true;
// }

// void UnlockCamera() {
//   cameraLocked = false;
// }

// void ToggleCameraZoom(bool enabled)
// {
//   cameraZoomEnabled = enabled;

//   if (!cameraZoomEnabled) {
//     worldTransform.setScale(2.f, 2.f);
//   }
// }

// void EnableCameraControls(float distX, float distY)
// {
//   // reset camera zoom
//   worldTransform.setScale(2.f, 2.f);
//   cameraControlsEnabled = true;
//   cameraControlsRange = sf::Vector2f(std::fabs(distX), std::fabs(distY));
//   cameraControlsStart = camera.GetView().getCenter();
// }

// Overworld::PlayerController& GetPlayerController()
// {
//   return playerController;
// }

// Overworld::TeleportController& GetTeleportController()
// {
//   return teleportController;
// }

// std::string& GetCurrentNaviID()
// {
//   return currentNaviId;
// }

// std::shared_ptr<Background> GetBackground()
// {
//   return this->bg;
// }

// Overworld::MenuSystem& GetMenuSystem()
// {
//   return menuSystem;
// }

// void AddItem(const std::string& id, const std::string& name, const std::string& description)
// {
//   items.push_back({ id, name, description });
// }

// void RemoveItem(const std::string& id)
// {
//   auto iter = std::find_if(items.begin(), items.end(), [&id](auto& item) { return item.id == id; });

//   if (iter != items.end()) {
//     items.erase(iter);
//   }
// }

// std::pair<unsigned, unsigned> PixelToRowCol(const sf::Vector2i& px, const sf::RenderWindow& window) const
// {
//   sf::Vector2f ortho = window.mapPixelToCoords(px);

//   // consider the point on screen relative to the camera focus
//   auto pos = ortho - window.getView().getCenter() - camera.GetView().getCenter();

//   auto& scale = worldTransform.getScale();
//   auto iso = map.ScreenToWorld(pos);
//   iso.x /= scale.x;
//   iso.y /= scale.y;

//   auto tileSize = map.GetTileSize();

//   // divide by the tile size to get the integer grid values
//   unsigned x = static_cast<unsigned>(iso.x / tileSize.x / 2);
//   unsigned y = static_cast<unsigned>(iso.y / tileSize.y);

//   return { y, x };
// }

// const bool IsMouseHovering(const WorldSprite& src)
// {
//   sf::Vector2f mapScale = GetWorldTransform().getScale();
//   auto cameraOffset = GetCamera().GetCenterOffset(mapScale, *this);

//   auto textureRect = src.getSprite().getTextureRect();
//   auto origin = src.getSprite().getOrigin();

//   auto& map = GetMap();
//   auto tileSize = map.GetTileSize();
//   auto& scale = worldTransform.getScale();

//   auto position = src.getPosition();
//   auto screenPosition = map.WorldToScreen(position);
//   screenPosition.y -= src.GetElevation() * tileSize.y * 0.5f;

//   auto bounds = sf::FloatRect(
//     (screenPosition.x - origin.x) * scale.x,
//     (screenPosition.y - origin.y) * scale.y,
//     std::abs(textureRect.width) * scale.x,
//     std::abs(textureRect.height) * scale.y
//   );

//   return Input().IsMouseHovering(getController().getWindow(), bounds, cameraOffset);
// }

