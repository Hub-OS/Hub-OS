use std::collections::VecDeque;

use super::{BattleCallback, BattleSimulation, SharedBattleResources, TimeFreezeEntityBackup};
use crate::bindable::Team;
use crate::ease::inverse_lerp;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths, RESOLUTION_F};
use crate::structures::GenerationalIndex;
use framework::prelude::{Color, GameIO, Vec2};

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

    pub fn is_action_freeze(&self) -> bool {
        self.time_is_frozen() && !matches!(self.state, TimeFreezeState::Animation(_))
    }

    pub fn fade_alpha(&self) -> f32 {
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

    pub fn tick(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        simulation.time_freeze_tracker.increment_time();

        if simulation.time_freeze_tracker.action_out_of_time() {
            if let Some(entity_backup) = simulation.time_freeze_tracker.take_entity_backup() {
                entity_backup.restore(game_io, resources, simulation);
            }

            simulation.time_freeze_tracker.advance_action();
        }

        if !simulation.time_freeze_tracker.time_is_frozen() {
            // handle any pending animations such as decross
            if let Some(callback) = simulation.time_freeze_tracker.advance_animation() {
                simulation.pending_callbacks.push(callback);
                simulation.call_pending_callbacks(game_io, resources);
            }
        }
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
                    self.should_defrost = true;
                }
            }
            TimeFreezeState::Animation(duration) => {
                if state_elapsed_time >= duration {
                    self.state = TimeFreezeState::Thawed;
                    self.state_start_time = self.active_time;
                    self.should_defrost = true;
                }
            }
        };
    }

    pub fn last_team(&self) -> Option<Team> {
        self.chain.last().map(|(team, _)| team).cloned()
    }

    pub fn can_counter(&self) -> bool {
        let state_elapsed_time = self.active_time - self.state_start_time;

        if let TimeFreezeState::Counterable = self.state {
            COUNTER_DURATION - state_elapsed_time > 2
        } else {
            false
        }
    }

    pub fn can_queued_counter(&self) -> bool {
        self.state == TimeFreezeState::Counterable
    }

    pub fn should_freeze(&self) -> bool {
        matches!(
            self.state,
            TimeFreezeState::Freeze | TimeFreezeState::Animation(_)
        ) && self.active_time - self.state_start_time == 0
    }

    pub fn should_defrost(&self) -> bool {
        self.should_defrost
    }

    pub fn action_should_start(&self) -> bool {
        self.state == TimeFreezeState::Action && self.active_time == self.state_start_time
    }

    pub fn active_action(&self) -> Option<GenerationalIndex> {
        if self.state != TimeFreezeState::Action {
            return None;
        }

        self.chain.last().map(|(_, index)| index).cloned()
    }

    pub fn end_action(&mut self) {
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
    fn advance_animation(&mut self) -> Option<BattleCallback> {
        self.state_start_time = self.active_time;

        if let Some((callback, duration)) = self.animation_queue.pop_front() {
            self.state = TimeFreezeState::Animation(duration);
            self.should_defrost = false;
            Some(callback)
        } else {
            self.state = TimeFreezeState::Thawed;
            self.should_defrost = true;
            None
        }
    }

    pub fn set_entity_backup(&mut self, backup: TimeFreezeEntityBackup) {
        self.character_backup = Some(backup);
    }

    pub fn take_entity_backup(&mut self) -> Option<TimeFreezeEntityBackup> {
        self.character_backup.take()
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
