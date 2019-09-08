#include "bnTextureResourceManager.h"

#include <stdlib.h>
#include <atomic>
#include <sstream>
#include <fstream>
using std::ifstream;
using std::stringstream;

TextureResourceManager& TextureResourceManager::GetInstance() {
  static TextureResourceManager instance;
  return instance;
}

void TextureResourceManager::LoadAllTextures(std::atomic<int> &status) {
  TextureType textureType = static_cast<TextureType>(0);
  while (textureType != TEXTURE_TYPE_SIZE) {
    status++;

    // TODO: Catch failed resources and try again
    Texture* texture = nullptr;
    texture = LoadTextureFromFile(paths[static_cast<int>(textureType)]);
    if (texture) textures.insert(pair<TextureType, Texture*>(textureType, texture));
    textureType = (TextureType)(static_cast<int>(textureType) + 1);
  }
}

Texture* TextureResourceManager::LoadTextureFromFile(string _path) {
  Texture* texture = new Texture();
  if (!texture->loadFromFile(_path)) {

    Logger::GetMutex()->lock();
    Logger::Logf("Failed loading texture: %s", _path.c_str());
    Logger::GetMutex()->unlock();

  } else {

    Logger::GetMutex()->lock();
    Logger::Logf("Loaded texture: %s", _path.c_str());
    Logger::GetMutex()->unlock();

  }
  return texture;
}

Texture* TextureResourceManager::GetTexture(TextureType _ttype) {
  return textures.at(_ttype);
}

sf::IntRect TextureResourceManager::GetCardRectFromID(unsigned ID) {
  return sf::IntRect((ID % 11) * 56, (ID / 11) * 48, 56, 48);
}

sf::IntRect TextureResourceManager::GetIconRectFromID(unsigned ID) {
  sf::IntRect result  = sf::IntRect(1+((ID % 20) * 16), 1+((ID / 20) * 16), 14, 14);
  return result;
}

Font* TextureResourceManager::LoadFontFromFile(string _path) {
  Font* font = new Font();
  if (!font->loadFromFile(_path)) {
    Logger::Logf("Failed loading font: %s", _path.c_str());
  } else {
    Logger::Logf("Loaded font: %s", _path.c_str());
  }
  return font;
}

TextureResourceManager::TextureResourceManager(void) {
  //-Tiles-
  //Blue tile
  paths.push_back("resources/tiles/tile_atlas_blue.png");

  //Red tile
  paths.push_back("resources/tiles/tile_atlas_red.png");

  //-Navis-
  // Megaman
  paths.push_back("resources/navis/megaman/navi_megaman_atlas.png");

  // Starman
  paths.push_back("resources/navis/starman/navi_starman_atlas.png");

  // Roll
  paths.push_back("resources/navis/roll/navi_roll_atlas.png");

  // Mobs
  paths.push_back("resources/mobs/mob_move.png");
  paths.push_back("resources/mobs/mob_explosion.png");
  paths.push_back("resources/mobs/boss_shine.png");

  // Mettaur
  paths.push_back("resources/mobs/mettaur/mettaur.png");

  // Metrid
  paths.push_back("resources/mobs/metrid/metrid.png");

  // ProgsMan
  paths.push_back("resources/mobs/progsman/mob_progsman_atlas.png");

  // canodumbs
  paths.push_back("resources/mobs/canodumb/canodumb_atlas.png");

  // MetalMan
  paths.push_back("resources/mobs/metalman/metalman_atlas.png");

  // Alpha
  paths.push_back("resources/mobs/alpha/alpha.png");

  // Starfish
  paths.push_back("resources/mobs/starfish/starfish_atlas.png");

  // Megalian
  paths.push_back("resources/mobs/megalian/megalian_atlas.png");

  // Select screen "Anything Goes"
  paths.push_back("resources/mobs/select/random.png");

  // Battle misc / Artifacts
  paths.push_back("resources/mobs/mystery_data/mystery_data.png");
  paths.push_back("resources/mobs/cube/cube.png");

  //-Spells-
  paths.push_back("resources/spells/spell_bullet_hit.png");
  paths.push_back("resources/spells/spell_buster_charge.png");
  paths.push_back("resources/spells/spell_charged_bullet_hit.png");
  paths.push_back("resources/spells/guard_hit.png");
  paths.push_back("resources/spells/spell_wave.png");
  paths.push_back("resources/spells/spell_prog_bomb.png");
  paths.push_back("resources/spells/auras.png");
  paths.push_back("resources/spells/thunder.png");
  paths.push_back("resources/spells/reflect_shield.png");
  paths.push_back("resources/spells/bubble.png");
  paths.push_back("resources/spells/bubble_trap.png");
  paths.push_back("resources/spells/elecpulse.png");
  paths.push_back("resources/spells/ninja_star.png");
  paths.push_back("resources/spells/poof.png");
  paths.push_back("resources/spells/areagrab.png");
  paths.push_back("resources/spells/sword_effect.png");
  paths.push_back("resources/spells/spell_meteor.png");
  paths.push_back("resources/spells/spell_ring_explosion.png");

  // Chip Select 
  paths.push_back("resources/ui/chip_select.png");
  paths.push_back("resources/ui/chip_cursor_small.png");
  paths.push_back("resources/ui/chip_cursor_big.png");
  paths.push_back("resources/ui/chip_cards.png");
  paths.push_back("resources/ui/chip_icons.png");
  paths.push_back("resources/ui/chip_nosupport.png");
  paths.push_back("resources/ui/chip_nodata.png");
  paths.push_back("resources/ui/chip_senddata.png");
  paths.push_back("resources/ui/chip_missing.png");
  paths.push_back("resources/ui/chip_lock.png");
  paths.push_back("resources/ui/chip_frame.png");
  paths.push_back("resources/ui/cust_badge.png");
  paths.push_back("resources/ui/cust_badge_mask.png");

  // In battle signs
  paths.push_back("resources/ui/program_advance.png");
  paths.push_back("resources/ui/battle_start.png");
  paths.push_back("resources/ui/enemy_deleted.png");
  paths.push_back("resources/ui/double_delete.png");
  paths.push_back("resources/ui/triple_delete.png");
  paths.push_back("resources/ui/counter_hit.png");
  paths.push_back("resources/ui/alert.png");

  // Battle Results Frame
  paths.push_back("resources/ui/battle_results.png");
  paths.push_back("resources/ui/zenny.png");
  paths.push_back("resources/ui/nodata.png");
  paths.push_back("resources/ui/press_a.png");
  paths.push_back("resources/ui/counter_star.png");

  // folder view
  paths.push_back("resources/ui/folder_chip.png");
  paths.push_back("resources/ui/folder_dock.png");
  paths.push_back("resources/ui/pack_dock.png");
  paths.push_back("resources/ui/folder_rarity.png");
  paths.push_back("resources/ui/scrollbar.png");
  paths.push_back("resources/ui/select_cursor.png");
  paths.push_back("resources/ui/folder_equip.png");
  paths.push_back("resources/ui/folder_name.png");
  paths.push_back("resources/ui/folder_cursor.png");
  paths.push_back("resources/ui/folder_options.png");
  paths.push_back("resources/ui/folder_size.png");
  paths.push_back("resources/ui/folder_textbox.png");
  paths.push_back("resources/ui/mb_placeholder.png");
  paths.push_back("resources/ui/folder_edit_next.png");
  paths.push_back("resources/ui/folder_sort.png");
  paths.push_back("resources/ui/letter_cursor.png");

  // Navi Select View
  paths.push_back("resources/backgrounds/select/glow_sheet.png");
  paths.push_back("resources/backgrounds/select/pad_base.png");
  paths.push_back("resources/backgrounds/select/pad_bottom.png");
  paths.push_back("resources/backgrounds/select/char_name.png");
  paths.push_back("resources/backgrounds/select/stat.png");
  paths.push_back("resources/backgrounds/select/element.png");
  paths.push_back("resources/backgrounds/select/info_box.png");
  paths.push_back("resources/backgrounds/select/symbol_slots.png");

  // Mugshots
  paths.push_back("resources/ui/navigator.png");
  paths.push_back("resources/ui/prog.png");
  paths.push_back("resources/ui/textbox.png");
  paths.push_back("resources/ui/textbox_next.png");
  paths.push_back("resources/ui/textbox_cursor.png");

  // Background/foreground
  paths.push_back("resources/backgrounds/title/bg_blue.png");
  paths.push_back("resources/backgrounds/title/prog-pulse.png");
  paths.push_back("resources/backgrounds/game_over/game_over.png");
  paths.push_back("resources/backgrounds/select/battle_select.png");
  paths.push_back("resources/backgrounds/main_menu/overlay.png");
  paths.push_back("resources/backgrounds/main_menu/ow.png");
  paths.push_back("resources/backgrounds/main_menu/ow2.png");
  paths.push_back("resources/backgrounds/main_menu/arrow.png");
  paths.push_back("resources/backgrounds/folder/bg.png");
  paths.push_back("resources/backgrounds/folder/folder_info.png");
  paths.push_back("resources/backgrounds/folder/folder_name.png");
  paths.push_back("resources/backgrounds/select/bg.png");

  // Overworld
  paths.push_back("resources/backgrounds/main_menu/mr_prog_ow.png");
  paths.push_back("resources/backgrounds/main_menu/numberman_ow.png");

  // other ui / icons
  paths.push_back("resources/ui/aura_numset.png");
  paths.push_back("resources/ui/hp_numset.png");
  paths.push_back("resources/ui/player_numset.png");
  paths.push_back("resources/ui/gamepad_support_icon.png");
  paths.push_back("resources/ui/main_menu_ui.png");
  paths.push_back("resources/ui/elements.png");

  // texture maps
  paths.push_back("resources/shaders/textures/distortion.png");
  paths.push_back("resources/shaders/textures/heat.png");
  paths.push_back("resources/shaders/textures/noise.png");
  paths.push_back("resources/shaders/textures/noise_invert.png");

  // editor ui
  paths.push_back("resources/ui/light.png");

  // font
  paths.push_back("resources/fonts/all_fonts.png");

  // config ui
  paths.push_back("resources/backgrounds/config/audio.png");
  paths.push_back("resources/backgrounds/config/end_btn.png");
}

TextureResourceManager::~TextureResourceManager(void) {
  for (auto it = textures.begin(); it != textures.end(); ++it) {
    delete it->second;
  }
}
