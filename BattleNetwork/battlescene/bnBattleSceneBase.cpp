#include "bnBattleSceneBase.h"

const int BattleSceneBase::GetCounterCount() const {

}

void BattleSceneBase::HandleCounterLoss(Character& subject) {

}

void BattleSceneBase::ProcessNewestComponents() {

}

const bool BattleSceneBase::IsCleared() {

}

const bool BattleSceneBase::IsBattleActive() {

}

void BattleSceneBase::FilterSupportCards(Battle::Card** cards, int cardCount) {

}

#ifdef __ANDROID__
void BattleSceneBase::SetupTouchControls() {

}

void BattleSceneBase::ShutdownTouchControls() {

}
#endif

BattleSceneBase::BattleSceneBase(swoosh::ActivityController* controller, Player* localPlayer) : swoosh::Activity(controller) {};

BattleSceneBase::~BattleSceneBase() {};

void BattleSceneBase::StartStateGraph(StateNode& start) {
  this->current = &start.state;
  this->current->onStart();
}

void BattleSceneBase::onUpdate(double elapsed) {
  if(!current) return;
  current->onUpdate(elapsed);

  auto options = nodeToEdges.equal_range(current);

  for(auto iter = options.first; iter != options.second; iter++) {
    if(iter->second.when()) {
      current->onEnd();
      current = &iter->second.b.get().state;
      current->onStart();
    }
  }
}

void BattleSceneBase::onDraw(sf::RenderTexture& surface) {
  if (current) current->onDraw(surface);
}

void BattleSceneBase::Quit(const FadeOut& mode) {
  if(quitting) return; 

  // end the current state
  if(current) {
    current->onEnd();
    delete current;
    current = nullptr;
  }

  // Delete all state transitions
  nodeToEdges.clear();

  for(auto&& state : states) {
    delete state;
  }

  states.clear();

  // Depending on the mode, use Swoosh's 
  // activity controller to fadeout with the right
  // visual appearance
  if(mode == FadeOut::white) {
    getController().queuePop<WhiteWashFade>();
  } else {
    getController().queuePop<BlackWashFade>();
  }

  quitting = true;
}

void BattleSceneBase::Inject(CardUsePublisher& pub) {
}

void BattleSceneBase::Inject(MobHealthUI& other) {

}

void BattleSceneBase::Inject(Component* other) {

}

void BattleSceneBase::Eject(Component::ID_t ID) {

}

void BattleSceneBase::Link(StateNode& a, StateNode& b, ChangeCondition&& when) {
  Edge edge{a, b, std::move(when)};
  nodeToEdges.insert(std::make_pair(&a.state, edge));
  nodeToEdges.insert(std::make_pair(&b.state, edge));
}