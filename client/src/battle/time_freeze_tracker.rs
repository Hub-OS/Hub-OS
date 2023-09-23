use super::{
    Artifact, BattleCallback, BattleSimulation, Character, Entity, Player, SharedBattleResources,
    TimeFreezeEntityBackup,
};
use crate::bindable::Team;
use crate::ease::inverse_lerp;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths, RESOLUTION_F};
use crate::structures::GenerationalIndex;
use framework::prelude::{Color, GameIO, Vec2};
use packets::structures::Input;
use std::collections::VecDeque;

const FADE_DURATION: FrameTime = 10;
const COUNTER_DURATION: FrameTime = 60;
const MAX_ACTION_DURATION: FrameTime = 60 * 15;

#[derive(Default, Clone, Copy, PartialEq, Eq)]
enum TimeFreezeState {
    #[default]
    Thawed,
    // freezing actions flow
    Freeze,
    FadeIn,
    Counterable,
    Action,
    FadeOut,
    // animation
    Animation(FrameTime),
}

#[derive(Default, Clone)]
pub struct TimeFreezeTracker {
    chain: Vec<(Team, GenerationalIndex)>,
    active_time: FrameTime,
    state_start_time: FrameTime,
    state: TimeFreezeState,
    should_defrost: bool,
    character_backup: Option<TimeFreezeEntityBackup>,
    animation_queue: VecDeque<(BattleCallback, FrameTime)>,
}

impl TimeFreezeTracker {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn time_is_frozen(&self) -> bool {
        self.state != TimeFreezeState::Thawed
    }

    fn is_action_freeze(&self) -> bool {
        self.time_is_frozen() && !matches!(self.state, TimeFreezeState::Animation(_))
    }

    fn fade_alpha(&self) -> f32 {
        let state_elapsed_time = self.active_time - self.state_start_time;

        match self.state {
            TimeFreezeState::Thawed => 0.0,
            TimeFreezeState::Freeze => 0.0,
            TimeFreezeState::FadeIn => inverse_lerp!(0, FADE_DURATION, state_elapsed_time),
            TimeFreezeState::Counterable | TimeFreezeState::Action => 1.0,
            TimeFreezeState::FadeOut => inverse_lerp!(FADE_DURATION, 0, state_elapsed_time),
            TimeFreezeState::Animation(_) => 0.0,
        }
    }

    pub fn set_team_action(&mut self, team: Team, action_index: GenerationalIndex) {
        if let Some(index) = self.chain.iter().position(|(t, _)| *t == team) {
            self.chain.remove(index);
        }

        if !self.time_is_frozen() {
            self.active_time = 0;
            self.state = TimeFreezeState::Freeze;
        }

        // set the state start time to allow other players to counter, as well as initialize
        self.state_start_time = self.active_time;

        self.chain.push((team, action_index));
    }

    pub fn queue_animation(&mut self, duration: FrameTime, begin_callback: BattleCallback) {
        if !self.time_is_frozen() {
            // set the state to an immediately completed animation
            // it will trigger the queue to pop
            self.state = TimeFreezeState::Animation(0);
        }

        self.animation_queue.push_back((begin_callback, duration));
    }

    pub fn last_team(&self) -> Option<Team> {
        self.chain.last().map(|(team, _)| team).cloned()
    }

    /// Returns true if an Action has enough time to counter if it enters the queue this frame
    pub fn can_counter(&self) -> bool {
        let state_elapsed_time = self.active_time - self.state_start_time;

        if let TimeFreezeState::Counterable = self.state {
            COUNTER_DURATION - state_elapsed_time > 2
        } else {
            false
        }
    }

    /// Returns true if queued Actions can counter
    pub fn can_queued_counter(&self) -> bool {
        self.state == TimeFreezeState::Counterable
    }

    fn should_freeze(&self) -> bool {
        matches!(
            self.state,
            TimeFreezeState::Freeze | TimeFreezeState::Animation(_)
        ) && self.active_time - self.state_start_time == 0
    }

    fn action_should_start(&self) -> bool {
        self.state == TimeFreezeState::Action && self.active_time == self.state_start_time
    }

    fn active_action(&self) -> Option<GenerationalIndex> {
        if self.state != TimeFreezeState::Action {
            return None;
        }

        self.chain.last().map(|(_, index)| index).cloned()
    }

    fn end_action(&mut self) {
        self.active_time = self.state_start_time + MAX_ACTION_DURATION;
    }

    fn advance_action(&mut self) {
        self.chain.pop();

        if self.chain.is_empty() {
            self.state = TimeFreezeState::FadeOut;
        } else {
            self.state = TimeFreezeState::Action;
        }

        self.state_start_time = self.active_time;
    }

    fn action_out_of_time(&self) -> bool {
        self.state == TimeFreezeState::Action
            && self.active_time - self.state_start_time >= MAX_ACTION_DURATION
    }

    #[must_use]
    fn advance_queue(&mut self) -> Option<BattleCallback> {
        self.active_time = 0;
        self.state_start_time = 0;

        if let Some((callback, duration)) = self.animation_queue.pop_front() {
            // process the next animation
            self.state = TimeFreezeState::Animation(duration);
            Some(callback)
        } else if !self.chain.is_empty() {
            // start processing actions
            self.state = TimeFreezeState::FadeIn;
            None
        } else {
            self.should_defrost = true;
            None
        }
    }

    fn set_entity_backup(&mut self, backup: TimeFreezeEntityBackup) {
        self.character_backup = Some(backup);
    }

    fn take_entity_backup(&mut self) -> Option<TimeFreezeEntityBackup> {
        self.character_backup.take()
    }

    fn increment_time(&mut self) {
        self.active_time += 1;
        self.should_defrost = false;

        let state_elapsed_time = self.active_time - self.state_start_time;

        match self.state {
            TimeFreezeState::Thawed | TimeFreezeState::Action => {}
            TimeFreezeState::Freeze => {
                self.state = TimeFreezeState::FadeIn;
                self.state_start_time = self.active_time;
            }
            TimeFreezeState::FadeIn => {
                if state_elapsed_time >= FADE_DURATION {
                    self.state = TimeFreezeState::Counterable;
                    self.state_start_time = self.active_time;
                }
            }
            TimeFreezeState::Counterable => {
                if state_elapsed_time >= COUNTER_DURATION {
                    self.state = TimeFreezeState::Action;
                    self.state_start_time = self.active_time;
                }
            }
            TimeFreezeState::FadeOut => {
                if state_elapsed_time >= FADE_DURATION {
                    self.state = TimeFreezeState::Thawed;
                    self.state_start_time = self.active_time;
                }
            }
            TimeFreezeState::Animation(duration) => {
                if state_elapsed_time >= duration {
                    self.state = TimeFreezeState::Thawed;
                    self.state_start_time = self.active_time;
                }
            }
        };
    }

    pub fn update(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if !simulation.time_freeze_tracker.time_is_frozen() {
            return;
        }

        // update fade color
        const FADE_COLOR: Color = Color::new(0.0, 0.0, 0.0, 0.3);

        let fade_alpha = simulation.time_freeze_tracker.fade_alpha();
        let fade_color = FADE_COLOR.multiply_alpha(fade_alpha);
        simulation.fade_sprite.set_color(fade_color);

        // detect freeze start
        if simulation.time_freeze_tracker.should_freeze() {
            Self::freeze(simulation);

            if simulation.time_freeze_tracker.is_action_freeze() {
                // play sfx
                let globals = game_io.resource::<Globals>().unwrap();
                simulation.play_sound(game_io, &globals.sfx.time_freeze);
            }
        }

        // state based updates
        if simulation.time_freeze_tracker.is_action_freeze() {
            Self::action_update(game_io, resources, simulation);
        } else {
            // just increment the time
            simulation.time_freeze_tracker.increment_time();
        }

        // handle any pending freezes, such as decross animation, or time freeze actions after an animation
        if !simulation.time_freeze_tracker.time_is_frozen() {
            if let Some(callback) = simulation.time_freeze_tracker.advance_queue() {
                simulation.pending_callbacks.push(callback);
                simulation.call_pending_callbacks(game_io, resources);
            }
        }

        if simulation.time_freeze_tracker.should_defrost {
            Self::defrost(simulation);
        }
    }

    fn freeze(simulation: &mut BattleSimulation) {
        // freeze non artifacts
        type Query<'a> = hecs::Without<&'a mut Entity, &'a Artifact>;

        for (_, entity) in simulation.entities.query_mut::<Query>() {
            entity.time_frozen = true;
        }
    }

    fn defrost(simulation: &mut BattleSimulation) {
        // unfreeze all entities
        for (_, entity) in simulation.entities.query_mut::<&mut Entity>() {
            if entity.time_frozen {
                entity.time_frozen = false;
            }
        }
    }

    fn action_update(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        // detect time freeze counter
        let time_freeze_tracker = &mut simulation.time_freeze_tracker;

        if time_freeze_tracker.can_counter() {
            Self::detect_counter_attempt(game_io, resources, simulation);
        }

        // detect action start
        if simulation.time_freeze_tracker.action_should_start() {
            Self::begin_action(simulation);
        }

        // detect action end
        if let Some(index) = simulation.time_freeze_tracker.active_action() {
            if !simulation.actions.contains_key(index) {
                // action completed, update tracking
                simulation.time_freeze_tracker.end_action();
            }
        }

        // increment the time
        simulation.time_freeze_tracker.increment_time();

        // handle the action running out of time
        if simulation.time_freeze_tracker.action_out_of_time() {
            if let Some(entity_backup) = simulation.time_freeze_tracker.take_entity_backup() {
                entity_backup.restore(game_io, resources, simulation);
            }

            simulation.time_freeze_tracker.advance_action();
        }
    }

    fn detect_counter_attempt(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let last_team = simulation.time_freeze_tracker.last_team().unwrap();

        type Query<'a> = (&'a Entity, &'a Player, &'a Character);
        let entities = &mut simulation.entities;

        for (id, (entity, player, character)) in entities.query_mut::<Query>() {
            if entity.deleted || entity.team == last_team {
                // can't counter
                // can't counter a card from the same team
                continue;
            }

            if !simulation.inputs[player.index].was_just_pressed(Input::UseCard) {
                // didn't try to counter
                continue;
            }

            if let Some(card_props) = character.cards.last() {
                if !card_props.time_freeze {
                    // must counter with a time freeze card
                    continue;
                }
            } else {
                // no cards to counter with
                continue;
            }

            Character::use_card(game_io, resources, simulation, id.into());
            break;
        }
    }

    fn begin_action(simulation: &mut BattleSimulation) {
        let action_index = simulation.time_freeze_tracker.active_action().unwrap();

        if let Some(action) = simulation.actions.get(action_index) {
            let entity_id = action.entity;

            // unfreeze our entity
            if let Some(entity_backup) =
                TimeFreezeEntityBackup::backup_and_prepare(simulation, entity_id, action_index)
            {
                simulation
                    .time_freeze_tracker
                    .set_entity_backup(entity_backup);
            } else {
                // entity erased?
                simulation.time_freeze_tracker.end_action();
                log::error!("Time freeze entity erased, yet action still exists?");
            }
        } else {
            // action deleted?
            simulation.time_freeze_tracker.end_action();
        }
    }

    pub fn draw_ui(
        &self,
        game_io: &GameIO,
        simulation: &BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        const MARGIN_TOP: f32 = 38.0;
        const BAR_WIDTH: f32 = 80.0;

        if !self.can_counter() {
            return;
        }

        let (team, index) = self.chain.last().cloned().unwrap();

        let Some(action) = simulation.actions.get(index) else {
            return;
        };

        // resolve where we should draw
        let mut position = Vec2::new(0.0, MARGIN_TOP);

        position.x = match team {
            Team::Red => RESOLUTION_F.x * 0.25,
            Team::Blue => RESOLUTION_F.x * 0.75,
            _ => RESOLUTION_F.x * 0.5,
        };

        if simulation.local_team.flips_perspective() {
            position.x = RESOLUTION_F.x - position.x;
        }

        let card_props = &action.properties;
        card_props.draw_summary(game_io, sprite_queue, position, true);

        // drawing bar
        let text_style = TextStyle::new(game_io, FontStyle::Thick);
        position.x -= BAR_WIDTH * 0.5;
        position.y += text_style.line_height() + text_style.line_spacing;

        let elapsed_time = self.active_time - self.state_start_time;
        let width_multiplier = inverse_lerp!(COUNTER_DURATION, 0, elapsed_time);

        let assets = &game_io.resource::<Globals>().unwrap().assets;
        let mut sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        sprite.set_width(BAR_WIDTH * width_multiplier);
        sprite.set_height(2.0);

        // draw bar shadow
        sprite.set_position(position + 1.0);
        sprite.set_color(Color::BLACK);
        sprite_queue.draw_sprite(&sprite);

        // draw bar
        sprite.set_position(position);
        sprite.set_color(Color::WHITE);
        sprite_queue.draw_sprite(&sprite);
    }
}
