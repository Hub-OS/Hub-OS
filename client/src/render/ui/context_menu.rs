use crate::render::ui::*;
use crate::render::*;
use crate::{bindable::GenerationalIndex, resources::*};
use framework::prelude::*;
use std::collections::VecDeque;

pub struct ContextMenu<T: Copy + 'static> {
    ui_layout: UiLayout,
    ui_sender: flume::Sender<T>,
    ui_receiver: flume::Receiver<T>,
    cursor_sprite: Sprite,
    cursor_animator: Animator,
    body_index: GenerationalIndex,
    arrow_sprite: Sprite,
    arrow_visible: bool,
    first_option_index: Option<GenerationalIndex>,
    open: bool,
    phantom_data: std::marker::PhantomData<T>,
}

impl<T: Copy + 'static> ContextMenu<T> {
    pub fn new_translated(game_io: &GameIO, label_translation_key: &str, position: Vec2) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();

        Self::new(game_io, globals.translate(label_translation_key), position)
    }

    pub fn new(game_io: &GameIO, label: String, position: Vec2) -> Self {
        let bounds = Rect::new(position.x, position.y, RESOLUTION_F.x, RESOLUTION_F.y);

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // cursor
        let cursor_texture = assets.texture(game_io, ResourcePaths::SELECT_CURSOR);
        let cursor_sprite = Sprite::new(game_io, cursor_texture);

        let mut cursor_animator =
            Animator::load_new(assets, ResourcePaths::SELECT_CURSOR_ANIMATION);
        cursor_animator.set_state("DEFAULT");
        cursor_animator.set_loop_mode(AnimatorLoopMode::Loop);

        // resources
        let texture = assets.texture(game_io, ResourcePaths::UI_NINE_PATCHES);
        let mut animator = Animator::load_new(assets, ResourcePaths::UI_NINE_PATCHES_ANIMATION);

        // arrow sprite
        let mut arrow_sprite = Sprite::new(game_io, texture.clone());
        animator.set_state("CONTEXT_RIGHT_ARROW");
        animator.apply(&mut arrow_sprite);

        // styles
        let label_9patch = build_9patch!(game_io, texture.clone(), &animator, "CONTEXT_LABEL");
        let body_9patch = build_9patch!(game_io, texture, &animator, "CONTEXT_BODY");

        let label_style = UiStyle {
            nine_patch: Some(label_9patch),
            padding_right: arrow_sprite.origin().x,
            ..Default::default()
        };

        let body_style = UiStyle {
            flex_direction: FlexDirection::Column,
            nine_patch: Some(body_9patch),
            padding_right: 1.0,
            padding_bottom: 3.0,
            ..Default::default()
        };

        let mut body_index = None;

        // ui
        let ui_layout = UiLayout::new_vertical(
            bounds,
            vec![
                UiLayoutNode::new(Text::new(game_io, FontName::Context).with_string(label))
                    .with_style(label_style),
                UiLayoutNode::new_container()
                    .with_handle(&mut body_index)
                    .with_style(body_style),
            ],
        )
        .with_wrapped_selection()
        .with_style(UiStyle {
            flex_direction: FlexDirection::Column,
            min_width: Dimension::Auto,
            ..Default::default()
        });

        let (ui_sender, ui_receiver) = flume::unbounded();

        Self {
            ui_layout,
            ui_sender,
            ui_receiver,
            cursor_sprite,
            cursor_animator,
            arrow_sprite,
            arrow_visible: false,
            body_index: body_index.unwrap(),
            first_option_index: None,
            open: false,
            phantom_data: Default::default(),
        }
    }

    pub fn with_translated_options(mut self, game_io: &GameIO, options: &[(&str, T)]) -> Self {
        self.set_and_translate_options(game_io, options);
        self
    }

    pub fn with_arrow(mut self, visible: bool) -> Self {
        self.arrow_visible = visible;
        self
    }

    pub fn is_open(&self) -> bool {
        self.open
    }

    pub fn open(&mut self) {
        self.open = true;

        if let Some(index) = self.first_option_index {
            self.ui_layout.set_focused_index(Some(index));
        }
    }

    pub fn close(&mut self) {
        self.open = false;
    }

    pub fn recalculate_layout(&mut self, game_io: &GameIO) {
        self.ui_layout.recalculate_layout(game_io);
    }

    pub fn bounds(&self) -> Rect {
        self.ui_layout.bounds()
    }

    pub fn set_and_translate_options(&mut self, game_io: &GameIO, options: &[(&str, T)]) {
        let globals = game_io.resource::<Globals>().unwrap();

        self.set_options(
            game_io,
            options
                .iter()
                .map(|(label_key, option)| (globals.translate(label_key), *option))
                .collect(),
        );
    }

    pub fn set_options(&mut self, game_io: &GameIO, options: Vec<(String, T)>) {
        let option_style = UiStyle {
            margin_top: LengthPercentageAuto::Points(3.0),
            ..Default::default()
        };

        let mut nodes: VecDeque<_> = options
            .into_iter()
            .map(|(label, option)| {
                let sender = self.ui_sender.clone();

                UiLayoutNode::new(
                    UiButton::new(
                        Text::new(game_io, FontName::Thin)
                            .with_string(label)
                            .with_shadow_color(CONTEXT_TEXT_SHADOW_COLOR),
                    )
                    .on_activate(move || {
                        sender.send(option).unwrap();
                    }),
                )
                .with_style(option_style.clone())
            })
            .collect();

        if let Some(first_node) = nodes.pop_front() {
            let node = first_node.with_handle(&mut self.first_option_index);
            nodes.push_front(node);
        }

        self.ui_layout.set_children(self.body_index, nodes.into());

        if let Some(index) = self.first_option_index {
            self.ui_layout.set_focused_index(Some(index));
        }
    }

    pub fn update(&mut self, game_io: &mut GameIO, ui_input_tracker: &UiInputTracker) -> Option<T> {
        if !self.open {
            return None;
        }

        self.cursor_animator.update();
        self.cursor_animator.apply(&mut self.cursor_sprite);

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            // closed
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            self.open = false;
            return None;
        }

        self.ui_layout.update(game_io, ui_input_tracker);

        self.ui_receiver.try_recv().ok()
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.ui_layout.set_position(position);
    }

    pub fn set_top_center(&mut self, mut position: Vec2) {
        position.x -= self.bounds().width * 0.5;
        self.ui_layout.set_position(position);
    }

    pub fn set_selected_index(&mut self, index: usize) {
        let index = self.ui_layout.get_child_index(self.body_index, index);

        if let Some(index) = index {
            self.ui_layout.set_focused_index(Some(index));
        }
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if !self.open {
            return;
        }

        self.ui_layout.draw(game_io, sprite_queue);

        if let Some(index) = self.ui_layout.focused_index() {
            let bounds = self.ui_layout.get_bounds(index).unwrap();

            let cursor_size = self.cursor_sprite.size();

            let mut cursor_position = bounds.center_left();
            cursor_position.x -= cursor_size.x;
            cursor_position.y -= cursor_size.y * 0.5 - 2.0;
            self.cursor_sprite.set_position(cursor_position);

            sprite_queue.draw_sprite(&self.cursor_sprite);
        }

        if self.arrow_visible {
            let bounds = self.ui_layout.get_bounds(TreeIndex::tree_root()).unwrap();
            self.arrow_sprite.set_position(bounds.top_right());
            sprite_queue.draw_sprite(&self.arrow_sprite);
        }
    }
}
