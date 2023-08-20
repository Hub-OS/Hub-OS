use super::{Action, Entity, TileState};
use crate::bindable::*;
use crate::render::FrameTime;
use crate::resources::{TEMP_TEAM_DURATION, TILE_FLICKER_DURATION};

#[derive(Default, Clone)]
pub struct Tile {
    position: (i32, i32),
    state_index: usize,
    state_lifetime: FrameTime,
    max_state_lifetime: Option<FrameTime>,
    immutable_team: bool,
    team: Team,
    original_team: Team,
    team_revert_timer: FrameTime,
    team_flicker: Option<(Team, FrameTime)>,
    direction: Direction,
    highlight: TileHighlight,
    flash_time: FrameTime,
    washed: bool, // an attack washed the state out
    ignored_attackers: Vec<EntityId>,
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

    pub fn team(&self) -> Team {
        self.team
    }

    pub fn original_team(&self) -> Team {
        self.original_team
    }

    pub fn set_team(&mut self, team: Team) {
        if team == Team::Unset {
            return;
        }

        if self.team == Team::Unset {
            self.original_team = team;
            self.team = team;
            return;
        }

        if !self.reservations.is_empty() || self.immutable_team {
            return;
        }

        self.team = team;

        if team == self.original_team {
            self.team_revert_timer = 0;
        } else if self.team_revert_timer == 0 {
            self.team_revert_timer = TEMP_TEAM_DURATION;
        }
    }

    pub fn team_revert_timer(&self) -> FrameTime {
        self.team_revert_timer
    }

    pub fn sync_team_revert_timer(&mut self, time: FrameTime) {
        self.team_revert_timer = time;

        if time == 0 {
            self.team_flicker = Some((self.team, TILE_FLICKER_DURATION));
            self.team = self.original_team;
        }
    }

    pub fn set_direction(&mut self, direction: Direction) {
        self.direction = direction;
    }

    pub fn direction(&self) -> Direction {
        self.direction
    }

    pub fn should_highlight(&self) -> bool {
        if self.state_index == TileState::HIDDEN {
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

    pub fn apply_wash(&mut self) {
        if self.washed {
            self.state_index = TileState::NORMAL;
            self.washed = false;
        }
    }

    pub fn set_washed(&mut self, washed: bool) {
        self.washed = washed;
    }

    pub fn ignoring_attacker(&self, id: EntityId) -> bool {
        self.ignored_attackers.contains(&id)
    }

    pub fn ignore_attacker(&mut self, id: EntityId) {
        if !self.ignored_attackers.contains(&id) {
            self.ignored_attackers.push(id)
        }
    }

    pub fn unignore_attacker(&mut self, id: EntityId) {
        if let Some(i) = self
            .ignored_attackers
            .iter()
            .position(|ignored| *ignored == id)
        {
            self.ignored_attackers.remove(i);
        }
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
        actions: &generational_arena::Arena<Action>,
        entity: &Entity,
    ) {
        if !Self::can_auto_reserve(actions, entity) {
            return;
        }

        self.reserve_for(entity.id);
    }

    pub fn handle_auto_reservation_removal(
        &mut self,
        actions: &generational_arena::Arena<Action>,
        entity: &Entity,
    ) {
        if !Self::can_auto_reserve(actions, entity) {
            return;
        }

        self.remove_reservation_for(entity.id);
    }

    fn can_auto_reserve(actions: &generational_arena::Arena<Action>, entity: &Entity) -> bool {
        if !entity.auto_reserves_tiles {
            return false;
        }

        if let Some(index) = entity.action_index {
            // synchronous card action
            let action = &actions[index];

            if action.executed {
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
        if self.direction == Direction::Left {
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

        if let Some(max_lifetime) = self.max_state_lifetime {
            if self.state_lifetime > max_lifetime {
                self.set_state_index(TileState::NORMAL, None);
            }
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
