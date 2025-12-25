use crate::bindable::{EntityId, EntityOwner};

#[derive(Default, Clone)]
pub struct OwnershipTracking {
    owned_entities: Vec<(EntityOwner, EntityId)>,
    pub(crate) team_owned_limit: u8,
    pub(crate) entity_owned_limit: u8,
    pub(crate) share_entity_limit: bool,
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

            let same_team_and_type = match (existing_ownership, ownership) {
                (EntityOwner::Team(team_a), EntityOwner::Team(team_b)) => *team_a == team_b,
                (EntityOwner::Entity(team_a, _), EntityOwner::Entity(team_b, _))
                    if self.share_entity_limit =>
                {
                    *team_a == team_b
                }
                (EntityOwner::Entity(_, a), EntityOwner::Entity(_, b)) => *a == b,
                _ => false,
            };

            if !same_team_and_type {
                continue;
            }

            if first_index.is_none() {
                first_index = Some(i);
            }

            count += 1;
        }

        let limit = match ownership {
            EntityOwner::Team(_) => self.team_owned_limit,
            EntityOwner::Entity(_, _) => self.entity_owned_limit,
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
