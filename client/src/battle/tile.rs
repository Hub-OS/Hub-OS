use super::{Action, ActionQueue, Entity, Player, TileState};
use crate::bindable::*;
use crate::render::FrameTime;
use crate::resources::{TEMP_TEAM_DURATION, TILE_FLICKER_DURATION};
use crate::structures::DenseSlotMap;

#[derive(Default, Clone)]
pub struct Tile {
    position: (i32, i32),
    state_index: usize,
    state_lifetime: FrameTime,
    state_visual_override: Option<usize>,
    max_state_lifetime: Option<FrameTime>,
    immutable_team: bool,
    team: Team,
    original_team: Team,
    team_reclaim_timer: FrameTime,
    team_flicker: Option<(Team, FrameTime)>,
    original_direction: Direction,
    direction: Direction,
    highlight: TileHighlight,
    flash_time: FrameTime,
    ignored_attackers: Vec<EntityId>,
    active_attackers: Vec<EntityId>,
    reservations: Vec<EntityId>,
}

impl Tile {
    pub fn new(position: (i32, i32), immutable_team: bool) -> Self {
        Self {
            position,
            immutable_team,
            state_index: TileState::NORMAL,
            ..Default::default()
        }
    }

    pub fn position(&self) -> (i32, i32) {
        self.position
    }

    pub fn state_index(&self) -> usize {
        self.state_index
    }

    pub fn set_state_index(&mut self, state: usize, max_state_lifetime: Option<FrameTime>) {
        self.state_lifetime = 0;
        self.state_index = state;
        self.max_state_lifetime = max_state_lifetime;
    }

    pub fn visible_state_index(&self) -> usize {
        self.state_visual_override.unwrap_or(self.state_index)
    }

    pub fn set_visible_state_index(&mut self, state: Option<usize>) {
        self.state_visual_override = state;
    }

    pub fn team(&self) -> Team {
        self.team
    }

    pub fn original_team(&self) -> Team {
        self.original_team
    }

    pub fn set_team(&mut self, entities: &mut hecs::World, team: Team, direction: Direction) {
        if team == Team::Unset {
            return;
        }

        if self.team == Team::Unset {
            self.original_team = team;
            self.team = team;
            self.original_direction = direction;
            self.direction = direction;
            return;
        }

        if self.has_claim_blocking_reservation(entities, team) || self.immutable_team {
            return;
        }

        self.team = team;
        self.direction = direction;

        if team == self.original_team {
            self.team_reclaim_timer = 0;
            self.direction = direction;
        } else if self.team_reclaim_timer == 0 {
            self.team_reclaim_timer = TEMP_TEAM_DURATION;
        }
    }

    pub fn has_claim_blocking_reservation(&self, entities: &mut hecs::World, team: Team) -> bool {
        self.reservations().iter().any(|id| {
            let Ok((entity, _)) = entities.query_one_mut::<(&Entity, &Player)>((*id).into()) else {
                // most reservations should block
                return true;
            };
            // player reservations must be from an opposing team
            entity.team != team
        })
    }

    pub fn team_reclaim_timer(&self) -> FrameTime {
        self.team_reclaim_timer
    }

    pub fn set_team_reclaim_timer(&mut self, time: FrameTime) {
        self.team_reclaim_timer = time;
    }

    pub fn try_reclaim(&mut self) {
        if self.team_reclaim_timer > 0 {
            return;
        }

        if self.team != self.original_team && self.team_flicker.is_none() {
            self.team_flicker = Some((self.team, TILE_FLICKER_DURATION));
        }

        self.team = self.original_team;
        self.direction = self.original_direction;
    }

    pub fn set_direction(&mut self, direction: Direction) {
        self.original_direction = direction;
        self.direction = direction;
    }

    pub fn direction(&self) -> Direction {
        self.direction
    }

    pub fn original_direction(&self) -> Direction {
        self.original_direction
    }

    pub fn should_highlight(&self) -> bool {
        if self.state_index == TileState::VOID {
            return false;
        }

        match self.highlight {
            TileHighlight::None => false,
            TileHighlight::Solid => true,
            TileHighlight::Flash => (self.flash_time / 4) % 2 == 0,
            TileHighlight::Automatic => {
                log::error!(
                    "A tile's TileHighlight is set to Automatic? Should only exist on spells"
                );
                false
            }
        }
    }

    pub fn set_highlight(&mut self, highlight: TileHighlight) {
        self.highlight = self.highlight.max(highlight);
    }

    pub fn ignoring_attacker(&self, id: EntityId) -> bool {
        self.ignored_attackers.contains(&id)
    }

    pub fn ignore_attacker(&mut self, id: EntityId) {
        if !self.ignored_attackers.contains(&id) {
            self.ignored_attackers.push(id)
        }
    }

    pub fn acknowledge_attacker(&mut self, id: EntityId) {
        if !self.active_attackers.contains(&id) {
            self.active_attackers.push(id)
        }
    }

    pub fn unignore_inactive_attackers(&mut self, entities: &mut hecs::World) {
        self.ignored_attackers.retain(|id| {
            if self.active_attackers.contains(id) {
                // keep ignoring continuous attackers
                return true;
            }

            // keep ignoring frozen attackers
            entities
                .query_one_mut::<&Entity>((*id).into())
                .is_ok_and(|e| e.time_frozen)
        });

        self.active_attackers.clear();
    }

    pub fn reserve_for(&mut self, id: EntityId) {
        self.reservations.push(id);
    }

    pub fn remove_reservation_for(&mut self, id: EntityId) {
        if let Some(index) = (self.reservations.iter()).position(|reservation| *reservation == id) {
            self.reservations.swap_remove(index);
        }
    }

    pub fn reservations(&self) -> &[EntityId] {
        &self.reservations
    }

    pub fn has_other_reservations(&self, exclude_id: EntityId) -> bool {
        self.reservations.iter().any(|id| *id != exclude_id)
    }

    pub fn handle_auto_reservation_addition(
        &mut self,
        actions: &DenseSlotMap<Action>,
        entity_id: EntityId,
        entity: &Entity,
        action_queue: Option<&ActionQueue>,
    ) {
        if !Self::can_auto_reserve(actions, entity, action_queue) {
            return;
        }

        self.reserve_for(entity_id);
    }

    pub fn handle_auto_reservation_removal(
        &mut self,
        actions: &DenseSlotMap<Action>,
        entity_id: EntityId,
        entity: &Entity,
        action_queue: Option<&ActionQueue>,
    ) {
        if !Self::can_auto_reserve(actions, entity, action_queue) {
            return;
        }

        self.remove_reservation_for(entity_id);
    }

    fn can_auto_reserve(
        actions: &DenseSlotMap<Action>,
        entity: &Entity,
        action_queue: Option<&ActionQueue>,
    ) -> bool {
        if !entity.auto_reserves_tiles || entity.deleted {
            return false;
        }

        if let Some(index) = action_queue.and_then(|q| q.active) {
            // synchronous card action
            let action = &actions[index];

            if action.executed && !action.allows_auto_reserve {
                // card action began, prevent automatic reservation changes
                return false;
            }
        }

        true
    }

    pub fn clear_reservations_for(&mut self, id: EntityId) {
        self.reservations.retain(|stored_id| *stored_id != id);
    }

    pub fn horizontal_scale(&self) -> f32 {
        if self.original_direction == Direction::Left {
            -1.0
        } else {
            1.0
        }
    }

    pub fn reset_highlight(&mut self) {
        if self.highlight != TileHighlight::Flash {
            self.flash_time = 0;
        } else {
            self.flash_time += 1;
        }

        self.highlight = TileHighlight::None;
    }

    pub fn update_state(&mut self) {
        self.state_lifetime += 1;

        if let Some(max_lifetime) = self.max_state_lifetime
            && self.state_lifetime > max_lifetime
        {
            self.set_state_index(TileState::NORMAL, None);
        }
    }

    pub fn update_team_flicker(&mut self) {
        if let Some((_, timer)) = &mut self.team_flicker {
            *timer -= 1;

            if *timer == 0 {
                self.team_flicker = None;
            }
        }
    }

    pub fn visible_team(&self) -> Team {
        if let Some((team, timer)) = self.team_flicker {
            if (timer / 4) % 2 == 0 {
                self.team
            } else {
                team
            }
        } else {
            self.team
        }
    }

    pub fn flicker_normal_state(&self) -> bool {
        if let Some(max_lifetime) = self.max_state_lifetime {
            let flicker_elapsed = self.state_lifetime - (max_lifetime - TILE_FLICKER_DURATION);

            if flicker_elapsed > 0 && (flicker_elapsed / 2) % 2 == 0 {
                return true;
            }
        }

        false
    }
}
