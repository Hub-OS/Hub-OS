#pragma once

enum class ScriptMetaType : int {
  error = 0,  // Flag that script meta is malformed and should not be added to the environment
  chipTable,  // Script contains card data as a lua table
  chipAction, // Script is a brand new card action
  artifact,   // Script defines a new artifact object
  character,  // Script defines a new character object
  obstacle,   // Script defines a new obstacle object
  spell,      // Script defines a new spell object
  player,     // Script defines a new playable character
  form        // Script defines a new form
};