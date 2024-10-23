use crate::battle::ActionTypes;
use crate::bindable::EntityId;
use crate::structures::GenerationalIndex;
use std::collections::VecDeque;

#[derive(Default, Clone)]
pub struct ActionQueue {
    pub pending: VecDeque<GenerationalIndex>,
    pub active: Option<GenerationalIndex>,
    pub action_type: ActionTypes,
}

impl ActionQueue {
    pub fn ensure(entities: &mut hecs::World, entity_id: EntityId) {
        let id = entity_id.into();

        if !entities.satisfies::<&ActionQueue>(id).unwrap_or_default() {
            let _ = entities.insert_one(id, ActionQueue::default());
        }
    }
}
