use super::Menu;
use crate::overworld::{OverworldArea, OverworldEvent};
use crate::render::ui::{
    build_9patch, FontStyle, NinePatch, ScrollTracker, TextInput, Textbox, UiInputTracker,
    UiLayout, UiLayoutNode, UiStyle,
};
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::*;
use packets::structures::Input;

pub struct EmoteMenu {
    search_sprite: Sprite,
    highlight_sprite: Sprite,
    emote_sprite: Sprite,
    emote_animator: Animator,
    start_point: Vec2,
    next_point: Vec2,
    emote_states: Vec<String>,
    filtered_emote_states: Vec<String>,
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

        // events
        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            search_sprite,
            highlight_sprite,
            emote_sprite: assets.new_sprite(game_io, ResourcePaths::OVERWORLD_EMOTES),
            emote_animator: Animator::new(),
            start_point,
            next_point,
            emote_states: Vec::new(),
            filtered_emote_states: Vec::new(),
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
                TextInput::new(game_io, FontStyle::Small)
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

        // reset options
        let state_iter = self.emote_animator.iter_states();
        self.emote_states = state_iter.map(|(name, _)| name.clone()).collect();
        self.filtered_emote_states = self.emote_states.clone();
        self.scroll_tracker
            .set_total_items(self.emote_states.len() + 1);
        self.scroll_tracker.set_selected_index(0);
    }

    fn update(&mut self, _game_io: &mut GameIO, _area: &mut OverworldArea) {
        if let Ok(search_text) = self.event_receiver.try_recv() {
            let state_iter = self.emote_states.iter();
            self.filtered_emote_states = state_iter
                .filter(|name| name.to_lowercase().contains(&search_text))
                .cloned()
                .collect();

            self.scroll_tracker
                .set_total_items(self.filtered_emote_states.len() + 1);

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
                let emote =
                    self.filtered_emote_states[self.scroll_tracker.selected_index() - 1].clone();

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
        let mut offset = self.start_point;

        for index in self.scroll_tracker.view_range() {
            // draw highlight
            if index == self.scroll_tracker.selected_index() {
                self.highlight_sprite.set_position(offset);
                sprite_queue.draw_sprite(&self.highlight_sprite);
            }

            if index == 0 {
                // draw search tool
                self.search_sprite.set_position(offset);
                sprite_queue.draw_sprite(&self.search_sprite);
            } else {
                // draw emote, set state first
                let state = &self.filtered_emote_states[index - 1];
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

            offset += self.next_point;
        }

        if let Some(layout) = &mut self.search_layout {
            layout.draw(game_io, sprite_queue);
        }
    }
}
