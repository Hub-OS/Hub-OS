use crate::bindable::{BlockColor, SpriteColorMode};
use crate::packages::PackageNamespace;
use crate::render::ui::{
    ContextMenu, FontStyle, GridCursor, SceneTitle, ScrollTracker, Text, TextStyle, Textbox,
    TextboxQuestion, UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, FrameTime, SpriteColorQueue};
use crate::resources::*;
use crate::saves::{BlockGrid, InstalledBlock};
use framework::prelude::*;

const DIM_COLOR: Color = Color::new(0.75, 0.75, 0.75, 1.0);

enum Event {
    Leave,
}

#[derive(Clone, Copy)]
enum BlockOption {
    Move,
    Remove,
}

struct CompactPackageInfo {
    id: String,
    name: String,
    color: BlockColor,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum State {
    ListSelection,
    GridSelection { x: usize, y: usize },
    BlockContext { x: usize, y: usize },
}

pub struct CustomizeScene {
    camera: Camera,
    background: Background,
    animator: Animator,
    top_bar: Sprite,
    grid_sprite: Sprite,
    grid_start: Vec2,
    grid_increment: Vec2,
    rail_sprite: Sprite,
    information_box_sprite: Sprite,
    information_text: Text,
    input_tracker: UiInputTracker,
    scroll_tracker: ScrollTracker,
    colors: Vec<BlockColor>,
    grid: BlockGrid,
    block_context_menu: ContextMenu<BlockOption>,
    held_block: Option<InstalledBlock>,
    cursor: GridCursor,
    block_returns_to_grid: bool,
    state: State,
    textbox: Textbox,
    time: FrameTime,
    packages: Vec<CompactPackageInfo>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    next_scene: NextScene,
}

impl CustomizeScene {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // load block packages
        let mut packages: Vec<_> = globals
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
                }
            })
            .collect();

        packages.sort_by(|a, b| a.id.cmp(&b.id));

        // load ui sprites
        let mut animator = Animator::load_new(assets, ResourcePaths::CUSTOMIZE_UI_ANIMATION);
        let mut grid_sprite = assets.new_sprite(game_io, ResourcePaths::CUSTOMIZE_UI);

        // grid sprite
        animator.set_state("GRID");
        animator.apply(&mut grid_sprite);

        let grid_start = animator.point("GRID_START").unwrap_or_default() - grid_sprite.origin();
        let grid_step = animator.point("GRID_STEP").unwrap_or_default();

        // rail sprite
        let mut rail_sprite = grid_sprite.clone();
        animator.set_state("RAIL");
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

        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new(bg_animator, bg_sprite),
            animator,
            top_bar,
            grid_sprite,
            grid_start,
            grid_increment: grid_step,
            rail_sprite,
            information_box_sprite,
            information_text,
            input_tracker: UiInputTracker::new(),
            scroll_tracker,
            colors: Vec::new(),
            grid: BlockGrid::new(),
            block_context_menu: ContextMenu::new(game_io, "", Vec2::ZERO).with_options(
                game_io,
                &[("Move", BlockOption::Move), ("Remove", BlockOption::Remove)],
            ),
            held_block: None,
            cursor,
            state: State::ListSelection,
            block_returns_to_grid: false,
            textbox: Textbox::new_navigation(game_io).with_position(RESOLUTION_F * 0.5),
            time: 0,
            packages,
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
        }
    }

    fn update_colors(&mut self) {
        // retain colors that are on the grid
        self.colors.retain(|color| {
            self.grid
                .installed_blocks()
                .any(|block| block.color == *color)
        });

        // add colors missing from our list
        for block in self.grid.installed_blocks() {
            if !self.colors.contains(&block.color) {
                self.colors.push(block.color);
            }
        }
    }

    fn update_text(&mut self, game_io: &GameIO) {
        match self.state {
            State::GridSelection { x, y } => {
                self.information_text.text = if let Some(block) = self.grid.get_block((x, y)) {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let packages = &globals.block_packages;
                    let package = packages
                        .package_or_fallback(PackageNamespace::Server, &block.package_id)
                        .unwrap();

                    package.description.clone()
                } else {
                    String::new()
                }
            }
            State::ListSelection => {
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
            _ => {}
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        let prev_state = self.state;
        let prev_held = self.held_block.is_some();

        if let Some(block) = &mut self.held_block {
            let prev_rotation = block.rotation;

            if self.input_tracker.is_active(Input::ShoulderL) {
                block.rotate_cc();
            }
            if self.input_tracker.is_active(Input::ShoulderR) {
                block.rotate_c();
            }

            if block.rotation != prev_rotation {
                globals.audio.play_sound(&globals.cursor_move_sfx);
            }

            if self.input_tracker.is_active(Input::Confirm) {
                let mut clone = block.clone();

                if let State::GridSelection { x, y } = self.state {
                    clone.position = (x, y);
                }

                let success = self.grid.install_block(game_io, clone).is_none();

                if success {
                    globals.audio.play_sound(&globals.cursor_select_sfx);
                    self.held_block = None;
                    self.update_colors();
                } else {
                    globals.audio.play_sound(&globals.cursor_error_sfx);
                }
            } else if self.input_tracker.is_active(Input::Cancel) {
                let block = self.held_block.take().unwrap();

                if self.block_returns_to_grid {
                    let (x, y) = block.position;
                    self.state = State::GridSelection { x, y };

                    self.grid.install_block(game_io, block);
                } else {
                    self.uninstall(game_io, block);

                    self.state = State::ListSelection;
                }

                globals.audio.play_sound(&globals.cursor_cancel_sfx);
            }
        }

        match self.state {
            State::ListSelection => {
                if self.input_tracker.is_active(Input::Cancel) && !prev_held {
                    let event_sender = self.event_sender.clone();

                    let question = TextboxQuestion::new(
                        String::from("Quit customizing and return to menu?"),
                        move |yes| {
                            if yes {
                                event_sender.send(Event::Leave).unwrap();
                            }
                        },
                    );

                    self.textbox.push_interface(question);
                    self.textbox.open();
                } else if self.input_tracker.is_active(Input::Left) {
                    let y = self.scroll_tracker.selected_index() - self.scroll_tracker.top_index();

                    self.state = State::GridSelection { x: 5, y: y.max(1) };

                    globals.audio.play_sound(&globals.cursor_select_sfx);
                } else {
                    let prev_index = self.scroll_tracker.selected_index();

                    self.scroll_tracker
                        .handle_vertical_input(&self.input_tracker);

                    let selected_index = self.scroll_tracker.selected_index();

                    if prev_index != selected_index {
                        globals.audio.play_sound(&globals.cursor_move_sfx);

                        self.update_text(game_io);
                    }

                    if self.input_tracker.is_active(Input::Confirm) {
                        self.state = State::GridSelection { x: 2, y: 2 };

                        if self.packages.get(selected_index).is_some() {
                            let package = self.packages.remove(selected_index);
                            self.scroll_tracker.set_total_items(self.packages.len() + 1);

                            self.held_block = Some(InstalledBlock {
                                package_id: package.id.clone(),
                                rotation: 0,
                                color: package.color,
                                position: (0, 0),
                            });
                            self.block_returns_to_grid = false;

                            globals.audio.play_sound(&globals.cursor_select_sfx);
                        }
                    }
                }
            }
            State::GridSelection { x: old_x, y: old_y } => {
                let cancel = self.input_tracker.is_active(Input::Cancel) && !prev_held;
                let returned_to_list = self.input_tracker.is_active(Input::Right) && old_x == 6;
                let has_block = self.grid.get_block((old_x, old_y)).is_some();

                if (cancel || returned_to_list) && self.held_block.is_none() {
                    self.state = State::ListSelection;

                    globals.audio.play_sound(&globals.cursor_cancel_sfx);
                } else if self.input_tracker.is_active(Input::Confirm) && !prev_held && has_block {
                    self.state = State::BlockContext { x: old_x, y: old_y };

                    self.block_context_menu.open();
                    self.block_context_menu.update(game_io, &self.input_tracker);

                    let menu_size = self.block_context_menu.bounds().size();
                    let mut position = self.cursor.target();
                    position.x += (self.grid_increment.x - menu_size.x) * 0.5;

                    let min_x = self.grid_increment.x * 0.5;
                    let max_x =
                        self.grid_increment.x * (BlockGrid::SIDE_LEN as f32 - 0.5) - menu_size.x;

                    position.x =
                        (position.x).clamp(self.grid_start.x + min_x, self.grid_start.x + max_x);

                    if old_y < 4 {
                        position.y += self.grid_increment.y * 0.75;
                    } else {
                        position.y -= menu_size.y + self.grid_increment.y * 0.25;
                    }

                    self.block_context_menu.set_position(position);
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

                    if self.state != prev_state {
                        globals.audio.play_sound(&globals.cursor_move_sfx);
                    }
                }
            }
            State::BlockContext { x, y } => {
                let selection = self.block_context_menu.update(game_io, &self.input_tracker);

                if let Some(op) = selection {
                    let block = self.grid.remove_block((x, y)).unwrap();

                    match op {
                        BlockOption::Move => {
                            self.held_block = Some(block);
                            self.block_returns_to_grid = true;
                        }
                        BlockOption::Remove => {
                            self.uninstall(game_io, block);
                        }
                    }

                    self.block_context_menu.close();
                    self.state = State::GridSelection { x, y };
                }

                if !self.block_context_menu.is_open() {
                    self.state = State::GridSelection { x, y };
                }
            }
        }

        if self.state != prev_state || prev_held != self.held_block.is_some() {
            self.update_cursor_sprite();
            self.update_text(game_io);
        }
    }

    fn uninstall(&mut self, game_io: &GameIO, block: InstalledBlock) {
        let index = self
            .packages
            .binary_search_by(|package| package.id.cmp(&block.package_id));

        let index = match index {
            Ok(index) | Err(index) => index,
        };

        let globals = game_io.resource::<Globals>().unwrap();
        let package = globals
            .block_packages
            .package_or_fallback(PackageNamespace::Server, &block.package_id)
            .unwrap();

        self.packages.insert(
            index,
            CompactPackageInfo {
                id: block.package_id.clone(),
                name: package.name.clone(),
                color: block.color,
            },
        );
        self.scroll_tracker.set_total_items(self.packages.len() + 1);
        self.update_colors();
    }

    fn update_cursor_sprite(&mut self) {
        match self.state {
            State::GridSelection { x, y } => {
                if self.held_block.is_some() {
                    if self.block_returns_to_grid {
                        self.cursor.use_claw_grip_cursor();
                    } else {
                        self.cursor.hide();
                    }
                } else if self.grid.get_block((x, y)).is_some() {
                    self.cursor.use_claw_cursor();
                } else {
                    self.cursor.use_grid_cursor();
                }
            }
            State::ListSelection => self.cursor.use_textbox_cursor(),
            _ => {}
        }

        self.update_grid_sprite();
    }

    fn update_grid_sprite(&mut self) {
        if matches!(self.state, State::GridSelection { .. }) {
            self.grid_sprite.set_color(Color::WHITE);
            self.rail_sprite.set_color(Color::WHITE);
        } else {
            self.grid_sprite.set_color(DIM_COLOR);
            self.rail_sprite.set_color(DIM_COLOR);
        }
    }

    fn update_cursor(&mut self) {
        match self.state {
            State::GridSelection { x, y } => {
                let position =
                    Vec2::new(x as f32, y as f32) * self.grid_increment + self.grid_start;

                self.cursor.set_target(position);
            }
            State::ListSelection => {
                let position = self.scroll_tracker.selected_cursor_position();
                self.cursor.set_target(position);
            }
            _ => {}
        }

        self.cursor.update();
    }

    fn handle_events(&mut self, game_io: &GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::Leave => {
                    let transition = crate::transitions::new_sub_scene_pop(game_io);
                    self.next_scene = NextScene::new_pop().with_transition(transition);
                }
            }
        }
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

        self.textbox.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        if !game_io.is_in_transition() && !self.textbox.is_open() {
            self.handle_input(game_io);
        }

        self.update_cursor();
        self.handle_events(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = game_io.resource::<Globals>().unwrap();

        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw grid
        sprite_queue.draw_sprite(&self.grid_sprite);

        // draw colors
        let mut color_sprite = Sprite::new(game_io, self.grid_sprite.texture().clone());

        self.animator.set_state("GRID");
        let mut color_position =
            self.animator.point("COLORS_START").unwrap_or_default() - self.grid_sprite.origin();
        let color_step = self.animator.point("COLORS_STEP").unwrap_or_default();

        for (i, color) in self.colors.iter().enumerate() {
            if i == 4 {
                color_position.x += 1.0;
            } else if i > 4 {
                color_position.x -= 1.0;
            }

            self.animator.set_state(color.state());
            self.animator.apply(&mut color_sprite);

            color_sprite.set_position(color_position);
            sprite_queue.draw_sprite(&color_sprite);

            color_position += color_step;
        }

        // draw grid blocks
        let mut block_sprite = Sprite::new(game_io, self.grid_sprite.texture().clone());

        if !matches!(self.state, State::GridSelection { .. }) {
            block_sprite.set_color(DIM_COLOR);
        }

        for y in 0..BlockGrid::SIDE_LEN {
            for x in 0..BlockGrid::SIDE_LEN {
                let Some(block) = self.grid.get_block((x, y)) else {
                    continue;
                };

                let position =
                    Vec2::new(x as f32, y as f32) * self.grid_increment + self.grid_start;
                block_sprite.set_position(position);

                if y == 0 {
                    self.animator.set_state("OVERLAP_U");
                } else if x == BlockGrid::SIDE_LEN - 1 {
                    self.animator.set_state("OVERLAP_R");
                } else if y == BlockGrid::SIDE_LEN - 1 {
                    self.animator.set_state("OVERLAP_D");
                } else if x == 0 {
                    self.animator.set_state("OVERLAP_L");
                } else {
                    block_sprite.set_position(position);

                    let packages = &globals.block_packages;
                    let package = packages
                        .package_or_fallback(PackageNamespace::Server, &block.package_id)
                        .unwrap();

                    if package.is_program {
                        self.animator.set_state(block.color.flat_state());
                    } else {
                        self.animator.set_state(block.color.plus_state());
                    }
                }

                self.animator.apply(&mut block_sprite);
                sprite_queue.draw_sprite(&block_sprite);

                block_sprite.set_rotation(0.0);
            }
        }

        // draw rail on top
        sprite_queue.draw_sprite(&self.rail_sprite);

        // draw package list
        let mut recycled_sprite = Sprite::new(game_io, self.grid_sprite.texture().clone());
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

        // draw held blocks
        if let Some(block) = &self.held_block {
            let packages = &globals.block_packages;
            let package = packages
                .package_or_fallback(PackageNamespace::Server, &block.package_id)
                .unwrap();

            // create sprite
            block_sprite.set_color(Color::WHITE.multiply_alpha(0.5));

            if package.is_program {
                self.animator.set_state(block.color.flat_held_state());
            } else {
                self.animator.set_state(block.color.plus_held_state());
            }

            self.animator.apply(&mut block_sprite);

            // loop over shape and draw blocks under cursor
            let cursor_position = self.cursor.position();

            for y in 0..5 {
                for x in 0..5 {
                    if !package.exists_at(block.rotation, (x, y)) {
                        continue;
                    }

                    let offset = (Vec2::new(x as f32, y as f32) - 2.0) * self.grid_increment;
                    block_sprite.set_position(cursor_position + offset);
                    sprite_queue.draw_sprite(&block_sprite);
                }
            }
        }

        // draw cursor
        self.cursor.draw(&mut sprite_queue);

        // draw context menu
        self.block_context_menu.draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        // draw top bar over everything
        sprite_queue.draw_sprite(&self.top_bar);
        SceneTitle::new("CUSTOMIZE").draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}
