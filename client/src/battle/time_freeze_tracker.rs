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

#[derive(Clone, Copy, PartialEq, Eq)]
enum ActionFreezeState {
    Freeze,
    FadeIn,
    Countered,
    DisplaySummary,
    Counterable,
    HideSummary,
    Action,
    ActionCleanup,
    FadeOut,
}

impl ActionFreezeState {
    const FADE_DURATION: FrameTime = 10;
    const COUNTER_DURATION: FrameTime = 60;
    const SUMMARY_TRANSITION_DURATION: FrameTime = 10;
    const COUNTERED_DURATION: FrameTime = Self::SUMMARY_TRANSITION_DURATION + 10;
    const MAX_ACTION_DURATION: FrameTime = 60 * 15;

    fn summary_scale(self, state_elapsed_time: FrameTime) -> f32 {
        match self {
            Self::DisplaySummary => {
                inverse_lerp!(0, Self::SUMMARY_TRANSITION_DURATION, state_elapsed_time)
            }
            Self::Counterable => 1.0,
            Self::HideSummary | Self::Countered => {
                inverse_lerp!(Self::SUMMARY_TRANSITION_DURATION, 0, state_elapsed_time)
            }
            _ => 0.0,
        }
    }

    fn fade_alpha(self, state_elapsed_time: FrameTime) -> f32 {
        match self {
            Self::Freeze => 0.0,
            Self::FadeIn => inverse_lerp!(0, Self::FADE_DURATION, state_elapsed_time),
            Self::FadeOut => inverse_lerp!(Self::FADE_DURATION, 0, state_elapsed_time),
            _ => 1.0,
        }
    }

    fn duration(self) -> FrameTime {
        match self {
            Self::Freeze => 0,
            Self::FadeIn | Self::FadeOut => Self::FADE_DURATION,
            Self::Countered => Self::COUNTERED_DURATION,
            Self::DisplaySummary | Self::HideSummary => Self::SUMMARY_TRANSITION_DURATION,
            Self::Counterable => Self::COUNTER_DURATION,
            Self::Action => Self::MAX_ACTION_DURATION,
            Self::ActionCleanup => {
                // let the cleanup logic handle the timing
                FrameTime::MAX
            }
        }
    }

    fn next_state(self) -> TimeFreezeState {
        match self {
            Self::Freeze => Self::FadeIn.into(),
            Self::FadeIn | Self::Countered => Self::DisplaySummary.into(),
            Self::DisplaySummary => Self::Counterable.into(),
            Self::Counterable => Self::HideSummary.into(),
            Self::HideSummary => Self::Action.into(),
            Self::Action => Self::ActionCleanup.into(),
            Self::ActionCleanup => Self::FadeOut.into(),
            Self::FadeOut => TimeFreezeState::Thawed,
        }
    }
}

#[derive(Default, Clone, Copy, PartialEq, Eq)]
enum TimeFreezeState {
    #[default]
    Thawed,
    Action(ActionFreezeState),
    Animation(FrameTime),
}

impl From<ActionFreezeState> for TimeFreezeState {
    fn from(action_freeze_state: ActionFreezeState) -> Self {
        Self::Action(action_freeze_state)
    }
}

#[derive(Default, Clone)]
pub struct TimeFreezeTracker {
    action_chain: Vec<(Team, GenerationalIndex)>,
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
        matches!(self.state, TimeFreezeState::Action(_))
    }

    fn fade_alpha(&self) -> f32 {
        let state_elapsed_time = self.active_time - self.state_start_time;

        match self.state {
            TimeFreezeState::Thawed => 0.0,
            TimeFreezeState::Action(action_freeze_state) => {
                action_freeze_state.fade_alpha(state_elapsed_time)
            }
            TimeFreezeState::Animation(_) => 0.0,
        }
    }

    pub fn set_team_action(&mut self, team: Team, action_index: GenerationalIndex) {
        if let Some(index) = self.action_chain.iter().position(|(t, _)| *t == team) {
            self.action_chain.remove(index);
        }

        if !self.time_is_frozen() {
            self.active_time = 0;
            self.state = ActionFreezeState::Freeze.into();
        } else if self.can_processing_action_counter() {
            self.state = ActionFreezeState::Countered.into();
        }

        // set the state start time to allow other players to counter, as well as initialize
        self.state_start_time = self.active_time;

        self.action_chain.push((team, action_index));
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
        self.action_chain.last().map(|(team, _)| team).cloned()
    }

    /// Returns true if an Action has enough time to counter if it enters the queue this frame
    pub fn can_counter(&self) -> bool {
        let state_elapsed_time = self.active_time - self.state_start_time;

        if self.can_processing_action_counter() {
            ActionFreezeState::COUNTER_DURATION - state_elapsed_time > 2
        } else {
            false
        }
    }

    /// Returns true if processing Actions can counter
    pub fn can_processing_action_counter(&self) -> bool {
        self.state == ActionFreezeState::Counterable.into()
    }

    fn should_freeze(&self) -> bool {
        matches!(
            self.state,
            TimeFreezeState::Action(ActionFreezeState::Freeze) | TimeFreezeState::Animation(_)
        ) && self.active_time == self.state_start_time
    }

    fn active_action(&self) -> Option<GenerationalIndex> {
        if self.state != ActionFreezeState::Action.into() {
            return None;
        }

        self.action_chain.last().map(|(_, index)| index).cloned()
    }

    fn end_action(&mut self) {
        self.state = ActionFreezeState::ActionCleanup.into();
    }

    fn advance_action(&mut self) {
        self.action_chain.pop();

        if self.action_chain.is_empty() {
            self.state = ActionFreezeState::FadeOut.into();
        } else {
            self.state = ActionFreezeState::Action.into();
        }

        self.state_start_time = self.active_time;
    }

    fn should_cleanup_action(&self) -> bool {
        self.state == ActionFreezeState::ActionCleanup.into()
    }

    #[must_use]
    fn advance_queue(&mut self) -> Option<BattleCallback> {
        self.active_time = 0;
        self.state_start_time = 0;

        if let Some((callback, duration)) = self.animation_queue.pop_front() {
            // process the next animation
            self.state = TimeFreezeState::Animation(duration);
            Some(callback)
        } else if !self.action_chain.is_empty() {
            // start processing actions
            self.state = ActionFreezeState::FadeIn.into();
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
            TimeFreezeState::Thawed => {}
            TimeFreezeState::Action(action_freeze_state) => {
                if state_elapsed_time >= action_freeze_state.duration() {
                    self.state = action_freeze_state.next_state();
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
        let time_freeze_tracker = &mut simulation.time_freeze_tracker;

        let TimeFreezeState::Action(action_freeze_state) = time_freeze_tracker.state else {
            log::error!("Expecting TimeFreezeState::Action");
            return;
        };

        let state_just_started =
            time_freeze_tracker.state_start_time == time_freeze_tracker.active_time;

        match action_freeze_state {
            ActionFreezeState::FadeIn => {
                if state_just_started {
                    let globals = game_io.resource::<Globals>().unwrap();
                    simulation.play_sound(game_io, &globals.sfx.time_freeze);
                }
            }
            ActionFreezeState::Countered => {
                if state_just_started {
                    let globals = game_io.resource::<Globals>().unwrap();
                    globals.audio.play_sound(&globals.sfx.trap);
                }
            }
            ActionFreezeState::Counterable => {
                if time_freeze_tracker.can_counter() {
                    Self::detect_counter_attempt(game_io, resources, simulation);
                }
            }
            ActionFreezeState::Action => {
                if state_just_started {
                    Self::begin_action(simulation);
                }
            }
            _ => {}
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

        // handle the action cleanup
        if simulation.time_freeze_tracker.should_cleanup_action() {
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

    fn team_ui_position(simulation: &BattleSimulation, team: Team) -> Vec2 {
        const MARGIN_TOP: f32 = 38.0;

        let mut x = match team {
            Team::Red => RESOLUTION_F.x * 0.25,
            Team::Blue => RESOLUTION_F.x * 0.75,
            _ => RESOLUTION_F.x * 0.5,
        };

        if simulation.local_team.flips_perspective() {
            x = RESOLUTION_F.x - x;
        }

        Vec2::new(x, MARGIN_TOP)
    }

    pub fn draw_ui(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        let TimeFreezeState::Action(action_state) = self.state else {
            return;
        };

        match action_state {
            ActionFreezeState::DisplaySummary
            | ActionFreezeState::Counterable
            | ActionFreezeState::HideSummary => {
                self.draw_counterable_action_text(game_io, simulation, sprite_queue, action_state);
            }
            ActionFreezeState::Countered => {
                self.draw_countered_action_text(game_io, resources, simulation, sprite_queue);
            }
            _ => {}
        }
    }

    fn draw_counterable_action_text(
        &self,
        game_io: &GameIO,
        simulation: &BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
        action_state: ActionFreezeState,
    ) {
        const BAR_WIDTH: f32 = 80.0;

        let (team, index) = self.action_chain.last().cloned().unwrap();

        let Some(action) = simulation.actions.get(index) else {
            return;
        };

        let state_elapsed_time = self.active_time - self.state_start_time;
        let summary_scale_y = action_state.summary_scale(state_elapsed_time);

        // resolve where we should draw
        let mut position = Self::team_ui_position(simulation, team);

        // draw action name and damage
        let card_props = &action.properties;
        let action_summary_scale = Vec2::new(1.0, summary_scale_y);
        card_props.draw_summary(game_io, sprite_queue, position, action_summary_scale, true);

        // drawing bar
        if action_state == ActionFreezeState::Counterable {
            let text_style = TextStyle::new(game_io, FontStyle::Thick);
            position.x -= BAR_WIDTH * 0.5;
            position.y += text_style.line_height() + text_style.line_spacing;

            let width_multiplier =
                inverse_lerp!(ActionFreezeState::COUNTER_DURATION, 0, state_elapsed_time);

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

    fn draw_countered_action_text(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        let Some((team, index)) = self.action_chain.iter().rev().nth(1).cloned() else {
            return;
        };

        let state_elapsed_time = self.active_time - self.state_start_time;
        let summary_scale_y = ActionFreezeState::Countered.summary_scale(state_elapsed_time);
        let position = Self::team_ui_position(simulation, team);

        // draw alert for the TFC
        let mut alert_animator = resources.alert_animator.borrow_mut();
        alert_animator.sync_time(state_elapsed_time - 1);

        if !alert_animator.is_complete() {
            let globals = game_io.resource::<Globals>().unwrap();
            let assets = &globals.assets;

            let mut alert_sprite = assets.new_sprite(game_io, ResourcePaths::BATTLE_ALERT);
            alert_sprite.set_position(position);
            alert_animator.apply(&mut alert_sprite);

            sprite_queue.draw_sprite(&alert_sprite);
        }

        // draw summary text
        if let Some(action) = simulation.actions.get(index) {
            let card_props = &action.properties;
            let scale = Vec2::new(1.0, summary_scale_y);
            card_props.draw_summary(game_io, sprite_queue, position, scale, true);
        }
    }
}
