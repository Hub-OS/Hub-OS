use crate::{battle::BattleCallback, bindable::*};

#[derive(Default, Clone)]
pub struct Spell {
    pub requested_highlight: TileHighlight,
    pub hit_props: HitProperties,
    pub collision_callback: BattleCallback<EntityId>,
    pub attack_callback: BattleCallback<EntityId>,
}
