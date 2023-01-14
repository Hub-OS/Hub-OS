use crate::bindable::{BlockColor, SpriteColorMode};
use crate::packages::PackageNamespace;
use crate::render::ui::{
    FontStyle, GridCursor, SceneTitle, ScrollTracker, Text, TextStyle, Textbox, UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, FrameTime, SpriteColorQueue};
use crate::resources::*;
use framework::prelude::*;

struct CompactPackageInfo {
    id: String,
    name: String,
    color: BlockColor,
    is_flat: bool,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum State {
    GridSelection { x: usize, y: usize },
    ListSelection,
}

pub struct CustomizeScene {
    camera: Camera,
    background: Background,
    animator: Animator,
    top_bar: Sprite,
    grid_sprite: Sprite,
    rail_sprite: Sprite,
    information_box_sprite: Sprite,
    information_text: Text,
    input_tracker: UiInputTracker,
    scroll_tracker: ScrollTracker,
    cursor: GridCursor,
    state: State,
    textbox: Textbox,
    time: FrameTime,
    packages: Vec<CompactPackageInfo>,
    next_scene: NextScene,
}

impl CustomizeScene {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // load block packages
        let packages: Vec<_> = globals
            .block_packages
            .local_packages()
            .map(|id| {
                let package = globals
                    .block_packages
                    .package_or_fallback(PackageNamespace::Server, id)
                    .unwrap();

                CompactPackageInfo {
                    id: id.clone(),
                    name: package.name.clone(),
                    color: package.block_color,
                    is_flat: package.is_program,
                }
            })
            .collect();

        // load ui sprites
        let mut animator = Animator::load_new(assets, ResourcePaths::CUSTOMIZE_UI_ANIMATION);
        let mut grid_sprite = assets.new_sprite(game_io, ResourcePaths::CUSTOMIZE_UI);

        // grid sprite
        animator.set_state("GRID_DIM");
        animator.apply(&mut grid_sprite);

        // rail sprite
        let mut rail_sprite = grid_sprite.clone();
        animator.set_state("RAIL_DIM");
        animator.apply(&mut rail_sprite);

        // information box sprite
        let mut information_box_sprite = grid_sprite.clone();
        animator.set_state("TEXTBOX");
        animator.apply(&mut information_box_sprite);

        let information_bounds = Rect::from_corners(
            animator.point("TEXT_START").unwrap_or_default(),
            animator.point("TEXT_END").unwrap_or_default(),
        ) - animator.origin();
        let information_text = Text::new(game_io, FontStyle::Thin)
            .with_bounds(information_bounds)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR);

        // bg
        let mut bg_animator = Animator::load_new(assets, ResourcePaths::CUSTOMIZE_BG_ANIMATION);
        let bg_sprite = assets.new_sprite(game_io, ResourcePaths::CUSTOMIZE_BG);

        // bg top bar
        let mut top_bar = bg_sprite.clone();
        bg_animator.set_state("TOP_BAR_OVERLAY");
        bg_animator.apply(&mut top_bar);

        // scroll tracker
        let mut scroll_tracker = ScrollTracker::new(game_io, 4)
            .with_view_margin(1)
            .with_total_items(packages.len() + 1)
            .with_custom_cursor(
                game_io,
                ResourcePaths::TEXTBOX_CURSOR_ANIMATION,
                ResourcePaths::TEXTBOX_CURSOR,
            );

        animator.set_state("OPTION");
        let list_next = animator.point("NEXT").unwrap_or_default();
        let list_start = animator.point("START").unwrap_or_default() + list_next;

        scroll_tracker.define_cursor(list_start, list_next.y);

        // cursor
        let cursor_position = scroll_tracker.selected_cursor_position();
        let cursor = GridCursor::new(game_io).with_position(cursor_position);

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new(bg_animator, bg_sprite),
            animator,
            top_bar,
            grid_sprite,
            rail_sprite,
            information_box_sprite,
            information_text,
            input_tracker: UiInputTracker::new(),
            scroll_tracker,
            cursor,
            state: State::ListSelection,
            textbox: Textbox::new_navigation(game_io)
                .with_position(Vec2::new(0.0, RESOLUTION_F.y * 0.5)),
            time: 0,
            packages,
            next_scene: NextScene::None,
        }
    }

    fn update_text(&mut self, game_io: &GameIO) {
        let index = self.scroll_tracker.selected_index();

        if let Some(package) = self.packages.get(index) {
            let globals = game_io.resource::<Globals>().unwrap();

            let package = globals
                .block_packages
                .package_or_fallback(PackageNamespace::Server, &package.id)
                .unwrap();

            self.information_text.text = package.description.clone();
        } else {
            self.information_text.text = String::from("RUN?");
        }
    }

    fn handle_input(&mut self, game_io: &GameIO) {
        let prev_state = self.state;

        match self.state {
            State::GridSelection { x: old_x, y: old_y } => {
                let cancel = self.input_tracker.is_active(Input::Cancel);
                let returned_to_list = self.input_tracker.is_active(Input::Right) && old_x == 6;

                if cancel || returned_to_list {
                    self.state = State::ListSelection;
                } else {
                    let (mut x, mut y) = (old_x, old_y);

                    let input_x = self.input_tracker.input_as_axis(Input::Left, Input::Right);
                    let input_y = self.input_tracker.input_as_axis(Input::Up, Input::Down);

                    if input_x < 0.0 && x > 0 {
                        x -= 1;
                    }

                    if input_x > 0.0 && x < 6 {
                        x += 1;
                    }

                    if input_y < 0.0 && y > 0 {
                        y -= 1;
                    }

                    if input_y > 0.0 && y < 6 {
                        y += 1;
                    }

                    self.state = State::GridSelection { x, y };
                }
            }
            State::ListSelection => {
                if self.input_tracker.is_active(Input::Left) {
                    let y = self.scroll_tracker.selected_index() - self.scroll_tracker.top_index();

                    self.state = State::GridSelection { x: 5, y: y.max(1) };
                } else {
                    let prev_index = self.scroll_tracker.selected_index();

                    self.scroll_tracker
                        .handle_vertical_input(&self.input_tracker);

                    if prev_index != self.scroll_tracker.selected_index() {
                        let globals = game_io.resource::<Globals>().unwrap();

                        globals.audio.play_sound(&globals.cursor_move_sfx);
                    }
                }
            }
        }

        if self.state != prev_state {
            let globals = game_io.resource::<Globals>().unwrap();

            globals.audio.play_sound(&globals.cursor_move_sfx);

            self.update_cursor_sprite();
        }
    }

    fn update_cursor_sprite(&mut self) {
        match self.state {
            State::GridSelection { .. } => {
                self.cursor.use_grid_cursor();
            }
            State::ListSelection => self.cursor.use_textbox_cursor(),
        }

        self.update_grid_sprite();
    }

    fn update_grid_sprite(&mut self) {
        if matches!(self.state, State::GridSelection { .. }) {
            self.animator.set_state("GRID");
            self.animator.apply(&mut self.grid_sprite);

            self.animator.set_state("RAIL");
            self.animator.apply(&mut self.rail_sprite);
        } else {
            self.animator.set_state("GRID_DIM");
            self.animator.apply(&mut self.grid_sprite);

            self.animator.set_state("RAIL_DIM");
            self.animator.apply(&mut self.rail_sprite);
        }
    }

    fn update_cursor(&mut self) {
        match self.state {
            State::GridSelection { x, y } => {
                self.animator.set_state("GRID");

                let start = self.animator.point("GRID_START").unwrap_or_default()
                    - self.grid_sprite.origin();

                let next = self.animator.point("GRID_NEXT").unwrap_or_default();
                let position = Vec2::new(x as f32, y as f32) * next + start;

                self.cursor.set_target(position);
            }
            State::ListSelection => {
                let position = self.scroll_tracker.selected_cursor_position();
                self.cursor.set_target(position);
            }
        }

        self.cursor.update();
    }
}

impl Scene for CustomizeScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.update_text(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.input_tracker.update(game_io);
        self.time += 1;

        if !game_io.is_in_transition() {
            self.handle_input(game_io);
        }

        self.update_cursor();
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw grid
        sprite_queue.draw_sprite(&self.grid_sprite);

        // draw grid blocks

        // draw rail on top
        sprite_queue.draw_sprite(&self.rail_sprite);

        // draw package list
        let mut recycled_sprite = self.grid_sprite.clone();
        let selected_index = if self.state == State::ListSelection {
            Some(self.scroll_tracker.selected_index())
        } else {
            None
        };

        self.animator.set_state("OPTION");
        let mut offset = self.animator.point("START").unwrap_or_default();
        let offset_jump = self.animator.point("NEXT").unwrap_or_default();

        let text_bounds = Rect::from_corners(
            self.animator.point("TEXT_START").unwrap_or_default(),
            self.animator.point("TEXT_END").unwrap_or_default(),
        );
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
            .with_bounds(text_bounds);

        if self.scroll_tracker.top_index() == 0 {
            offset += offset_jump;
        }

        text_style.bounds += offset - offset_jump;

        for i in self.scroll_tracker.view_range() {
            recycled_sprite.set_position(offset);
            text_style.bounds += offset_jump;
            offset += offset_jump;

            if i == self.packages.len() {
                // draw RUN button
                if Some(i) == selected_index {
                    self.animator.set_state("RUN_BLINK");
                    self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                    self.animator.sync_time(self.time);
                } else {
                    self.animator.set_state("RUN");
                }

                self.animator.apply(&mut recycled_sprite);
                sprite_queue.draw_sprite(&recycled_sprite);
                continue;
            }

            // draw package option
            if Some(i) == selected_index {
                self.animator.set_state("OPTION_BLINK");
                self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                self.animator.sync_time(self.time);
            } else {
                self.animator.set_state("OPTION");
            }

            self.animator.apply(&mut recycled_sprite);
            sprite_queue.draw_sprite(&recycled_sprite);

            let package_name = &self.packages[i].name;
            text_style.draw(game_io, &mut sprite_queue, package_name)
        }

        // draw information text
        sprite_queue.draw_sprite(&self.information_box_sprite);
        self.information_text.draw(game_io, &mut sprite_queue);

        // draw cursor
        self.cursor.draw(&mut sprite_queue);

        // draw top bar over everything
        sprite_queue.draw_sprite(&self.top_bar);
        SceneTitle::new("CUSTOMIZE").draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
