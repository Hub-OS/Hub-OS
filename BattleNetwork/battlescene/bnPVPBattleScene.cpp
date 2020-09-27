#include "bnPVPBattleScene.h"

PVPBattleScene::PVPBattleScene(const PVPBattleProperties& props)
: IBattleScene(props.controller, props.player) 
{}

PVPBattleScene::~PVPBattleScene() 
{}
