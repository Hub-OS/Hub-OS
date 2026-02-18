use crate::bindable::{BlockColor, SpriteColorMode};
use crate::ease::inverse_lerp;
use crate::packages::{AugmentPackage, PackageId, PackageNamespace};
use crate::render::ui::{
    BlockPreview, ContextMenu, FontName, GridArrow, GridArrowStatus, GridCursor, GridScrollTracker,
    SceneTitle, ScrollTracker, SubSceneFrame, Text, TextStyle, Textbox, TextboxMessage,
    TextboxQuestion, UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, FrameTime, SpriteColorQueue};
use crate::resources::*;
use crate::saves::{BlockGrid, BlockShape, GlobalSave, InstalledBlock};
use framework::cfg_macros::cfg_android;
use framework::prelude::*;
use itertools::Itertools;
use packets::structures::PackageCategory;
use std::borrow::Cow;
use std::collections::{HashMap, HashSet};
use std::sync::Arc;

const DIM_COLOR: Color = Color::new(0.75, 0.75, 0.75, 1.0);
const TEXT_COLOR: Color = Color::WHITE;
const INVALID_TEXT_COLOR: Color = Color::ORANGE;
const COLOR_UI_DELAY: FrameTime = 24;

enum Event {
    Leave,
    Applied,
    Clear,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum GridOption {
    Move,
    Remove,
    More,
    Export,
    Import,
    Clear,
}

#[derive(Clone)]
struct ListItem {
    id: PackageId,
    name: Arc<str>,
    colors: Vec<(BlockColor, usize)>,
    selected_color: usize,
}

impl ListItem {
    fn new(game_io: &GameIO, restrictions: &Restrictions, package: &AugmentPackage) -> Self {
        let color_iter = package.block_colors.iter();
        let id = &package.package_info.id;

        let mut colors: Vec<_> = color_iter
            .unique()
            .map(|&color| (color, restrictions.block_count(game_io, id, color)))
            .filter(|(_, max_count)| *max_count > 0)
            .collect();

        colors.sort_by_key(|(color, _)| *color);

        Self {
            id: package.package_info.id.clone(),
            name: package.name.clone(),
            colors,
            selected_color: 0,
        }
    }

    fn remaining_colors(&self) -> usize {
        self.colors.iter().filter(|(_, c)| *c > 0).count()
    }

    fn select_next_color(&mut self) -> bool {
        let initial_index = self.selected_color;

        loop {
            self.selected_color += 1;
            self.selected_color %= self.colors.len();

            if self.selected_color == initial_index {
                return false;
            }

            if self.colors[self.selected_color].1 > 0 {
                return true;
            }
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum State {
    ListSelection,
    ColorSelection,
    GridSelection { x: i8, y: i8 },
    BlockContext { x: i8, y: i8 },
    Applying,
}

pub struct BlocksScene {
    camera: Camera,
    background: Background,
    scene_title: SceneTitle,
    frame: SubSceneFrame,
    animator: Animator,
    grid_sprite: Sprite,
    grid_start: Vec2,
    grid_increment: Vec2,
    rail_sprite: Sprite,
    apply_text: String,
    information_box_sprite: Sprite,
    information_label: Text,
    information_text: Text,
    input_tracker: UiInputTracker,
    scroll_tracker: ScrollTracker,
    colors: Vec<BlockColor>,
    grid: BlockGrid,
    tracked_invalid: HashSet<(Cow<'static, PackageId>, BlockColor)>,
    arrow: GridArrow,
    block_preview: Option<BlockPreview>,
    grid_context_menu: ContextMenu<GridOption>,
    held_block: Option<InstalledBlock>,
    cycling_variant: bool,
    block_original_variant: usize,
    block_original_rotation: u8,
    cursor: GridCursor,
    block_returns_to_grid: bool,
    state: State,
    textbox: Textbox,
    time: FrameTime,
    list: Vec<ListItem>,
    color_selector: ColorSelector,
    special_hold_time: FrameTime,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    next_scene: NextScene,
}

impl BlocksScene {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // installed blocks
        let global_save = &globals.global_save;
        let restrictions = &globals.restrictions;
        let blocks = global_save.active_blocks().to_vec();

        // track count
        let mut placed_counts = HashMap::new();

        for block in blocks.iter() {
            let id = &block.package_id;

            placed_counts
                .entry((id, block.color))
                .and_modify(|count| *count += 1)
                .or_insert(1);
        }

        // load block packages
        let mut packages: Vec<_> = globals
            .augment_packages
            .packages(PackageNamespace::Local)
            .filter(|package| !package.shapes.is_empty())
            .filter(|package| {
                restrictions.validate_package_tree(game_io, package.package_info.triplet())
            })
            .map(|package| {
                let mut list_item = ListItem::new(game_io, restrictions, package);
                let id = &package.package_info.id;

                for (i, (color, count)) in list_item.colors.iter_mut().enumerate() {
                    let placed_count = placed_counts.get(&(id, *color)).cloned().unwrap_or(0);
                    *count = count.saturating_sub(placed_count);

                    if placed_count > 0 && *count > 0 {
                        list_item.selected_color = i;
                    }
                }

                list_item
            })
            .filter(|list_item| !list_item.colors.is_empty())
            .collect();

        packages.sort_by(|a, b| a.id.cmp(&b.id));

        // load ui sprites
        let mut animator = Animator::load_new(assets, ResourcePaths::BLOCKS_UI_ANIMATION);
        let mut grid_sprite = assets.new_sprite(game_io, ResourcePaths::BLOCKS_UI);

        // color selection
        animator.set_state("DEFAULT");
        let color_selection_step = animator.point_or_zero("COLOR_SELECTION_STEP");

        // grid sprite
        animator.set_state("GRID");
        animator.apply(&mut grid_sprite);

        let grid_start = animator.point_or_zero("GRID_START") - grid_sprite.origin();
        let grid_step = animator.point_or_zero("GRID_STEP");

        // rail sprite
        let mut rail_sprite = grid_sprite.clone();
        animator.set_state("RAIL");
        animator.apply(&mut rail_sprite);

        // information box sprite
        let mut information_box_sprite = grid_sprite.clone();
        animator.set_state("TEXTBOX");
        animator.apply(&mut information_box_sprite);

        let mut information_label = Text::new(game_io, FontName::Context);
        let mut info_label_pos = animator.point_or_zero("LABEL_CENTER") - animator.origin();
        information_label.text = globals.translate("blocks-information-box-label");
        info_label_pos.x -= information_label.measure().size.x * 0.5;
        information_label.style.bounds.set_position(info_label_pos);

        let information_bounds = Rect::from_corners(
            animator.point_or_zero("TEXT_START"),
            animator.point_or_zero("TEXT_END"),
        ) - animator.origin();

        let information_text = Text::new(game_io, FontName::Thin)
            .with_bounds(information_bounds)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR);

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
        let list_next = animator.point_or_zero("STEP");
        let list_start = animator.point_or_zero("START") + list_next;

        scroll_tracker.define_cursor(list_start, list_next.y);

        // cursor
        let cursor_position = scroll_tracker.selected_cursor_position();
        let cursor = GridCursor::new(game_io).with_position(cursor_position);

        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_character_scene(game_io),
            scene_title: SceneTitle::new(game_io, "blocks-scene-title"),
            frame: SubSceneFrame::new(game_io).with_everything(true),
            animator,
            grid_sprite,
            grid_start,
            grid_increment: grid_step,
            rail_sprite,
            apply_text: globals.translate("blocks-apply-label"),
            information_box_sprite,
            information_label,
            information_text,
            input_tracker: UiInputTracker::new(),
            scroll_tracker,
            colors: Vec::new(),
            grid: BlockGrid::new(PackageNamespace::Local).with_blocks(game_io, blocks),
            tracked_invalid: HashSet::new(),
            arrow: GridArrow::new(game_io),
            block_preview: None,
            grid_context_menu: ContextMenu::new(game_io, Default::default(), Vec2::ZERO),
            held_block: None,
            cycling_variant: false,
            block_original_variant: 0,
            block_original_rotation: 0,
            cursor,
            state: State::ListSelection,
            block_returns_to_grid: false,
            textbox: Textbox::new_navigation(game_io).with_position(RESOLUTION_F * 0.5),
            time: 0,
            list: packages,
            color_selector: ColorSelector::new(game_io, color_selection_step),
            special_hold_time: 0,
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
        }
    }

    fn update_invalid(&mut self, game_io: &GameIO) {
        self.tracked_invalid.clear();

        let globals = Globals::from_resources(game_io);
        let restrictions = &globals.restrictions;

        // test ownership
        let mut block_counts = HashMap::new();

        for block in self.grid.installed_blocks() {
            let id = &block.package_id;

            let count = *block_counts
                .entry((id, block.color))
                .and_modify(|count| *count += 1)
                .or_insert(1);

            if count == restrictions.block_count(game_io, id, block.color) + 1 {
                let key = (Cow::Owned(id.clone()), block.color);
                self.tracked_invalid.insert(key);
            }
        }

        // test package validity
        for block in self.grid.installed_blocks() {
            let key = (Cow::Borrowed(&block.package_id), block.color);

            if self.tracked_invalid.contains(&key) {
                // don't need to check the tree if this block is already marked as invalid
                continue;
            }

            let triplet = (
                PackageCategory::Augment,
                PackageNamespace::Local,
                block.package_id.clone(),
            );

            if restrictions.validate_package_tree(game_io, triplet) {
                // valid package, skip
                continue;
            }

            let key = (Cow::Owned(block.package_id.clone()), block.color);
            self.tracked_invalid.insert(key);
        }
    }

    fn update_colors(&mut self) {
        // retain colors that are on the grid

        self.colors.retain(|color| {
            self.grid.installed_blocks().any(|block| {
                let tracked_invalid_key = (Cow::Borrowed(&block.package_id), *color);

                block.color == *color && !self.tracked_invalid.contains(&tracked_invalid_key)
            })
        });

        // add colors missing from our list
        for block in self.grid.installed_blocks() {
            let tracked_invalid_key = (Cow::Borrowed(&block.package_id), block.color);
            let is_valid = !self.tracked_invalid.contains(&tracked_invalid_key);

            if !self.colors.contains(&block.color) && is_valid {
                self.colors.push(block.color);
            }
        }
    }

    fn is_valid(&self, package_id: &PackageId, block_color: BlockColor) -> bool {
        let tracked_invalid_key = (Cow::Borrowed(package_id), block_color);

        !self.tracked_invalid.contains(&tracked_invalid_key)
    }

    fn update_text(&mut self, game_io: &GameIO) {
        self.block_preview = None;

        match self.state {
            State::GridSelection { x, y } => {
                if let Some(block) = self.grid.get_block((x, y)) {
                    let globals = Globals::from_resources(game_io);
                    let packages = &globals.augment_packages;
                    let package = packages
                        .package(PackageNamespace::Local, &block.package_id)
                        .unwrap();

                    self.information_text.text.clear();
                    self.information_text.text.push_str(&package.description);
                    self.information_text.style.color =
                        if self.is_valid(&block.package_id, block.color) {
                            Color::WHITE
                        } else {
                            Color::ORANGE
                        };
                } else {
                    self.information_text.text.clear();
                }
            }
            State::ListSelection => {
                let index = self.scroll_tracker.selected_index();

                if let Some(list_item) = self.list.get(index) {
                    let globals = Globals::from_resources(game_io);
                    let block_color = list_item
                        .colors
                        .get(list_item.selected_color)
                        .unwrap_or(&list_item.colors[0])
                        .0;

                    let package = globals
                        .augment_packages
                        .package(PackageNamespace::Local, &list_item.id)
                        .unwrap();

                    self.information_text.text.clear();
                    self.information_text.text.push_str(&package.description);

                    let is_valid = self.is_valid(&list_item.id, block_color);
                    self.information_text.style.color = if is_valid {
                        Color::WHITE
                    } else {
                        Color::ORANGE
                    };

                    // update block preview
                    self.animator.set_state("OPTION");
                    let list_step = self.animator.point_or_zero("STEP");
                    let mut position = self.animator.point_or_zero("START") + list_step;

                    let index_offset = index - self.scroll_tracker.top_index();
                    position += list_step * index_offset as f32;
                    position += self.animator.point_or_zero("BLOCK_PREVIEW");

                    let mut block_preview = BlockPreview::new(
                        game_io,
                        block_color,
                        package.is_flat,
                        package.shapes.first().copied().unwrap_or_default(),
                    )
                    .with_position(position);

                    if list_item.remaining_colors() > 1 {
                        block_preview.add_multi_color_indicator(game_io);
                    }

                    self.block_preview = Some(block_preview);
                } else {
                    self.information_text.text = String::from("RUN?");
                    self.information_text.style.color = Color::WHITE;
                }
            }
            State::Applying => match self.arrow.status() {
                GridArrowStatus::Block { position, progress } => {
                    let block_name = if let Some(block) = self.grid.get_block(position) {
                        let globals = Globals::from_resources(game_io);
                        let package = globals
                            .augment_packages
                            .package(PackageNamespace::Local, &block.package_id)
                            .unwrap();

                        &package.name
                    } else {
                        "None"
                    };

                    let text_progress = inverse_lerp!(0.0, 0.8, progress);
                    let len = (block_name.len() as f32 * text_progress) as usize;

                    self.information_text.text = format!("RUN...\n{}", &block_name[0..len]);
                }
                _ => {
                    self.information_text.text = String::from("OK");
                }
            },
            _ => {}
        }
    }

    fn open_grid_context_menu(
        &mut self,
        game_io: &mut GameIO,
        grid_y: i8,
        options: &[(&str, GridOption)],
    ) {
        self.grid_context_menu
            .set_and_translate_options(game_io, options);

        self.grid_context_menu.open();

        self.grid_context_menu.update(game_io, &self.input_tracker);

        let menu_size = self.grid_context_menu.bounds().size();
        let mut position = self.cursor.target();
        position.x += (self.grid_increment.x - menu_size.x) * 0.5;

        let min_x = self.grid_increment.x * 0.5;
        let max_x = self.grid_increment.x * (BlockGrid::SIDE_LEN as f32 - 0.5) - menu_size.x;

        position.x = (position.x).clamp(self.grid_start.x + min_x, self.grid_start.x + max_x);

        if grid_y < 4 {
            position.y += self.grid_increment.y * 0.75;
        } else {
            position.y -= menu_size.y + self.grid_increment.y * 0.25;
        }

        self.grid_context_menu.set_position(position);
    }

    fn handle_grid_option(&mut self, game_io: &mut GameIO, x: i8, y: i8, selection: GridOption) {
        match selection {
            GridOption::Move => {
                let block = self.grid.remove_block((x, y)).unwrap();

                self.block_original_variant = block.variant;
                self.block_original_rotation = block.rotation;
                self.held_block = Some(block);
                self.block_returns_to_grid = true;
            }
            GridOption::Remove => {
                let block = self.grid.remove_block((x, y)).unwrap();

                self.uninstall(game_io, block);
            }
            GridOption::More => {
                self.grid_context_menu.set_and_translate_options(
                    game_io,
                    &[
                        ("augments-option-export", GridOption::Export),
                        ("augments-option-import", GridOption::Import),
                        ("augments-option-clear", GridOption::Clear),
                    ],
                );
            }
            GridOption::Export => {
                let text = self.grid.export_string(game_io);

                let copied = game_io.input_mut().set_clipboard_text(text);

                let globals = Globals::from_resources(game_io);
                let message = if copied {
                    globals.translate("copied-to-clipboard")
                } else {
                    globals.translate("copy-to-clipboard-failed")
                };
                let interface = TextboxMessage::new(message);

                self.textbox.push_interface(interface);
                self.textbox.open();
            }
            GridOption::Import => {
                let text = game_io.input_mut().request_clipboard_text();

                let blocks = BlockGrid::import_string(&text);

                if let Some(blocks) = blocks {
                    self.clear_grid(game_io);

                    for block in blocks {
                        let Ok(i) = self
                            .list
                            .binary_search_by(|package| package.id.cmp(&block.package_id))
                        else {
                            continue;
                        };

                        let list_item = &self.list[i];
                        let Some(color_index) =
                            list_item.colors.iter().position(|(c, _)| *c == block.color)
                        else {
                            continue;
                        };

                        if self.take_block_from_list(i, color_index).is_none() {
                            continue;
                        }

                        let conflicts = self.grid.install_block(game_io, block);

                        if conflicts.is_some() {
                            continue;
                        }
                    }
                } else {
                    let globals = Globals::from_resources(game_io);

                    let message = globals.translate("clipboard-read-failed");
                    let interface = TextboxMessage::new(message);

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
            }
            GridOption::Clear => {
                let globals = Globals::from_resources(game_io);
                let question = globals.translate("blocks-clear-question");

                let event_sender = self.event_sender.clone();
                let interface = TextboxQuestion::new(game_io, question, move |response| {
                    if response {
                        let _ = event_sender.send(Event::Clear);
                    }
                });

                self.textbox.push_interface(interface);
                self.textbox.open();
            }
        }

        if selection != GridOption::More {
            self.grid_context_menu.close();
            self.state = State::GridSelection { x, y };
        }
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        let globals = Globals::from_resources(game_io);

        let prev_state = self.state;
        let prev_held = self.held_block.is_some();

        if let Some(block) = &mut self.held_block {
            let prev_variant = block.variant;
            let prev_rotation = block.rotation;

            let input = InputUtil::new(game_io);
            let holding_shoudler_l = input.is_down(Input::ShoulderL);
            let holding_shoudler_r = input.is_down(Input::ShoulderR);

            if (holding_shoudler_l && input.was_just_pressed(Input::ShoulderR))
                || (holding_shoudler_r && input.was_just_pressed(Input::ShoulderL))
            {
                let total_variants = globals
                    .augment_packages
                    .package(PackageNamespace::Local, &block.package_id)
                    .map(|package| package.shapes.len())
                    .unwrap_or(1);

                if total_variants > 1 {
                    block.variant += 1;
                    block.variant %= total_variants;
                } else {
                    globals.audio.play_sound_with_behavior(
                        &globals.sfx.cursor_error,
                        AudioBehavior::NoOverlap,
                    );
                }

                self.cycling_variant = true;
            } else if self.cycling_variant {
                if !holding_shoudler_l && !holding_shoudler_r {
                    self.cycling_variant = false;
                }
            } else if input.was_released(Input::ShoulderL) && !holding_shoudler_r {
                block.rotate_cc();
            } else if input.was_released(Input::ShoulderR) && !holding_shoudler_l {
                block.rotate_c();
            }

            if block.rotation != prev_rotation || block.variant != prev_variant {
                globals.audio.play_sound(&globals.sfx.cursor_move);
            }

            if self.input_tracker.pulsed(Input::Confirm) {
                let mut clone = block.clone();

                if let State::GridSelection { x, y } = self.state {
                    clone.position = (x, y);
                }

                let success = self.grid.install_block(game_io, clone).is_none();

                if success {
                    globals.audio.play_sound(&globals.sfx.cursor_select);
                    self.held_block = None;
                    self.update_colors();
                } else {
                    globals.audio.play_sound(&globals.sfx.cursor_error);
                }
            } else if self.input_tracker.pulsed(Input::Cancel) {
                let mut block = self.held_block.take().unwrap();

                if self.block_returns_to_grid {
                    block.variant = self.block_original_variant;
                    block.rotation = self.block_original_rotation;

                    let (x, y) = block.position;
                    self.state = State::GridSelection { x, y };

                    self.grid.install_block(game_io, block);
                } else {
                    self.uninstall(game_io, block);

                    self.state = State::ListSelection;
                }

                globals.audio.play_sound(&globals.sfx.cursor_cancel);
            }
        }

        match self.state {
            State::ListSelection => {
                let input_util = InputUtil::new(game_io);

                if input_util.is_down(Input::Special) {
                    self.special_hold_time += 1;
                } else {
                    self.special_hold_time = 0;
                }

                if self.input_tracker.pulsed(Input::Cancel) && !prev_held {
                    let event_sender = self.event_sender.clone();

                    let question = TextboxQuestion::new(
                        game_io,
                        globals.translate("blocks-leave-question"),
                        move |yes| {
                            if yes {
                                event_sender.send(Event::Leave).unwrap();
                            }
                        },
                    );

                    self.textbox.push_interface(question);
                    self.textbox.open();
                } else if self.input_tracker.pulsed(Input::Left) {
                    let y = self.scroll_tracker.selected_index() - self.scroll_tracker.top_index();

                    self.state = State::GridSelection {
                        x: 5,
                        y: y.max(1) as _,
                    };

                    globals.audio.play_sound(&globals.sfx.cursor_select);
                } else {
                    // handle scrolling
                    let prev_index = self.scroll_tracker.selected_index();

                    self.scroll_tracker
                        .handle_vertical_input(&self.input_tracker);

                    if self.input_tracker.pulsed(Input::End) {
                        let last_index = self.scroll_tracker.total_items() - 1;
                        self.scroll_tracker.set_selected_index(last_index);
                    }

                    // sfx
                    let selected_index = self.scroll_tracker.selected_index();

                    if prev_index != selected_index {
                        globals.audio.play_sound(&globals.sfx.cursor_move);

                        self.update_text(game_io);
                    }

                    // handle selection
                    if self.input_tracker.pulsed(Input::Confirm) {
                        if self.list.get(selected_index).is_some() {
                            // selected a block
                            self.take_selected_block_from_list();
                            globals.audio.play_sound(&globals.sfx.cursor_select);
                        } else {
                            // selected APPLY
                            self.state = State::Applying;

                            globals.audio.play_sound(&globals.sfx.customize_start);

                            // save blocks
                            let globals = Globals::from_resources_mut(game_io);
                            let global_save = &mut globals.global_save;

                            global_save.installed_blocks.insert(
                                global_save.selected_character.clone(),
                                self.grid.installed_blocks().cloned().collect(),
                            );

                            global_save.character_update_times.insert(
                                global_save.selected_character.clone(),
                                GlobalSave::current_time(),
                            );

                            global_save.save();
                        }
                    } else if input_util.was_released(Input::Special) {
                        // cycle color
                        if let Some(list_item) = self.list.get_mut(selected_index) {
                            if list_item.select_next_color() {
                                self.update_text(game_io);

                                globals.audio.play_sound(&globals.sfx.cursor_move);
                            } else {
                                globals.audio.play_sound_with_behavior(
                                    &globals.sfx.cursor_error,
                                    AudioBehavior::NoOverlap,
                                );
                            }
                        }
                    } else if self.special_hold_time > COLOR_UI_DELAY {
                        // try open the color selector
                        if let Some(list_item) = self.list.get(selected_index) {
                            self.color_selector.load_list_item(game_io, list_item);
                            self.state = State::ColorSelection;
                            globals.audio.play_sound(&globals.sfx.cursor_select);
                        }
                    }
                }
            }
            State::ColorSelection => {
                self.color_selector.update(game_io, &self.input_tracker);

                if let Some(list_item) = self.list.get_mut(self.scroll_tracker.selected_index()) {
                    list_item.selected_color = self.color_selector.scroll_tracker.selected_index();
                }

                if self.input_tracker.pulsed(Input::Confirm) {
                    if self.take_selected_block_from_list() {
                        globals.audio.play_sound(&globals.sfx.cursor_select);
                    } else {
                        globals.audio.play_sound(&globals.sfx.cursor_error);
                    }
                } else if self.input_tracker.pulsed(Input::Cancel) {
                    self.state = State::ListSelection;
                    globals.audio.play_sound(&globals.sfx.cursor_cancel);
                }
            }
            State::GridSelection { x: old_x, y: old_y } => {
                let cancel = self.input_tracker.pulsed(Input::Cancel) && !prev_held;
                let returned_to_list = self.input_tracker.pulsed(Input::Right) && old_x == 6;
                let pressed_end = self.input_tracker.pulsed(Input::End);
                let has_block = self.grid.get_block((old_x, old_y)).is_some();

                if (cancel || returned_to_list || pressed_end) && self.held_block.is_none() {
                    self.state = State::ListSelection;

                    if pressed_end {
                        let last_index = self.scroll_tracker.total_items() - 1;
                        self.scroll_tracker.set_selected_index(last_index);
                    }

                    globals.audio.play_sound(&globals.sfx.cursor_cancel);
                } else if self.input_tracker.pulsed(Input::Confirm) && !prev_held {
                    self.state = State::BlockContext { x: old_x, y: old_y };

                    let options: &[(&str, GridOption)] = if has_block {
                        if cfg_android!() {
                            &[
                                ("blocks-option-move", GridOption::Move),
                                ("blocks-option-remove", GridOption::Remove),
                                ("augments-option-clear", GridOption::Clear),
                            ]
                        } else {
                            &[
                                ("blocks-option-move", GridOption::Move),
                                ("blocks-option-remove", GridOption::Remove),
                                ("blocks-option-more", GridOption::More),
                            ]
                        }
                    } else if cfg_android!() {
                        &[("augments-option-clear", GridOption::Clear)]
                    } else {
                        &[
                            ("augments-option-export", GridOption::Export),
                            ("augments-option-import", GridOption::Import),
                            ("augments-option-clear", GridOption::Clear),
                        ]
                    };

                    self.open_grid_context_menu(game_io, old_y, options);
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
                        globals.audio.play_sound(&globals.sfx.cursor_move);
                    }
                }
            }
            State::BlockContext { x, y } => {
                let selection = self.grid_context_menu.update(game_io, &self.input_tracker);

                if let Some(op) = selection {
                    self.handle_grid_option(game_io, x, y, op);
                } else if !self.grid_context_menu.is_open() {
                    self.state = State::GridSelection { x, y };
                }
            }
            State::Applying => {
                let prev_status = self.arrow.status();

                self.arrow.update();

                let current_status = self.arrow.status();

                if prev_status != current_status {
                    self.update_text(game_io);

                    // sfx
                    match current_status {
                        GridArrowStatus::Block { position, progress } => {
                            if progress == 0.0 {
                                if self.grid.get_block(position).is_some() {
                                    globals.audio.play_sound(&globals.sfx.customize_block);
                                } else {
                                    globals.audio.play_sound(&globals.sfx.customize_empty);
                                }
                            }
                        }
                        GridArrowStatus::Fading => {
                            globals.audio.play_sound(&globals.sfx.customize_complete);
                        }
                        _ => {}
                    }
                }

                if self.arrow.status() == GridArrowStatus::Complete {
                    let success_message = globals.translate("blocks-success");
                    let interface = TextboxMessage::new(success_message);
                    self.textbox.push_interface(interface);

                    let event_sender = self.event_sender.clone();
                    let question = globals.translate("blocks-leave-question");
                    let interface = TextboxQuestion::new(game_io, question, move |yes| {
                        if yes {
                            event_sender.send(Event::Leave).unwrap();
                        } else {
                            event_sender.send(Event::Applied).unwrap();
                        }
                    });
                    self.textbox.push_interface(interface);

                    self.textbox.open();
                }
            }
        }

        if self.state != prev_state || prev_held != self.held_block.is_some() {
            self.update_cursor_sprite();
            self.update_text(game_io);
            self.special_hold_time = 0;
        }
    }

    fn take_selected_block_from_list(&mut self) -> bool {
        let selected_index = self.scroll_tracker.selected_index();

        let Some(list_item) = self.list.get_mut(selected_index) else {
            return false;
        };

        let color_index = list_item.selected_color;
        let package_id = list_item.id.clone();

        let Some(color) = self.take_block_from_list(selected_index, color_index) else {
            return false;
        };

        self.block_returns_to_grid = false;
        self.held_block = Some(InstalledBlock {
            package_id,
            variant: 0,
            rotation: 0,
            color,
            position: (0, 0),
        });
        self.state = State::GridSelection { x: 2, y: 2 };

        true
    }

    fn take_block_from_list(&mut self, index: usize, color_index: usize) -> Option<BlockColor> {
        let list_item = self.list.get_mut(index)?;
        let (color, count) = list_item.colors.get_mut(color_index)?;

        if *count == 0 {
            return None;
        }

        *count -= 1;
        let new_count = *count;
        let color = *color;

        if list_item.colors.iter().all(|(_, count)| *count == 0) {
            self.list.remove(index);
            self.scroll_tracker.set_total_items(self.list.len() + 1);
        } else if new_count == 0 {
            list_item.select_next_color();
        }

        Some(color)
    }

    fn uninstall(&mut self, game_io: &GameIO, block: InstalledBlock) {
        let tracked_invalid_key = (Cow::Borrowed(&block.package_id), block.color);

        if self.tracked_invalid.contains(&tracked_invalid_key) {
            self.update_invalid(game_io);

            // drop invalid blocks
            return;
        }

        let index = self
            .list
            .binary_search_by(|package| package.id.cmp(&block.package_id));

        match index {
            Ok(index) => {
                let list_item = &mut self.list[index];
                let mut colors_iter = list_item.colors.iter_mut();

                if let Some(i) = colors_iter.position(|(color, _)| *color == block.color) {
                    let (_, count) = &mut list_item.colors[i];
                    *count += 1;

                    list_item.selected_color = i;
                }
            }
            Err(index) => {
                let globals = Globals::from_resources(game_io);
                let package = globals
                    .augment_packages
                    .package(PackageNamespace::Local, &block.package_id)
                    .unwrap();

                let mut list_item = ListItem::new(game_io, &globals.restrictions, package);

                for (i, (color, count)) in list_item.colors.iter_mut().enumerate() {
                    if *color == block.color {
                        *count = 1;
                        list_item.selected_color = i;
                    } else {
                        *count = 0;
                    }
                }

                self.list.insert(index, list_item);
                self.scroll_tracker.set_total_items(self.list.len() + 1);
            }
        };

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
            State::ColorSelection | State::Applying => {
                self.cursor.hide();
            }
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
                    let globals = Globals::from_resources(game_io);
                    globals.audio.pop_music_stack();
                    globals.audio.restart_music();

                    let transition = crate::transitions::new_sub_scene_pop(game_io);
                    self.next_scene = NextScene::new_pop().with_transition(transition);
                }
                Event::Applied => {
                    self.arrow.reset();
                    self.state = State::ListSelection;
                    self.update_cursor_sprite();
                    self.update_text(game_io);
                }
                Event::Clear => {
                    self.clear_grid(game_io);
                    self.update_cursor_sprite();
                }
            }
        }
    }

    fn clear_grid(&mut self, game_io: &GameIO) {
        let blocks: Vec<_> = self.grid.installed_blocks().cloned().collect();

        for block in blocks {
            self.grid.remove_block(block.position);
            self.uninstall(game_io, block);
        }
    }

    fn resolve_flashing_blocks(&self) -> Vec<(i8, i8)> {
        if self.state != State::Applying {
            return Vec::new();
        }

        let (end_x, y) = self.arrow.current_block();

        (0..=end_x)
            .flat_map(|x| self.grid.get_block((x, y)))
            .map(|block| block.position)
            .collect()
    }
}

impl Scene for BlocksScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.update_text(game_io);
        self.update_grid_sprite();
        self.update_colors();
        self.update_invalid(game_io);

        let globals = Globals::from_resources(game_io);
        globals.audio.push_music_stack();
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.input_tracker.update(game_io);
        self.time += 1;

        self.textbox.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        if !game_io.is_in_transition() && !self.textbox.is_open() {
            self.handle_input(game_io);

            let globals = Globals::from_resources(game_io);

            if !globals.audio.is_music_playing() {
                globals.audio.pick_music(&globals.music.customize, true);
            }
        }

        self.update_cursor();
        self.handle_events(game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = Globals::from_resources(game_io);

        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw grid
        sprite_queue.draw_sprite(&self.grid_sprite);

        // draw colors
        let default_block_color = if !matches!(self.state, State::GridSelection { .. }) {
            DIM_COLOR
        } else {
            Color::WHITE
        };

        let mut color_sprite = Sprite::new(game_io, self.grid_sprite.texture().clone());
        color_sprite.set_color(default_block_color);

        self.animator.set_state("GRID");
        let mut color_position =
            self.animator.point_or_zero("COLORS_START") - self.grid_sprite.origin();
        let color_step = self.animator.point_or_zero("COLORS_STEP");

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
        let flashing_blocks = self.resolve_flashing_blocks();

        for y in 0..BlockGrid::SIDE_LEN {
            for x in 0..BlockGrid::SIDE_LEN {
                let Some(block) = self.grid.get_block((x, y)) else {
                    continue;
                };

                if flashing_blocks.contains(&block.position) {
                    block_sprite.set_color(Color::WHITE);
                } else {
                    block_sprite.set_color(default_block_color);
                }

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
                    let packages = &globals.augment_packages;
                    let package = packages
                        .package(PackageNamespace::Local, &block.package_id)
                        .unwrap();

                    if package.is_flat {
                        self.animator.set_state(block.color.flat_state());
                    } else {
                        self.animator.set_state(block.color.plus_state());
                    }
                }

                self.animator.apply(&mut block_sprite);
                sprite_queue.draw_sprite(&block_sprite);
            }
        }

        // draw rail on top of blocks
        sprite_queue.draw_sprite(&self.rail_sprite);

        // draw a border around invalid blocks, recycling the block sprite
        self.animator.set_state("INVALID_FRAME");
        self.animator.apply(&mut block_sprite);

        for y in 1..BlockGrid::SIDE_LEN - 1 {
            for x in 1..BlockGrid::SIDE_LEN - 1 {
                let Some(block) = self.grid.get_block((x, y)) else {
                    continue;
                };

                let tracked_invalid_key = (Cow::Borrowed(&block.package_id), block.color);

                if !self.tracked_invalid.contains(&tracked_invalid_key) {
                    continue;
                }

                let position =
                    Vec2::new(x as f32, y as f32) * self.grid_increment + self.grid_start;
                block_sprite.set_position(position);

                sprite_queue.draw_sprite(&block_sprite);
            }
        }

        // draw held blocks
        if let Some(block) = &self.held_block {
            let packages = &globals.augment_packages;
            let package = packages
                .package(PackageNamespace::Local, &block.package_id)
                .unwrap();

            // create sprite
            block_sprite.set_color(Color::WHITE.multiply_alpha(0.5));

            if package.is_flat {
                self.animator.set_state(block.color.flat_held_state());
            } else {
                self.animator.set_state(block.color.plus_held_state());
            }

            self.animator.apply(&mut block_sprite);

            // loop over shape and draw blocks under cursor
            let cursor_position = self.cursor.position();

            for y in 0..BlockGrid::SIDE_LEN {
                for x in 0..BlockGrid::SIDE_LEN {
                    if !package.exists_at(block.variant, block.rotation, (x, y)) {
                        continue;
                    }

                    let offset = (Vec2::new(x as f32, y as f32) - BlockShape::CENTER_OFFSET as f32)
                        * self.grid_increment;
                    block_sprite.set_position(cursor_position + offset);
                    sprite_queue.draw_sprite(&block_sprite);
                }
            }
        }

        // draw grid arrow
        self.arrow.draw(&mut sprite_queue);

        // draw cursor
        self.cursor.draw(&mut sprite_queue);

        // draw package list
        let mut recycled_sprite = Sprite::new(game_io, self.grid_sprite.texture().clone());
        let selected_index = if self.state == State::ListSelection {
            Some(self.scroll_tracker.selected_index())
        } else {
            None
        };

        self.animator.set_state("OPTION");
        let mut offset = self.animator.point_or_zero("START");
        let offset_jump = self.animator.point_or_zero("STEP");

        let text_bounds = Rect::from_corners(
            self.animator.point_or_zero("TEXT_START"),
            self.animator.point_or_zero("TEXT_END"),
        );
        let mut text_style = TextStyle::new(game_io, FontName::Thick)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
            .with_bounds(text_bounds);

        if self.scroll_tracker.top_index() == 0 {
            offset += offset_jump;
        }

        text_style.bounds += offset - offset_jump;

        self.animator.set_state("OPTION");
        let count_offset = self.animator.point_or_zero("COUNT_RIGHT");
        let mut count_text_style = TextStyle::new_monospace(game_io, FontName::DuplicateCount);
        count_text_style.letter_spacing = 0.0;

        for i in self.scroll_tracker.view_range() {
            recycled_sprite.set_position(offset);
            text_style.bounds += offset_jump;
            offset += offset_jump;

            if i == self.list.len() {
                // draw APPLY button
                if Some(i) == selected_index {
                    self.animator.set_state("APPLY_BLINK");
                    self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                    self.animator.sync_time(self.time);
                } else {
                    self.animator.set_state("APPLY");
                }

                self.animator.apply(&mut recycled_sprite);
                sprite_queue.draw_sprite(&recycled_sprite);

                let text_width = text_style.measure(&self.apply_text).size.x;
                text_style.bounds += Vec2::new((text_bounds.width - text_width) * 0.5, 0.0);
                text_style.monospace = true;
                text_style.draw(game_io, &mut sprite_queue, &self.apply_text);
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

            // draw name
            let list_item = &self.list[i];
            text_style.draw(game_io, &mut sprite_queue, &list_item.name);

            // draw count
            let count = list_item.colors[list_item.selected_color].1;

            if count > 1 {
                let text = format!("x{count}");

                let mut position = recycled_sprite.position() + count_offset;
                position.x -= count_text_style.measure(&text).size.x;
                count_text_style.bounds.set_position(position);

                count_text_style.draw(game_io, &mut sprite_queue, &text);
            }
        }

        // draw the information box
        sprite_queue.draw_sprite(&self.information_box_sprite);
        self.information_label.draw(game_io, &mut sprite_queue);
        self.information_text.draw(game_io, &mut sprite_queue);

        // draw context menus
        self.grid_context_menu.draw(game_io, &mut sprite_queue);

        // draw frame
        self.frame.draw(&mut sprite_queue);
        self.scene_title.draw(game_io, &mut sprite_queue);

        // draw block preview
        if let Some(preview) = &mut self.block_preview {
            preview.draw(&mut sprite_queue);
        }

        if self.state == State::ColorSelection {
            self.color_selector.draw(game_io, &mut sprite_queue);
        }

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

struct ColorSelector {
    scroll_tracker: GridScrollTracker,
    items: Vec<(BlockPreview, String)>, // preview, count string
    step: Vec2,
}

impl ColorSelector {
    fn new(game_io: &GameIO, step: Vec2) -> Self {
        Self {
            scroll_tracker: GridScrollTracker::new(game_io, 0, 0).with_step(step),
            items: Default::default(),
            step,
        }
    }

    fn load_list_item(&mut self, game_io: &GameIO, list_item: &ListItem) {
        // update scroll trackers
        let len = list_item.colors.len();
        let mut side_len = len.isqrt();

        if side_len * side_len < len {
            // aim for a perfect square, but make sure we can fit every option
            side_len += 1;
        }

        self.scroll_tracker.set_view_size(side_len, side_len);
        self.scroll_tracker.set_total_items(len);
        self.scroll_tracker
            .set_selected_index(list_item.selected_color);

        // load previews
        let globals = Globals::from_resources(game_io);
        let package = globals
            .augment_packages
            .package(PackageNamespace::Local, &list_item.id)
            .unwrap();

        let grid_size = (Vec2::new(side_len as f32, len.div_ceil(side_len) as f32)) * self.step;
        let top_left = (RESOLUTION_F - grid_size) * 0.5;

        let colors_iter = list_item.colors.iter().enumerate();
        self.items.clear();
        self.items.extend(colors_iter.map(|(i, &(color, count))| {
            let mut preview = BlockPreview::new(
                game_io,
                color,
                package.is_flat,
                package.shapes.first().copied().unwrap_or_default(),
            );

            let mut offset = Vec2::new((i % side_len) as f32, (i / side_len) as f32) * self.step;
            let preview_padding = (self.step - preview.size()) * 0.5;
            offset += preview_padding;

            preview.set_position(top_left + offset);

            if i == 0 {
                // resolve the first cursor position using the position and size of the first preview
                self.scroll_tracker.set_position(top_left);

                let cursor_offset = Vec2::new(-8.0, preview.size().y * 0.25) + preview_padding;
                self.scroll_tracker.define_cursor(cursor_offset);
            }

            if count == 0 {
                preview.set_tint(Color::WHITE.multiply_color(0.5));
            }

            (preview, format!("x{count}"))
        }));
    }

    fn update(&mut self, game_io: &GameIO, input_tracker: &UiInputTracker) {
        let prev_index = self.scroll_tracker.selected_index();
        self.scroll_tracker.handle_input(input_tracker);

        let new_index = self.scroll_tracker.selected_index();

        if prev_index == new_index {
            return;
        }

        let globals = Globals::from_resources(game_io);
        globals.audio.play_sound(&globals.sfx.cursor_move);
    }

    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        // shade behind the selector
        let mut shade = assets.new_sprite(game_io, ResourcePaths::PIXEL);
        shade.set_size(RESOLUTION_F);
        shade.set_color(Color::BLACK.multiply_alpha(0.5));
        sprite_queue.draw_sprite(&shade);

        // draw list items
        let mut text_style = TextStyle::new_monospace(game_io, FontName::DuplicateCount);
        text_style.letter_spacing = 0.0;

        for (preview, count_string) in &mut self.items {
            preview.draw(sprite_queue);

            let preview_size = preview.size();
            let text_size = text_style.measure(count_string).size;

            let text_offset = Vec2::new((preview_size.x - text_size.x) * 0.5, preview_size.y + 1.0);
            let text_position = preview.position() + text_offset;
            text_style.bounds.set_position(text_position);

            text_style.draw(game_io, sprite_queue, count_string);
        }

        self.scroll_tracker.draw_cursor(sprite_queue);
    }
}
