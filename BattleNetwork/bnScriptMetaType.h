#pragma once

enum class ScriptMetaType : int {
  ERROR_STATE = 0,    // Flag that script meta is malformed and should not be added to the environment
  CHIP_TABLE_SCRIPT,  // Script contains card data as a lua table
  CHIP_ACTION_SCRIPT, // Script is a brand new card action
  ARTIFACT_SCRIPT,    // Script defines a new artifact object
  CHARACTER_SCRIPT,   // Script defines a new character object
  OBSTACLE_SCRIPT,    // Script defines a new obstacle object
  SPELL_SCRIPT,       // Script defines a new spell object
  PLAYER_SCRIPT,      // Script defines a new playable character
  FORM_SCRIPT         // Script defines a new form
};