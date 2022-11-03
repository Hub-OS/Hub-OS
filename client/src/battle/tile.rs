use crate::bindable::*;

#[derive(Default, Clone)]
pub struct Tile {
    state: TileState,
    team: Team,
    direction: Direction,
    highlight: TileHighlight,
    washed: bool, // an attack washed the state out
    ignored_attackers: Vec<EntityID>,
    reservations: Vec<EntityID>,
    pub entity_count: usize,
}

impl Tile {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn state(&self) -> TileState {
        self.state
    }

    pub fn set_state(&mut self, state: TileState) {
        if self.state == TileState::Metal && state == TileState::Cracked {
            // can't crack metal panels
            return;
        }

        if self.state.immutable() {
            return;
        }

        self.state = state;
    }

    pub fn team(&self) -> Team {
        self.team
    }

    pub fn set_team(&mut self, team: Team) {
        if team == Team::Unset {
            return;
        }

        if self.team == Team::Unset || self.entity_count == 0 {
            self.team = team;
        }
    }

    pub fn set_direction(&mut self, direction: Direction) {
        self.direction = direction;
    }

    pub fn direction(&self) -> Direction {
        self.direction
    }

    pub fn reset_highlight(&mut self) {
        self.highlight = TileHighlight::None;
    }

    pub fn set_highlight(&mut self, highlight: TileHighlight) {
        self.highlight = self.highlight.max(highlight);
    }

    pub fn apply_wash(&mut self) {
        if self.washed {
            self.state = TileState::Normal;
            self.washed = false;
        }
    }

    pub fn attempt_wash(&mut self, element: Element) {
        self.washed = matches!(
            (self.state, element),
            (TileState::Sand, Element::Wind)
                | (TileState::Grass, Element::Fire)
                | (TileState::Volcano, Element::Aqua)
        );
    }

    pub fn ignoring_attacker(&self, id: EntityID) -> bool {
        self.ignored_attackers.contains(&id)
    }

    pub fn ignore_attacker(&mut self, id: EntityID) {
        if !self.ignored_attackers.contains(&id) {
            self.ignored_attackers.push(id)
        }
    }

    pub fn unignore_attacker(&mut self, id: EntityID) {
        if let Some(i) = self
            .ignored_attackers
            .iter()
            .position(|ignored| *ignored == id)
        {
            self.ignored_attackers.remove(i);
        }
    }

    pub fn reserve_for(&mut self, id: EntityID) {
        if !self.reservations.contains(&id) {
            self.reservations.push(id);
        }
    }

    pub fn remove_reservation_for(&mut self, id: EntityID) {
        if let Some(index) = (self.reservations.iter()).position(|reservation| *reservation == id) {
            self.reservations.swap_remove(index);
        }
    }

    pub fn reservations(&self) -> &[EntityID] {
        &self.reservations
    }

    pub fn has_other_reservations(&self, exclude_id: EntityID) -> bool {
        !self.reservations.is_empty()
            && (self.reservations.len() > 1 || !self.reservations.contains(&exclude_id))
    }

    pub fn horizontal_scale(&self) -> f32 {
        if self.direction == Direction::Left {
            -1.0
        } else {
            1.0
        }
    }

    pub fn apply_bonus_damage(&self, props: &HitProperties) -> bool {
        let element = match self.state {
            TileState::Grass => Element::Wood,
            TileState::Lava => Element::Fire,
            TileState::Sea => Element::Aqua,
            _ => Element::None,
        };

        props.is_super_effective(element)
    }
}