use crate::bindable::{EntityId, EntityOwner};

#[derive(Default, Clone)]
pub struct OwnershipTracking {
    owned_entities: Vec<(EntityOwner, EntityId)>,
}

impl OwnershipTracking {
    #[must_use]
    // returns an entity to delete
    pub fn track(&mut self, ownership: EntityOwner, entity_id: EntityId) -> Option<EntityId> {
        let mut first_index = None;
        let mut count = 0;
        let mut updated_ownership = false;

        for (i, (existing_ownership, existing_id)) in self.owned_entities.iter_mut().enumerate() {
            if *existing_id == entity_id {
                // matches an existing id, update who owns it
                *existing_ownership = ownership;
                updated_ownership = true;
            }

            if *existing_ownership != ownership {
                continue;
            }

            if first_index.is_none() {
                first_index = Some(i);
            }

            count += 1;
        }

        let limit = match ownership {
            EntityOwner::Team(_) => 2,
            EntityOwner::Entity(_) => 1,
        };

        let pending_deletion = if count >= limit {
            let (_, id) = self.owned_entities.remove(first_index.unwrap());
            Some(id)
        } else {
            None
        };

        if !updated_ownership {
            // only push if we didn't already track this entity
            self.owned_entities.push((ownership, entity_id));
        }

        pending_deletion
    }

    pub fn untrack(&mut self, entity_id: EntityId) {
        self.owned_entities.retain(|(_, other)| *other != entity_id);
    }

    pub fn resolve_owner(&mut self, entity_id: EntityId) -> Option<EntityOwner> {
        self.owned_entities
            .iter()
            .find(|(_, id)| *id == entity_id)
            .map(|(owner, _)| *owner)
    }
}
