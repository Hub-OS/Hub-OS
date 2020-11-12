#include "bnPVPBattleScene.h"

using swoosh::ActivityController;

PVPBattleScene::PVPBattleScene(ActivityController& controller, const PVPBattleProperties& props) : BattleSceneBase(controller, props.base) 
{}

PVPBattleScene::~PVPBattleScene() 
{}

void PVPBattleScene::OnHit(Character & victim, const Hit::Properties & props)
{
}
