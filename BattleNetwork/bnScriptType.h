#pragma once

enum class ScriptType : int {
  CHIP_TABLE_SCRIPT,  // Script contains chip data as a lua table
  CHIP_ACTION_SCRIPT, // Script is a brand new chip action
  ARTIFACT_SCRIPT,    // Script defines a new artifact object
  CHARACTER_SCRIPT,   // Script defines a new character object
  OBSTACLE_SCRIPT,    // Script defines a new obstacle object
  SPELL_SCRIPT,       // Script defines a new spell object
  PLAYER_SCRIPT,      // Script defines a new playable character
  FORM_SCRIPT         // Script defines a new form
};