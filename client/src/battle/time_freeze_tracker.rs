use super::{
    Action, Artifact, BattleCallback, BattleSimulation, Character, Entity, Player,
    SharedBattleResources, TimeFreezeEntityBackup,
};
use crate::bindable::{CardProperties, EntityId, SpriteColorMode, Team, TimeFreezeChainLimit};
use crate::ease::inverse_lerp;
use crate::render::ui::{FontName, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue, SpriteShaderEffect};
use crate::resources::{
    AssetManager, BATTLE_INFO_SHADOW_COLOR, Globals, RESOLUTION_F, ResourcePaths,
};
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
    BeginAction,
    Action,
    ActionCleanup,
    PollEntityAction,
    FadeOut,
}

impl ActionFreezeState {
    const FADE_DURATION: FrameTime = 10;
    const COUNTER_DURATION: FrameTime = 60;
    const SUMMARY_TRANSITION_DURATION: FrameTime = 10;
    const COUNTERED_DURATION: FrameTime = Self::SUMMARY_TRANSITION_DURATION + 10;
    const MAX_ACTION_DURATION: FrameTime = 60 * 15;

    fn displaying_top_summary(self) -> bool {
        matches!(
            self,
            ActionFreezeState::DisplaySummary
                | ActionFreezeState::Counterable
                | ActionFreezeState::HideSummary
        )
    }

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
            Self::BeginAction => 1,
            Self::Action => Self::MAX_ACTION_DURATION,
            Self::ActionCleanup => 1,
            Self::PollEntityAction => 1,
        }
    }

    fn next_state(self) -> TimeFreezeState {
        match self {
            Self::Freeze => Self::FadeIn.into(),
            Self::FadeIn | Self::Countered => Self::DisplaySummary.into(),
            Self::DisplaySummary => Self::Counterable.into(),
            Self::Counterable => Self::HideSummary.into(),
            Self::HideSummary => Self::BeginAction.into(),
            Self::BeginAction => Self::Action.into(),
            Self::Action => Self::ActionCleanup.into(),
            Self::ActionCleanup => Self::PollEntityAction.into(),
            Self::PollEntityAction => Self::FadeOut.into(),
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
struct TrackedAction {
    team: Team,
    entity: EntityId,
    action_index: GenerationalIndex,
    prevent_counter: bool,
}

#[derive(Default, Clone)]
pub struct TimeFreezeTracker {
    action_chain: Vec<TrackedAction>,
    active_time: FrameTime,
    state_start_time: FrameTime,
    state: TimeFreezeState,
    previous_state: TimeFreezeState,
    should_defrost: bool,
    skipping_intros: bool,
    chain_limit: TimeFreezeChainLimit,
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

    pub fn set_chain_limit(&mut self, limit: TimeFreezeChainLimit) {
        self.chain_limit = limit;
    }

    #[must_use]
    pub fn set_team_action(
        &mut self,
        team: Team,
        action_index: GenerationalIndex,
        action: &Action,
    ) -> Option<GenerationalIndex> {
        let tracked_action = TrackedAction {
            team,
            entity: action.entity,
            action_index,
            prevent_counter: action.properties.prevent_time_freeze_counter,
        };

        if matches!(
            self.state,
            TimeFreezeState::Action(ActionFreezeState::PollEntityAction)
        ) {
            // action queued in the middle of an existing action
            let index = if self
                .action_chain
                .last()
                .is_some_and(|t| t.entity == action.entity)
            {
                // make this play as the next action / after clean up
                self.action_chain.len().saturating_sub(1)
            } else {
                // make this the last action to run
                0
            };

            self.action_chain.insert(index, tracked_action);

            return None;
        }

        let mut tracked_action_iter = self.action_chain.iter();
        let dropped_chain_index = match self.chain_limit {
            TimeFreezeChainLimit::Unlimited => None,
            TimeFreezeChainLimit::OnePerTeam => tracked_action_iter.position(|t| t.team == team),
            TimeFreezeChainLimit::OnePerEntity => {
                tracked_action_iter.position(|t| t.entity == action.entity)
            }
        };

        let dropped_action_index = dropped_chain_index.map(|index| {
            let tracked_action = self.action_chain.remove(index);
            tracked_action.action_index
        });

        if !self.time_is_frozen() {
            self.active_time = 0;
            self.state = ActionFreezeState::Freeze.into();
            self.skipping_intros = action.properties.skip_time_freeze_intro;
        } else if self.can_processing_action_counter() {
            self.state = ActionFreezeState::Countered.into();
        } else if action.properties.skip_time_freeze_intro {
            self.state = ActionFreezeState::BeginAction.into();
        } else {
            self.state = ActionFreezeState::DisplaySummary.into();
        }

        // set the state start time to allow other players to counter, as well as initialize
        self.state_start_time = self.active_time;

        self.action_chain.push(tracked_action);

        dropped_action_index
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
        self.action_chain.last().map(|t| t.team)
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
            && self.action_chain.last().is_some_and(|t| !t.prevent_counter)
    }

    // the entity polled for back to back freezes
    pub fn polled_entity(&self) -> Option<EntityId> {
        if self.state != ActionFreezeState::PollEntityAction.into() {
            return None;
        }

        self.action_chain.last().map(|t| t.entity)
    }

    fn should_freeze(&self) -> bool {
        matches!(
            self.state,
            TimeFreezeState::Action(ActionFreezeState::Freeze)
                | TimeFreezeState::Action(ActionFreezeState::ActionCleanup)
                | TimeFreezeState::Animation(_)
        ) && self.active_time == self.state_start_time
    }

    pub fn active_action_index(&self) -> Option<GenerationalIndex> {
        if self.state != ActionFreezeState::Action.into()
            && self.state != ActionFreezeState::BeginAction.into()
        {
            return None;
        }

        self.action_chain.last().map(|t| t.action_index)
    }

    fn end_action(&mut self) {
        self.state = ActionFreezeState::ActionCleanup.into();
    }

    fn advance_team_action(&mut self) {
        self.action_chain.pop();

        if self.action_chain.is_empty() {
            self.state = ActionFreezeState::FadeOut.into();
        } else {
            self.state = ActionFreezeState::BeginAction.into();
        }

        self.state_start_time = self.active_time;
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
        self.should_defrost = false;
        self.active_time += 1;

        if self.state != self.previous_state {
            self.previous_state = self.state;
            // prevent state changes if we just changed states
            return;
        }

        let state_elapsed_time = self.active_time - self.state_start_time;

        match self.state {
            TimeFreezeState::Thawed => {}
            TimeFreezeState::Action(action_freeze_state) => {
                if state_elapsed_time >= action_freeze_state.duration() {
                    self.state = action_freeze_state.next_state();
                    self.state_start_time = self.active_time;

                    if self.skipping_intros && self.state == ActionFreezeState::FadeIn.into() {
                        // skip straight to the action
                        self.state = ActionFreezeState::BeginAction.into();
                    }
                }
            }
            TimeFreezeState::Animation(duration) => {
                if state_elapsed_time >= duration {
                    self.state = TimeFreezeState::Thawed;
                    self.state_start_time = self.active_time;
                }
            }
        };

        // we only care about state differences from updates, not from expiration
        self.previous_state = self.state;
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
        if !simulation.time_freeze_tracker.skipping_intros {
            const FADE_COLOR: Color = Color::new(0.0, 0.0, 0.0, 0.3);

            let fade_alpha = simulation.time_freeze_tracker.fade_alpha();
            let fade_color = FADE_COLOR.multiply_alpha(fade_alpha);
            resources.battle_fade_color.set(fade_color);
        }

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
        if !simulation.time_freeze_tracker.time_is_frozen()
            && let Some(callback) = simulation.time_freeze_tracker.advance_queue()
        {
            simulation.pending_callbacks.push(callback);
            simulation.call_pending_callbacks(game_io, resources);
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
            ActionFreezeState::BeginAction => {
                Self::begin_action(simulation);
            }
            ActionFreezeState::Action => {
                if let Some(index) = time_freeze_tracker.active_action_index() {
                    // detect action end
                    if !simulation.actions.contains_key(index) {
                        // action completed, update tracking
                        time_freeze_tracker.end_action();
                    }
                }
            }
            ActionFreezeState::ActionCleanup => {
                if let Some(action_index) = simulation.time_freeze_tracker.active_action_index() {
                    // try deleting, just in case we only timed out
                    Action::delete_multi(game_io, resources, simulation, true, [action_index]);
                }

                if let Some(entity_backup) = simulation.time_freeze_tracker.take_entity_backup() {
                    entity_backup.restore(simulation);
                }

                // delete anything that hit 0 hp during our action
                simulation.mark_deleted(game_io, resources);
            }
            ActionFreezeState::PollEntityAction => {
                time_freeze_tracker.advance_team_action();
            }
            _ => {}
        }

        // increment the time
        let time_freeze_tracker = &mut simulation.time_freeze_tracker;
        time_freeze_tracker.increment_time();

        // skip text animation if the action was deleted
        if let TimeFreezeState::Action(action_freeze_state) = time_freeze_tracker.state {
            let action_index = time_freeze_tracker.active_action_index();
            let action_deleted = action_index.is_some_and(|i| simulation.actions.contains_key(i));

            if action_deleted && action_freeze_state.displaying_top_summary() {
                time_freeze_tracker.end_action();
            }
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
        let Some(action_index) = simulation.time_freeze_tracker.active_action_index() else {
            // action deleted?
            simulation.time_freeze_tracker.end_action();
            return;
        };

        let Some(action) = simulation.actions.get(action_index) else {
            // action deleted?
            simulation.time_freeze_tracker.end_action();
            return;
        };

        let entity_id = action.entity;

        // unfreeze our entity
        let Some(entity_backup) =
            TimeFreezeEntityBackup::backup_and_prepare(simulation, entity_id, action_index)
        else {
            // entity erased?
            simulation.time_freeze_tracker.end_action();
            log::error!("Time freeze entity erased, yet action still exists?");
            return;
        };

        simulation
            .time_freeze_tracker
            .set_entity_backup(entity_backup);
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

        let tracked_action = self.action_chain.last().cloned().unwrap();

        let Some(action) = simulation.actions.get(tracked_action.action_index) else {
            return;
        };

        let state_elapsed_time = self.active_time - self.state_start_time;
        let summary_scale_y = action_state.summary_scale(state_elapsed_time);

        // resolve where we should draw
        let mut position = Self::team_ui_position(simulation, tracked_action.team);

        // draw action name and damage
        let default_card_props = CardProperties::default();
        let card_props =
            if action.properties.conceal && simulation.local_team != tracked_action.team {
                &default_card_props
            } else {
                &action.properties
            };

        let action_summary_scale = Vec2::new(1.0, summary_scale_y);
        card_props.draw_summary(game_io, sprite_queue, position, action_summary_scale, true);

        // drawing bar
        if self.can_processing_action_counter() {
            let text_style = TextStyle::new(game_io, FontName::Thick);
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
            sprite.set_color(BATTLE_INFO_SHADOW_COLOR);
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
        let Some(tracked_action) = self.action_chain.iter().rev().nth(1).cloned() else {
            return;
        };

        let state_elapsed_time = self.active_time - self.state_start_time;
        let summary_scale_y = ActionFreezeState::Countered.summary_scale(state_elapsed_time);
        let position = Self::team_ui_position(simulation, tracked_action.team);

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
        if let Some(action) = simulation.actions.get(tracked_action.action_index) {
            let default_card_props = CardProperties::default();
            let card_props =
                if action.properties.conceal && simulation.local_team != tracked_action.team {
                    &default_card_props
                } else {
                    &action.properties
                };

            let scale = Vec2::new(1.0, summary_scale_y);
            card_props.draw_summary(game_io, sprite_queue, position, scale, true);
        }
    }

    pub fn draw_intro_aura(
        &self,
        simulation: &BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
        blind_filter: Option<Team>,
    ) {
        if !matches!(
            self.state,
            TimeFreezeState::Action(
                ActionFreezeState::DisplaySummary | ActionFreezeState::Counterable,
            )
        ) {
            return;
        }

        let Some(tracker) = self.action_chain.last().cloned() else {
            return;
        };

        if blind_filter.is_some_and(|team| team != tracker.team) {
            return;
        }

        let entities = &simulation.entities;
        let Ok(mut entity_query) = entities.query_one::<&mut Entity>(tracker.entity.into()) else {
            return;
        };

        let Some(entity) = entity_query.get() else {
            return;
        };

        let Some(sprite_tree) = simulation.sprite_trees.get(entity.sprite_tree_index) else {
            return;
        };

        let mut elapsed = self.active_time - self.state_start_time;

        if self.state == TimeFreezeState::Action(ActionFreezeState::Counterable) {
            elapsed += ActionFreezeState::DisplaySummary.duration();
        }

        let elapsed = elapsed as f32;

        let mut sprite_tree = sprite_tree.clone();
        let progress = elapsed / 40.0;

        if !(0.0..1.0).contains(&progress) {
            return;
        }

        let perspective_flipped = simulation.local_team.flips_perspective();

        // start at tile center
        let field = &simulation.field;
        let mut position = field.calc_tile_center((entity.x, entity.y), perspective_flipped);

        // apply the entity offset
        position += entity.corrected_offset(perspective_flipped);

        // apply elevation
        position.y -= entity.elevation;

        let alpha = (1.0 - progress) * 0.75;
        let color = Color::from_rgb_u8s(32, 32, 64).multiply_alpha(alpha);

        for sprite_node in sprite_tree.values_mut() {
            sprite_node.set_shader_effect(SpriteShaderEffect::Grayscale);
            sprite_node.set_color_mode(SpriteColorMode::Multiply);
            sprite_node.set_color(color);
        }

        // increase by a % of the original size every frame
        let root_node = sprite_tree.root_mut();
        let scale = root_node.scale();
        root_node.set_scale(scale + elapsed * 0.015);

        let flipped = perspective_flipped ^ entity.flipped();
        sprite_tree.draw_with_offset(sprite_queue, position, flipped);
    }
}
