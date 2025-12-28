use crate::bindable::SpriteColorMode;
use crate::render::ui::{FontName, NinePatch, TextStyle, build_9patch};
use crate::render::{Animator, Camera, FrameTime, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, RESOLUTION_F, ResourcePaths};
use crate::saves::Config;
use framework::prelude::*;
use std::ops::Range;

const PRESSED_COLOR: Color = Color::new(1.0, 1.0, 1.0, 0.9);
const RELEASED_COLOR: Color = Color::new(1.0, 1.0, 1.0, 0.3);

const CONFIRM_PRESSED_BRIGHTNESS: f32 = 1.0;
const CONFIRM_RELEASED_BRIGHTNESS: f32 = 0.9;
const CONFIRM_DISABLED_ALPHA: f32 = 0.8;

const BASE: f32 = 64.0 * 3.0;

const LEFT_INPUT_RANGE: Range<usize> = 0..2;
const RIGHT_INPUT_RANGE: Range<usize> = 2..8;

const BUTTON_ORDER: [Button; 8] = [
    Button::LeftTrigger,
    Button::Select,
    Button::RightTrigger,
    Button::Start,
    Button::A,
    Button::B,
    Button::X,
    Button::Y,
];

#[derive(PartialEq, Eq)]
enum ViewState {
    Default,
    Editor,
    Keyboard,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum EditorButton {
    Confirm,
    Reset,
    ScaleDown,
    ScaleUp,
}

impl EditorButton {
    fn state(&self) -> &'static str {
        match self {
            Self::Confirm => "CONFIRM",
            Self::Reset => "RESET",
            Self::ScaleDown => "-",
            Self::ScaleUp => "+",
        }
    }
}

const EDITOR_BUTTON_LIST: &[EditorButton] = &[
    EditorButton::Confirm,
    EditorButton::Reset,
    EditorButton::ScaleDown,
    EditorButton::ScaleUp,
];

/// measured in frames
const TOUCH_WINDOW: FrameTime = 20;
const OPEN_EDITOR_TOUCH_TARGET: u8 = 5;

pub struct VirtualController {
    camera: Camera,
    rectangle: FlatModel,
    button_sprites: Vec<(Button, Sprite)>,
    dpad_sprite: Sprite,
    dpad_dead_zone: Vec2,
    touch_scale: f32,
    touch_positions: Vec<Vec2>,
    last_touch_id: Option<u64>,
    previous_touch_count: usize,
    previous_view_state: ViewState,

    // debug
    empty_touch_count: u8,
    last_empty_touch: FrameTime,

    // moving UI
    editor_camera: Camera,
    last_movement_touch: Option<Vec2>,
    selection_start: Option<Vec2>,
    selected_buttons: Vec<Button>,
    dragging_buttons: bool,
    nine_patch: NinePatch,
    editor_text_style: TextStyle,
    /// (label, offset, hovered)
    editor_buttons: Vec<(EditorButton, Vec2, bool)>,
}

impl VirtualController {
    pub fn new(game_io: &mut GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let sprite = assets.new_sprite(game_io, ResourcePaths::VIRTUAL_CONTROLLER);
        let mut animator = Animator::load_new(assets, ResourcePaths::VIRTUAL_CONTROLLER_ANIMATION);

        // build editor nine patch
        let nine_patch = build_9patch!(game_io, sprite.texture().clone(), &animator, "NINE_PATCH");

        // build editor buttons
        animator.set_state("NINE_PATCH_BODY");
        let editor_text_style = TextStyle::new(game_io, FontName::Thick)
            .with_color(Color::WHITE.multiply_color(animator.point_or_zero("TEXT_BRIGHTNESS").x));

        // build button sprites
        let button_sprites = BUTTON_ORDER
            .into_iter()
            .map(|button| {
                let button_name: &'static str = button.into();
                let mut sprite = sprite.clone();

                animator.set_state(button_name);
                animator.apply(&mut sprite);

                (button, sprite)
            })
            .collect::<Vec<_>>();

        // build dpad
        let mut dpad_sprite = sprite;
        animator.set_state("DPAD");
        animator.apply(&mut dpad_sprite);

        let dpad_dead_zone = animator.point_or_zero("DEAD_ZONE");

        // ui positions
        let mut editor_animator =
            Animator::load_new(assets, ResourcePaths::VIRTUAL_CONTROLLER_EDIT_ANIMATION);
        editor_animator.set_state("DEFAULT");

        let editor_buttons = EDITOR_BUTTON_LIST
            .iter()
            .map(|button| {
                let offset = editor_animator.point_or_zero(button.state());
                (*button, offset, false)
            })
            .collect();

        Self {
            camera: Camera::new(game_io),
            rectangle: FlatModel::new_square_model(game_io),
            button_sprites,
            dpad_sprite,
            dpad_dead_zone,
            touch_scale: 1.0,
            touch_positions: Default::default(),
            last_touch_id: None,
            previous_touch_count: 0,
            previous_view_state: ViewState::Default,
            empty_touch_count: 0,
            last_empty_touch: 0,
            editor_camera: Camera::new(game_io),
            last_movement_touch: None,
            selection_start: None,
            selected_buttons: Default::default(),
            dragging_buttons: false,
            nine_patch,
            editor_text_style,
            editor_buttons,
        }
    }

    fn unnormalize(resolution: Vec2, position: Vec2) -> Vec2 {
        (position * Vec2::new(0.5, -0.5) + 0.5) * resolution
    }

    fn update_touch_positions(&mut self, game_io: &GameIO) {
        let window = game_io.window();
        let scale = window.render_scale();
        let render_offset = window.render_offset();
        let view_size = window.resolution().as_vec2() * scale;

        let touch_iter = game_io.input().touches().iter();

        self.previous_touch_count = self.touch_positions.len();

        self.touch_positions.clear();
        self.touch_positions.extend(
            touch_iter
                .map(|touch| Self::unnormalize(view_size, touch.position) + render_offset)
                .map(|touch| touch * self.touch_scale),
        )
    }

    fn accept_input_update(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource_mut::<Globals>().unwrap();

        let mut pressed_something = false;

        for (button, sprite) in &self.button_sprites {
            let bounds = sprite.bounds();
            let pressed = self
                .touch_positions
                .iter()
                .any(|&position| bounds.contains(position));

            if pressed {
                globals.emulated_input.emulate_button(*button);
                pressed_something = true;
            }
        }

        // update input with dpad
        let dpad_bounds = self.dpad_sprite.bounds();
        let dead_zone = self.dpad_dead_zone;

        for mut position in self.touch_positions.iter().cloned() {
            if !dpad_bounds.contains(position) {
                continue;
            }

            position -= dpad_bounds.center();

            if position.x.abs() > dead_zone.x {
                if position.x > 0.0 {
                    globals.emulated_input.emulate_button(Button::DPadRight)
                } else {
                    globals.emulated_input.emulate_button(Button::DPadLeft)
                }
            }

            if position.y.abs() > dead_zone.y {
                if position.y > 0.0 {
                    globals.emulated_input.emulate_button(Button::DPadDown)
                } else {
                    globals.emulated_input.emulate_button(Button::DPadUp)
                }
            }

            pressed_something = true;
        }

        // handle gestures

        self.last_empty_touch += 1;

        if !pressed_something && !self.touch_positions.is_empty() && self.previous_touch_count == 0
        {
            self.last_empty_touch = 0;
            self.empty_touch_count += 1;
        }

        if self.last_empty_touch > TOUCH_WINDOW {
            self.empty_touch_count = 0;
        }

        if self.empty_touch_count >= OPEN_EDITOR_TOUCH_TARGET {
            // open input editor with enough presses
            globals.editing_virtual_controller = true;
            self.empty_touch_count = 0;
        } else if !pressed_something
            && self.touch_positions.len() == 3
            && self.previous_touch_count != 3
        {
            // enable debug when three fingers are on the screen pressing nothing
            globals.debug_visible = !globals.debug_visible;
        }

        if !pressed_something && !self.touch_positions.is_empty() && self.previous_touch_count == 0
        {
            // close keyboard
            game_io.input_mut().end_text_input();
        }
    }

    fn first_button_under(&self, point: Vec2) -> Option<Button> {
        for (button, sprite) in &self.button_sprites {
            let bounds = sprite.bounds();

            if bounds.contains(point) {
                return Some(*button);
            }
        }

        let dpad_bounds = self.dpad_sprite.bounds();

        if dpad_bounds.contains(point) {
            return Some(Button::LeftStick);
        }

        None
    }

    fn select_first_button_at(&mut self, point: Vec2) {
        self.selected_buttons.clear();

        if let Some(button) = self.first_button_under(point) {
            self.selected_buttons.push(button);
        }
    }

    fn select_buttons_in_range(&mut self, point_a: Vec2, point_b: Vec2) {
        let selection_range = Rect::from_corners(point_a.min(point_b), point_a.max(point_b));

        self.selected_buttons.clear();

        for (button, sprite) in &self.button_sprites {
            let bounds = sprite.bounds();

            if bounds.overlaps(selection_range) {
                self.selected_buttons.push(*button);
            }
        }

        let dpad_bounds = self.dpad_sprite.bounds();

        if dpad_bounds.overlaps(selection_range) {
            self.selected_buttons.push(Button::LeftStick);
        }
    }

    fn for_each_selected_button_position(
        &mut self,
        game_io: &mut GameIO,
        callback: impl Fn(&mut Vec2),
    ) {
        let globals = game_io.resource_mut::<Globals>().unwrap();

        for selected_button in &self.selected_buttons {
            let Some(position) = globals
                .config
                .virtual_input_positions
                .get_mut(selected_button)
            else {
                continue;
            };

            callback(position);
        }
    }

    fn move_selected_buttons(&mut self, game_io: &mut GameIO, offset: Vec2) {
        self.for_each_selected_button_position(game_io, |pos| *pos += offset);
    }

    fn prepare_selected_buttons(&mut self, game_io: &mut GameIO) {
        self.for_each_selected_button_position(game_io, |pos| *pos = pos.floor());
    }

    fn respawn_buttons(&mut self, game_io: &mut GameIO) {
        let iter = std::iter::once((&Button::LeftStick, &self.dpad_sprite))
            .chain(self.button_sprites.iter().map(|(b, s)| (b, s)));

        let camera_bounds = self.camera.bounds();
        let camera_scale = self.camera.scale().y;

        for (button, sprite) in iter {
            let overlapping_area = camera_bounds.scissor(sprite.bounds());

            if overlapping_area.size().min_element() > 8.0 * camera_scale {
                continue;
            }

            let globals = game_io.resource_mut::<Globals>().unwrap();
            let Some(position) = globals.config.virtual_input_positions.get_mut(button) else {
                continue;
            };

            *position = camera_bounds.center();
            position.y = -position.y;

            if BUTTON_ORDER[RIGHT_INPUT_RANGE].contains(button) {
                position.x = -position.x;
            }
        }
    }

    fn update_editor_buttons(&mut self, game_io: &mut GameIO, mut touch: Option<Vec2>) {
        let patch_left = self.nine_patch.left_width();
        let patch_top = self.nine_patch.top_height();

        let camera_center = self.camera.bounds().center();

        let mut base_bounds = Rect::ZERO;
        base_bounds.x -= patch_left;
        base_bounds.y -= patch_top;
        base_bounds.width += patch_left + self.nine_patch.right_width();
        base_bounds.height += patch_top + self.nine_patch.bottom_height();

        let globals = game_io.resource_mut::<Globals>().unwrap();
        let config = &mut globals.config;

        if !self.selected_buttons.is_empty() {
            // this touch is for selecting multiple buttons, ignore it
            touch = None;
        }

        if let Some(mut touch) = touch {
            // reproject touch point to ignore scale
            touch -= camera_center;
            touch *= config.virtual_controller_scale;

            // detect hover
            for (button, offset, hovered) in &mut self.editor_buttons {
                let mut bounds = base_bounds;
                bounds += *offset;

                let text_metrics = self.editor_text_style.measure(button.state());
                bounds -= text_metrics.size * 0.5;
                bounds.set_size(bounds.size() + text_metrics.size);

                *hovered = bounds.contains(touch);
            }

            // the rest deals with handling button use, which occurs on release
            return;
        } else {
            // clear hover
            for (_, _, hovered) in &mut self.editor_buttons {
                *hovered = false;
            }
        }

        // try activating
        let Some(mut last_touch) = self.last_movement_touch else {
            return;
        };

        if !self.selected_buttons.is_empty() {
            // this release was to finish selection, not to press
            return;
        }

        // reproject touch point to ignore scale
        last_touch -= camera_center;
        last_touch *= config.virtual_controller_scale;

        for (button, offset, _) in &self.editor_buttons {
            let mut bounds = base_bounds;
            bounds += *offset;

            let text_metrics = self.editor_text_style.measure(button.state());
            bounds -= text_metrics.size * 0.5;
            bounds.set_size(bounds.size() + text_metrics.size);

            if !bounds.contains(last_touch) {
                continue;
            }

            match button {
                EditorButton::Confirm => {
                    globals.editing_virtual_controller = false;

                    // save edits made outside of config
                    if !globals.editing_config {
                        globals.config.save();
                    }

                    return;
                }
                EditorButton::Reset => {
                    config.virtual_input_positions = Config::default_virtual_input_positions();
                    config.virtual_controller_scale = 1.0;
                }
                EditorButton::ScaleDown => {
                    config.virtual_controller_scale =
                        (config.virtual_controller_scale - 0.1).max(0.2)
                }
                EditorButton::ScaleUp => {
                    config.virtual_controller_scale =
                        (config.virtual_controller_scale + 0.1).min(2.5)
                }
            }
        }
    }

    fn move_buttons_update(&mut self, game_io: &mut GameIO) {
        let touch_id = game_io.input().touches().first().map(|t| t.id);

        if touch_id != self.last_touch_id && self.last_touch_id.is_some() && touch_id.is_some() {
            self.selected_buttons.clear();
            self.dragging_buttons = false;
        }

        let touch = self.touch_positions.first().cloned();

        if let Some(touch) = touch {
            if let Some(start) = self.selection_start {
                // continue selection
                self.select_buttons_in_range(start, touch);
            } else if self.selected_buttons.is_empty() {
                // try to select a button to drag
                self.select_first_button_at(touch);

                if self.selected_buttons.is_empty() {
                    // start range selection
                    self.selection_start = Some(touch);
                }
            } else if self.dragging_buttons {
                // handle motion
                if let Some(last_touch) = self.last_movement_touch {
                    self.move_selected_buttons(game_io, touch - last_touch);
                }
            } else if self
                .first_button_under(touch)
                .is_some_and(|b| self.selected_buttons.contains(&b))
            {
                // just started moving the buttons
                self.dragging_buttons = true;
                self.prepare_selected_buttons(game_io);
            } else {
                // nothing under the touch position, we should empty it
                self.selected_buttons.clear();
            }
        } else if self.last_movement_touch.is_some() {
            self.respawn_buttons(game_io);

            // clear range selection
            self.selection_start = None;
            self.dragging_buttons = false;
        }

        self.update_editor_buttons(game_io, touch);

        self.last_movement_touch = touch;
    }

    fn confirm_positions(&mut self) {
        self.last_movement_touch = None;
        self.selection_start = None;
    }

    fn try_toggling_visibility(&self, game_io: &mut GameIO) {
        if game_io.window().ime_height() > 0 {
            return;
        }

        let input = game_io.input();

        let globals = game_io.resource::<Globals>().unwrap();
        let previously_visible = globals.global_save.virtual_controller_visible;

        let new_visible = if previously_visible {
            input
                .controller(0)
                .is_none_or(|c| c.latest_button().is_none())
        } else {
            !input.touches().is_empty()
        };

        if new_visible != previously_visible {
            let globals = game_io.resource_mut::<Globals>().unwrap();
            globals.global_save.virtual_controller_visible = new_visible;
            globals.global_save.save();
        }
    }
}

impl GameOverlay for VirtualController {
    fn pre_update(&mut self, game_io: &mut GameIO) {
        // update camera
        let window_size = game_io.window().size().as_vec2();
        let globals = game_io.resource::<Globals>().unwrap();
        let y_relative_scale =
            window_size.y / RESOLUTION_F.y * globals.config.virtual_controller_scale;

        self.camera
            .set_scale(RESOLUTION_F / window_size * y_relative_scale);
        self.camera.snap(window_size * 0.5 / y_relative_scale);

        self.editor_camera
            .set_scale(self.camera.scale() / globals.config.virtual_controller_scale);

        // update touch positions
        self.touch_scale = 1.0 / y_relative_scale;
        self.update_touch_positions(game_io);

        // update buttons
        let globals = game_io.resource_mut::<Globals>().unwrap();
        let config = &globals.config;
        let camera_bounds = self.camera.bounds();

        let get_relative_position = |button: Button| -> Vec2 {
            config
                .virtual_input_positions
                .get(&button)
                .cloned()
                .unwrap_or_default()
        };

        // update dpad
        {
            let sprite = &mut self.dpad_sprite;

            let mut position = get_relative_position(Button::LeftStick);
            position.y += camera_bounds.bottom();
            sprite.set_position(position.floor());
        }

        // update left inputs
        for i in LEFT_INPUT_RANGE {
            let (button, sprite) = &mut self.button_sprites[i];

            let mut position = get_relative_position(*button);
            position.y += camera_bounds.bottom();
            sprite.set_position(position.floor());
        }

        // update right inputs
        for i in RIGHT_INPUT_RANGE {
            let (button, sprite) = &mut self.button_sprites[i];

            let position = get_relative_position(*button) + camera_bounds.bottom_right();
            sprite.set_position(position.floor());
        }

        // flush input
        globals.emulated_input.flush();

        // handle update
        let editing = globals.editing_virtual_controller;

        let view_state = if game_io.input().accepting_text() && game_io.window().ime_height() > 0 {
            ViewState::Keyboard
        } else if editing {
            ViewState::Editor
        } else {
            ViewState::Default
        };

        // avoid updating when the player is touching the screen while switching state
        if view_state == self.previous_view_state || self.touch_positions.is_empty() {
            match view_state {
                ViewState::Default => self.accept_input_update(game_io),
                ViewState::Editor => self.move_buttons_update(game_io),
                ViewState::Keyboard => {
                    if !self.touch_positions.is_empty() && self.previous_touch_count == 0 {
                        // close keyboard
                        game_io.input_mut().end_text_input();
                    }
                }
            }

            self.previous_view_state = view_state;
        }

        self.last_touch_id = game_io.input().touches().first().map(|t| t.id);
        self.try_toggling_visibility(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        if game_io.input().accepting_text() && game_io.window().ime_height() > 0 {
            // hide if the keyboard is open
            return;
        }

        let globals = game_io.resource::<Globals>().unwrap();

        if !globals.global_save.virtual_controller_visible {
            return;
        }

        let editing = globals.editing_virtual_controller;

        let assets = &globals.assets;
        let mut pixel = assets.new_sprite(game_io, ResourcePaths::PIXEL);

        // draw editing buttons
        if editing {
            let mut queue =
                SpriteColorQueue::new(game_io, &self.editor_camera, SpriteColorMode::Multiply);

            let camera_center = self.editor_camera.bounds().center();

            let text_color = self.editor_text_style.color;

            for (button, offset, hovered) in &self.editor_buttons {
                let text = button.state();

                // handle state colors
                let brightness = if *hovered {
                    CONFIRM_PRESSED_BRIGHTNESS
                } else {
                    CONFIRM_RELEASED_BRIGHTNESS
                };

                self.editor_text_style.color = text_color.multiply_color(brightness);
                self.nine_patch
                    .set_color(Color::WHITE.multiply_color(brightness));

                // resolve bounds and render
                let metrics = self.editor_text_style.measure(text);

                let position = camera_center + *offset - metrics.size * 0.5;
                let bounds = Rect::from_corners(position, position + metrics.size);
                self.nine_patch.draw(&mut queue, bounds);

                self.editor_text_style.bounds.set_position(position);
                self.editor_text_style.draw(game_io, &mut queue, text);
            }

            // reset text style color
            self.editor_text_style.color = text_color;

            render_pass.consume_queue(queue);
        }

        let mut queue = SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw buttons
        for (button, sprite) in &mut self.button_sprites {
            let bounds = sprite.bounds();
            let pressed = if self.previous_view_state == ViewState::Keyboard {
                false
            } else if editing {
                self.selected_buttons.contains(button)
            } else {
                self.touch_positions
                    .iter()
                    .any(|&position| bounds.contains(position))
            };

            let color = if pressed {
                PRESSED_COLOR
            } else {
                RELEASED_COLOR
            };

            sprite.set_color(color);
            queue.draw_sprite(sprite);

            if editing && pressed {
                draw_rect(&mut queue, &mut pixel, sprite.bounds());
            }
        }

        // draw dpad
        let mut dpad_color = RELEASED_COLOR;

        if self.selected_buttons.contains(&Button::LeftStick) {
            draw_rect(&mut queue, &mut pixel, self.dpad_sprite.bounds());
            dpad_color = PRESSED_COLOR
        }

        self.dpad_sprite.set_color(dpad_color);
        queue.draw_sprite(&self.dpad_sprite);

        // draw selection rect
        if let (Some(start), Some(&end)) = (self.selection_start, self.touch_positions.first()) {
            draw_rect(&mut queue, &mut pixel, Rect::from_corners(start, end));
        }

        render_pass.consume_queue(queue);
    }
}

fn draw_rect(queue: &mut SpriteColorQueue, pixel_sprite: &mut Sprite, bounds: Rect) {
    let width = bounds.width.abs();
    let height = bounds.height.abs();

    let top_left = bounds.top_left();

    // draw vertical bars
    pixel_sprite.set_position(top_left);
    pixel_sprite.set_height(height);
    queue.draw_sprite(pixel_sprite);

    pixel_sprite.set_position(bounds.top_right() - Vec2::new(1.0, 0.0));
    queue.draw_sprite(pixel_sprite);

    // draw horizontal bars
    pixel_sprite.set_position(top_left);
    pixel_sprite.set_width(width);
    pixel_sprite.set_height(1.0);
    queue.draw_sprite(pixel_sprite);

    pixel_sprite.set_position(bounds.bottom_left() - Vec2::new(0.0, 1.0));
    queue.draw_sprite(pixel_sprite);

    // reset size for the next use
    pixel_sprite.set_width(1.0);
}
