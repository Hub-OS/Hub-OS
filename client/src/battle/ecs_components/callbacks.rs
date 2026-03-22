use crate::battle::BattleCallback;
use crate::bindable::{EntityId, GenerationalIndex};
use crate::structures::SlotMap;

#[derive(Clone)]
pub struct SpawnCallback(pub BattleCallback);

#[derive(Clone)]
pub struct IntroCallback(pub BattleCallback<(), Option<GenerationalIndex>>);

#[derive(Clone)]
pub struct UpdateCallback(pub BattleCallback);

#[derive(Clone)]
pub struct DeleteCallback(pub BattleCallback);

#[derive(Default, Clone)]
pub struct DeleteCallbacks(pub SlotMap<BattleCallback>);

#[derive(Default, Clone)]
pub struct EraseCallbacks(pub SlotMap<BattleCallback>);

#[derive(Clone)]
pub struct CanMoveToCallback(pub BattleCallback<(i32, i32), bool>);

#[derive(Clone)]
pub struct IdleCallback(pub BattleCallback);

#[derive(Clone)]
pub struct CollisionCallback(pub BattleCallback<EntityId>);

#[derive(Clone)]
pub struct AttackCallback(pub BattleCallback<EntityId>);

#[derive(Clone)]
pub struct CounterCallback(pub BattleCallback<EntityId>);

#[derive(Clone)]
pub struct CounteredCallback(pub BattleCallback);

#[derive(Clone)]
pub struct BattleStartCallback(pub BattleCallback);

#[derive(Clone)]
pub struct BattleEndCallback(pub BattleCallback<bool>);
