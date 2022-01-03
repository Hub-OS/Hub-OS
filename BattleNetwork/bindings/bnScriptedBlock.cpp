#include "bnScriptedBlock.h"

ScriptedBlock::ScriptedBlock(sol::state& script):
  script(script)
{
}

ScriptedBlock::~ScriptedBlock()
{
}

void ScriptedBlock::run(Player* player)
{
  if (run_func) {
    run_func(player);
  }
}
