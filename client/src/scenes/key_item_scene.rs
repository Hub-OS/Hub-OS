use crate::bindable::SpriteColorMode;
use crate::render::ui::{
    FontName, SceneTitle, ScrollTracker, ScrollableFrame, SubSceneFrame, TextStyle, Textbox,
    TextboxDoorstop, TextboxDoorstopKey, UiInputTracker,
};
use crate::render::{Animator, Background, Camera, SpriteColorQueue};
use crate::resources::{Globals, Input, ResourcePaths, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;
use indexmap::IndexMap;
use packets::structures::{Inventory, ItemDefinition, TextureAnimPathPair};

const LINE_HEIGHT: f32 = 16.0;

pub struct KeyItemsScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    key_items: Vec<ItemDefinition>,
    cursor_left_start: Vec2,
    cursor_right_start: Vec2,
    text_offset: Vec2,
    ui_input_tracker: UiInputTracker,
    v_scroll_tracker: ScrollTracker,
    h_scroll_tracker: ScrollTracker,
    items_frame: ScrollableFrame,
    textbox: Textbox,
    doorstop_key: Option<TextboxDoorstopKey>,
    next_scene: NextScene,
}

impl KeyItemsScene {
    pub fn new(
        game_io: &GameIO,
        item_registry: &IndexMap<String, ItemDefinition>,
        inventory: &Inventory,
    ) -> Self {
        // key items
        let key_items: Vec<_> = inventory
            .items()
            .flat_map(|(item_id, count)| Some((item_registry.get(item_id)?, *count)))
            .filter(|(item_definition, _)| !item_definition.consumable)
            .flat_map(|(item_definition, count)| std::iter::repeat_n(item_definition, count))
            .cloned()
            .collect();

        // layout
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let mut ui_animator = Animator::load_new(assets, ResourcePaths::KEY_ITEMS_UI_ANIMATION);
        ui_animator.set_state("DEFAULT");

        // cursors
        let cursor_left_start = ui_animator.point_or_zero("CURSOR_LEFT");
        let cursor_right_start = ui_animator.point_or_zero("CURSOR_RIGHT");
        let text_offset = ui_animator.point_or_zero("TEXT_OFFSET");

        // scrollable frame
        let frame_bounds = Rect::from_corners(
            ui_animator.point_or_zero("FRAME_START"),
            ui_animator.point_or_zero("FRAME_END"),
        );

        let scrollable_frame = ScrollableFrame::new(game_io, frame_bounds);

        // scroll trackers
        let mut v_scroll_tracker = ScrollTracker::new(game_io, 4);
        let mut h_scroll_tracker = ScrollTracker::new(game_io, 2).with_wrap(true);

        // add 1 to total items to round division up instead of down
        v_scroll_tracker.set_total_items(key_items.len().div_ceil(2));
        h_scroll_tracker.set_total_items(if key_items.len() > 1 {
            2
        } else {
            key_items.len()
        });

        v_scroll_tracker.define_cursor(cursor_left_start, LINE_HEIGHT);
        v_scroll_tracker.define_scrollbar(
            scrollable_frame.scroll_start(),
            scrollable_frame.scroll_end(),
        );

        // textbox
        let mut textbox = Textbox::new_navigation(game_io)
            .begin_open()
            .with_text_animation_enabled(false);

        textbox.set_next_avatar(
            game_io,
            assets,
            Some(&TextureAnimPathPair {
                texture: ResourcePaths::KEY_ITEMS_MUG.into(),
                animation: ResourcePaths::BLANK.into(),
            }),
        );

        let (mut interface, doorstop_key) = TextboxDoorstop::new();

        interface = interface.with_string(if let Some(item) = key_items.first() {
            item.description.clone()
        } else {
            globals.translate("key-items-no-items-description")
        });

        textbox.push_interface(interface);

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            scene_title: SceneTitle::new(game_io, "key-items-scene-title"),
            frame: SubSceneFrame::new(game_io).with_everything(true),
            key_items,
            cursor_left_start,
            cursor_right_start,
            text_offset,
            ui_input_tracker: UiInputTracker::new(),
            v_scroll_tracker,
            h_scroll_tracker,
            items_frame: scrollable_frame,
            textbox,
            doorstop_key: Some(doorstop_key),
            next_scene: NextScene::None,
        }
    }
}

impl Scene for KeyItemsScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.textbox.update(game_io);

        if game_io.is_in_transition() {
            return;
        }

        self.ui_input_tracker.update(game_io);

        if self.ui_input_tracker.pulsed(Input::Cancel) {
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            let transition = crate::transitions::new_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
            return;
        }

        // update v_scroll_tracker
        let prev_v_index = self.v_scroll_tracker.selected_index();

        self.v_scroll_tracker
            .handle_vertical_input(&self.ui_input_tracker);

        let v_index = self.v_scroll_tracker.selected_index();
        let v_index_changed = prev_v_index != v_index;

        if v_index_changed {
            // update item count for h_scroll_tracker
            let item_count = if v_index + 1 < self.v_scroll_tracker.total_items() {
                2
            } else {
                2 - self.key_items.len() % 2
            };

            self.h_scroll_tracker.set_total_items(item_count);
        }

        // update h_scroll_tracker
        let prev_h_index = self.h_scroll_tracker.selected_index();

        self.h_scroll_tracker
            .handle_horizontal_input(&self.ui_input_tracker);

        let h_index = self.h_scroll_tracker.selected_index();
        let h_index_changed = prev_h_index != h_index;

        if v_index_changed || h_index_changed {
            // sfx
            let globals = Globals::from_resources(game_io);
            globals.audio.play_sound(&globals.sfx.cursor_move);

            // update textbox
            let item = &self.key_items[v_index * 2 + h_index];
            let (mut interface, doorstop_key) = TextboxDoorstop::new();
            interface = interface.with_str(&item.description);
            self.doorstop_key = Some(doorstop_key);

            self.textbox.push_interface(interface);
        }

        // update cursor column
        let cursor_start = if h_index == 0 {
            self.cursor_left_start
        } else {
            self.cursor_right_start
        };

        self.v_scroll_tracker
            .define_cursor(cursor_start, LINE_HEIGHT);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw item frame
        self.items_frame.draw(game_io, &mut sprite_queue);

        // draw items
        let mut text_style = TextStyle::new(game_io, FontName::Thin);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;

        if !self.key_items.is_empty() {
            let mut item_range = self.v_scroll_tracker.view_range();
            item_range.start *= 2;
            item_range.end *= 2;
            item_range.end = item_range.end.min(self.key_items.len());

            let mut y_offset = 0.0;

            for (i, item) in self.key_items[item_range].iter().enumerate() {
                let mut position = if i % 2 == 0 {
                    self.cursor_left_start
                } else {
                    self.cursor_right_start
                };

                position += self.text_offset;
                position.y += y_offset;

                text_style.bounds.set_position(position);
                text_style.draw(game_io, &mut sprite_queue, &item.name);

                if i % 2 == 1 {
                    y_offset += LINE_HEIGHT;
                }
            }
        } else {
            // default for no key items
            let position = self.cursor_left_start + self.text_offset;

            text_style.bounds.set_position(position);
            text_style.draw(game_io, &mut sprite_queue, "None");
        }

        // draw scrollbar and cursor
        self.v_scroll_tracker.draw_cursor(&mut sprite_queue);
        self.v_scroll_tracker.draw_scrollbar(&mut sprite_queue);

        // draw frame
        self.frame.draw(&mut sprite_queue);
        self.scene_title.draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
