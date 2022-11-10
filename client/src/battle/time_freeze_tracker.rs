use super::{BattleAnimator, BattleSimulation, Entity, StatusDirector};
use crate::bindable::{EntityID, Team};
use crate::ease::inverse_lerp;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths, RESOLUTION_F};
use framework::prelude::{Color, GameIO};

const FADE_DURATION: FrameTime = 10;
const COUNTER_DURATION: FrameTime = 60;
const MAX_ACTION_DURATION: FrameTime = 60 * 15;

#[derive(Default, Clone, PartialEq, Eq)]
enum TimeFreezeState {
    #[default]
    Thawed,
    FadeIn,
    Counterable,
    Action,
    FadeOut,
}

#[derive(Default, Clone)]
pub struct TimeFreezeTracker {
    chain: Vec<(Team, generational_arena::Index)>,
    active_time: FrameTime,
    state_start_time: FrameTime,
    state: TimeFreezeState,
    character_backup: Option<(
        EntityID,
        Option<generational_arena::Index>,
        BattleAnimator,
        StatusDirector,
    )>,
}

impl TimeFreezeTracker {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn time_is_frozen(&self) -> bool {
        self.state != TimeFreezeState::Thawed
    }

    pub fn fade_alpha(&self) -> f32 {
        let state_elapsed_time = self.active_time - self.state_start_time;

        match self.state {
            TimeFreezeState::Thawed => 0.0,
            TimeFreezeState::FadeIn => inverse_lerp!(0, FADE_DURATION, state_elapsed_time),
            TimeFreezeState::Counterable | TimeFreezeState::Action => 1.0,
            TimeFreezeState::FadeOut => inverse_lerp!(FADE_DURATION, 0, state_elapsed_time),
        }
    }

    pub fn set_team_action(&mut self, team: Team, action_index: generational_arena::Index) {
        if let Some(index) = self.chain.iter().position(|(t, _)| *t == team) {
            self.chain.remove(index);
        }

        if !self.time_is_frozen() {
            self.active_time = 0;
            self.state = TimeFreezeState::FadeIn;
        }

        // set the state start time to allow other players to counter, as well as initialize
        self.state_start_time = self.active_time;

        self.chain.push((team, action_index));
    }

    pub fn increment_time(&mut self) {
        self.active_time += 1;

        let state_elapsed_time = self.active_time - self.state_start_time;

        match self.state {
            TimeFreezeState::Thawed | TimeFreezeState::Action => {}
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
        };
    }

    pub fn last_team(&self) -> Option<Team> {
        self.chain.last().map(|(team, _)| team).cloned()
    }

    pub fn can_counter(&self) -> bool {
        self.state == TimeFreezeState::Counterable
    }

    pub fn freeze_should_start(&self) -> bool {
        self.state == TimeFreezeState::FadeIn && self.active_time == 0
    }

    pub fn action_should_start(&self) -> bool {
        self.state == TimeFreezeState::Action && self.active_time == self.state_start_time
    }

    pub fn active_action(&self) -> Option<generational_arena::Index> {
        if self.state != TimeFreezeState::Action {
            return None;
        }

        self.chain.last().map(|(_, index)| index).cloned()
    }

    pub fn end_action(&mut self) {
        self.active_time = self.state_start_time + MAX_ACTION_DURATION;
    }

    pub fn advance_action(&mut self) {
        self.chain.pop();

        if self.chain.is_empty() {
            self.state = TimeFreezeState::FadeOut;
        } else {
            self.state = TimeFreezeState::Action;
        }

        self.state_start_time = self.active_time;
    }

    pub fn action_out_of_time(&self) -> bool {
        self.state == TimeFreezeState::Action
            && self.active_time - self.state_start_time >= MAX_ACTION_DURATION
    }

    pub fn back_up_character(
        &mut self,
        id: EntityID,
        action_index: Option<generational_arena::Index>,
        animator: BattleAnimator,
        status_director: StatusDirector,
    ) {
        self.character_backup = Some((id, action_index, animator, status_director));
    }

    pub fn take_character_backup(
        &mut self,
    ) -> Option<(
        EntityID,
        Option<generational_arena::Index>,
        BattleAnimator,
        StatusDirector,
    )> {
        self.character_backup.take()
    }

    pub fn draw_ui(
        &self,
        game_io: &GameIO<Globals>,
        simulation: &BattleSimulation,
        sprite_queue: &mut SpriteColorQueue,
    ) {
        const MARGIN_TOP: f32 = 38.0;
        const BAR_WIDTH: f32 = 80.0;

        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.bounds.y = MARGIN_TOP;

        if !self.can_counter() {
            return;
        }

        let (team, index) = self.chain.last().cloned().unwrap();

        let Some(action) = simulation.card_actions.get(index) else {
            return;
        };

        let same_team = {
            let id = simulation.local_player_id.into();
            let entities = &simulation.entities;

            entities
                .query_one::<&Entity>(id)
                .ok()
                .and_then(|mut query| query.get().map(|entity| team == entity.team))
                .unwrap_or_default()
        };

        let text_center_x = if same_team {
            RESOLUTION_F.x * 0.25
        } else {
            RESOLUTION_F.x * 0.75
        };

        let card_props = &action.properties;

        let name_text = &card_props.short_name;
        let damage_text = if card_props.damage == 0 {
            String::new()
        } else {
            format!("{}", card_props.damage)
        };

        // measure text
        let name_width = text_style.measure(name_text).size.x;
        text_style.font_style = FontStyle::GradientGold;
        let damage_width = text_style.measure(&damage_text).size.x;
        let text_width = name_width + text_style.letter_spacing + damage_width;

        // draw name
        text_style.shadow_color = Color::BLACK;
        text_style.font_style = FontStyle::Thick;
        text_style.bounds.x = text_center_x - text_width / 2.0;
        text_style.draw(game_io, sprite_queue, name_text);

        // draw damage
        text_style.shadow_color = Color::TRANSPARENT;
        text_style.font_style = FontStyle::GradientGold;
        text_style.bounds.x += name_width + text_style.letter_spacing;
        text_style.draw(game_io, sprite_queue, &damage_text);

        // drawing bar
        let elapsed_time = self.active_time - self.state_start_time;
        let width_multiplier = inverse_lerp!(COUNTER_DURATION, 0, elapsed_time);
        let mut position = text_style.bounds.position();
        position.x = text_center_x - BAR_WIDTH / 2.0;
        position.y += text_style.line_height() + text_style.line_spacing;

        let assets = &game_io.globals().assets;
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