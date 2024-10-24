use super::{Entity, Spell};
use crate::bindable::{EntityId, HitFlag, HitProperties, Team, TileHighlight};

#[derive(Clone)]
pub struct AttackBox {
    pub attacker_id: EntityId,
    pub team: Team,
    pub x: i32,
    pub y: i32,
    pub props: HitProperties,
    pub highlight: bool,
}

impl AttackBox {
    pub fn new_from(
        (x, y): (i32, i32),
        entity_id: EntityId,
        entity: &Entity,
        spell: &Spell,
    ) -> Self {
        let mut props = spell.hit_props.clone();

        // apply status durations from context
        for &(flag, mut duration) in props.context.durations.iter() {
            if let Some(existing) = props.durations.get(&flag) {
                duration = duration.max(*existing);
            }

            props.durations.insert(flag, duration);
        }

        // overwrite drag if the hit props doesn't directly define any
        if props.flags & HitFlag::DRAG == 0 {
            props.drag = props.context.drag;
        }

        // apply context to hit props
        props.flags |= props.context.flags;

        Self {
            attacker_id: entity_id,
            team: entity.team,
            x,
            y,
            props,
            highlight: spell.requested_highlight == TileHighlight::Automatic,
        }
    }
}
