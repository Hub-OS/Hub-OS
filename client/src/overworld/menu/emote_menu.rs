use super::Menu;
use crate::overworld::{idle_str, run_str, walk_str, OverworldArea, OverworldEvent};
use crate::render::ui::{
    build_9patch, FontStyle, NinePatch, ScrollTracker, TextInput, TextStyle, Textbox,
    UiInputTracker, UiLayout, UiLayoutNode, UiStyle,
};
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;
use packets::structures::{Direction, Input};

#[derive(Clone, PartialEq, Eq)]
enum EmoteCategory {
    Icon,
    ActorAnimation,
}

pub struct EmoteMenu {
    search_sprite: Sprite,
    highlight_sprite: Sprite,
    actor_animation_sprite: Sprite,
    emote_sprite: Sprite,
    emote_animator: Animator,
    start_point: Vec2,
    next_point: Vec2,
    emotes: Vec<(String, EmoteCategory)>,
    filtered_emotes: Vec<(String, EmoteCategory)>,
    event_sender: flume::Sender<String>,
    event_receiver: flume::Receiver<String>,
    ui_input_tracker: UiInputTracker,
    scroll_tracker: ScrollTracker,
    search_box_9patch: NinePatch,
    search_box_bounds: Rect,
    search_layout: Option<UiLayout>,
    open: bool,
}

impl EmoteMenu {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // layout
        let mut layout_animator =
            Animator::load_new(assets, ResourcePaths::OVERWORLD_EMOTE_UI_ANIMATION);
        layout_animator.set_state("DEFAULT");
        let start_point = layout_animator.point("START").unwrap_or_default();
        let search_box_bounds = Rect::from_corners(
            layout_animator
                .point("SEARCH_BOX_START")
                .unwrap_or_default(),
            layout_animator.point("SEARCH_BOX_END").unwrap_or_default(),
        );
        let next_point = layout_animator.point("NEXT").unwrap_or_default();
        let view_size = layout_animator.point("VIEW_SIZE").unwrap_or_default().x as usize;

        // search sprite
        let mut search_sprite = assets.new_sprite(game_io, ResourcePaths::OVERWORLD_EMOTE_UI);
        layout_animator.set_state("SEARCH");
        layout_animator.apply(&mut search_sprite);

        // search box 9patch
        let search_box_9patch = build_9patch!(
            game_io,
            search_sprite.texture().clone(),
            &layout_animator,
            "SEARCH_BOX"
        );

        // highlight sprite
        let mut highlight_sprite = search_sprite.clone();
        layout_animator.set_state("HIGHLIGHT");
        layout_animator.apply(&mut highlight_sprite);

        let mut actor_animation_sprite = search_sprite.clone();
        layout_animator.set_state("ACTOR_ANIMATION");
        layout_animator.apply(&mut actor_animation_sprite);

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            search_sprite,
            highlight_sprite,
            actor_animation_sprite,
            emote_sprite: assets.new_sprite(game_io, ResourcePaths::OVERWORLD_EMOTES),
            emote_animator: Animator::new(),
            start_point,
            next_point,
            emotes: Vec::new(),
            filtered_emotes: Vec::new(),
            event_sender,
            event_receiver,
            ui_input_tracker: UiInputTracker::new(),
            scroll_tracker: ScrollTracker::new(game_io, view_size),
            search_box_9patch,
            search_box_bounds,
            search_layout: None,
            open: false,
        }
    }

    fn open_search(&mut self, game_io: &GameIO) {
        let event_sender = self.event_sender.clone();

        self.search_layout = Some(UiLayout::new_horizontal(
            self.search_box_bounds,
            vec![UiLayoutNode::new(
                TextInput::new(game_io, FontStyle::ThinSmall)
                    .with_color(Color::WHITE)
                    .with_silent(true)
                    .with_active(true)
                    .on_change(move |value| event_sender.send(value.to_string()).unwrap()),
            )
            .with_style(UiStyle {
                flex_grow: 1.0,
                flex_shrink: 1.0,
                nine_patch: Some(self.search_box_9patch.clone()),
                // align_self: Some(AlignSelf::Center),
                ..Default::default()
            })],
        ));
    }
}

impl Menu for EmoteMenu {
    fn is_fullscreen(&self) -> bool {
        false
    }

    fn is_open(&self) -> bool {
        self.open
    }

    fn open(&mut self, _game_io: &mut GameIO, area: &mut OverworldArea) {
        self.open = true;

        // clone emotes
        self.emote_sprite = area.emote_sprite.clone();
        self.emote_animator = area.emote_animator.clone();

        // resolve options
        // emoticons
        let emoticon_state_iter = self.emote_animator.iter_states();
        let emoticon_iter =
            emoticon_state_iter.map(|(name, _)| (name.to_uppercase(), EmoteCategory::Icon));

        // actor animations
        let default_states: Vec<_> = [
            Direction::Up,
            Direction::Left,
            Direction::Down,
            Direction::Right,
            Direction::UpLeft,
            Direction::UpRight,
            Direction::DownLeft,
            Direction::DownRight,
        ]
        .into_iter()
        .flat_map(|direction| [idle_str(direction), walk_str(direction), run_str(direction)])
        .collect();

        let player_animator = area
            .entities
            .query_one_mut::<&Animator>(area.player_data.entity)
            .unwrap();
        let player_state_iter = player_animator.iter_states();
        let actor_animation_iter = player_state_iter
            .map(|(name, _)| (name.to_uppercase(), EmoteCategory::ActorAnimation))
            .filter(|(name, _)| {
                !default_states.contains(&name.as_str()) && !self.emote_animator.has_state(name)
            });

        // merge
        self.emotes = emoticon_iter.chain(actor_animation_iter).collect();
        self.filtered_emotes = self.emotes.clone();
        self.scroll_tracker.set_total_items(self.emotes.len() + 1);
        self.scroll_tracker.set_selected_index(0);
    }

    fn update(&mut self, _game_io: &mut GameIO, _area: &mut OverworldArea) {
        if let Ok(mut search_text) = self.event_receiver.try_recv() {
            search_text.make_ascii_uppercase();

            let state_iter = self.emotes.iter();
            self.filtered_emotes = state_iter
                .filter(|(name, _)| name.contains(&search_text))
                .cloned()
                .collect();

            self.scroll_tracker
                .set_total_items(self.filtered_emotes.len() + 1);

            self.search_layout = None;
        }
    }

    fn handle_input(
        &mut self,
        game_io: &mut GameIO,
        area: &mut OverworldArea,
        _textbox: &mut Textbox,
    ) {
        self.ui_input_tracker.update(game_io);

        if let Some(layout) = &mut self.search_layout {
            layout.update(game_io, &self.ui_input_tracker);
            return;
        }

        if self.ui_input_tracker.is_active(Input::Confirm) {
            if self.scroll_tracker.selected_index() == 0 {
                self.open_search(game_io);
            } else {
                let (emote, _) =
                    self.filtered_emotes[self.scroll_tracker.selected_index() - 1].clone();

                area.event_sender
                    .send(OverworldEvent::EmoteSelected(emote))
                    .unwrap();

                self.open = false;
            }
            return;
        }

        if self.ui_input_tracker.is_active(Input::Cancel) {
            self.open = false;
            return;
        }

        self.scroll_tracker
            .handle_horizontal_input(&self.ui_input_tracker);
        self.scroll_tracker
            .handle_vertical_input(&self.ui_input_tracker);
    }

    fn draw(
        &mut self,
        game_io: &GameIO,
        _render_pass: &mut RenderPass,
        sprite_queue: &mut SpriteColorQueue,
        area: &OverworldArea,
    ) {
        let selected_index = self.scroll_tracker.selected_index();
        let mut offset = self.start_point;

        for index in self.scroll_tracker.view_range() {
            // draw highlight
            if index == selected_index {
                self.highlight_sprite.set_position(offset);
                sprite_queue.draw_sprite(&self.highlight_sprite);
            }

            if index == 0 {
                // draw search tool
                self.search_sprite.set_position(offset);
                sprite_queue.draw_sprite(&self.search_sprite);
            } else {
                // draw emote
                let (state, category) = &self.filtered_emotes[index - 1];

                match category {
                    EmoteCategory::Icon => {
                        self.emote_animator.set_state(state);

                        // scale
                        let scale = self.emote_animator.point("SCALE").unwrap_or(Vec2::ONE);
                        self.emote_sprite.set_scale(scale);

                        // apply animation
                        self.emote_animator.sync_time(area.world_time);
                        self.emote_animator.apply(&mut self.emote_sprite);

                        // draw
                        self.emote_sprite.set_position(offset);
                        sprite_queue.draw_sprite(&self.emote_sprite);
                    }
                    EmoteCategory::ActorAnimation => {
                        // draw actor animation placeholder
                        self.actor_animation_sprite.set_position(offset);
                        sprite_queue.draw_sprite(&self.actor_animation_sprite);
                    }
                }
            }

            offset += self.next_point;
        }

        if let Some(layout) = &mut self.search_layout {
            // draw search
            layout.draw(game_io, sprite_queue);
        } else {
            // draw selection name
            let selection_name = if selected_index == 0 {
                "SEARCH"
            } else {
                self.filtered_emotes[selected_index - 1].0.as_str()
            };

            let text_bounds = self.search_box_9patch.body_bounds(self.search_box_bounds);

            TextStyle::new(game_io, FontStyle::ThinSmall)
                .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
                .with_bounds(text_bounds)
                .draw(game_io, sprite_queue, selection_name);
        }
    }
}
