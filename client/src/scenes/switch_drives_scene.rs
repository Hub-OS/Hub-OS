use crate::bindable::SpriteColorMode;
use crate::packages::{AugmentPackage, PackageId, PackageNamespace};
use crate::render::ui::{
    ContextMenu, FontName, SceneTitle, ScrollTracker, SubSceneFrame, Text, TextStyle, Textbox,
    TextboxMessage, TextboxQuestion, UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, FrameTime, SpriteColorQueue};
use crate::resources::*;
use crate::saves::{GlobalSave, InstalledSwitchDrive};
use enum_map::{Enum, EnumMap, enum_map};
use framework::prelude::*;
use itertools::Itertools;
use packets::structures::SwitchDriveSlot;
use std::borrow::Cow;
use std::collections::HashSet;

#[derive(Clone, Copy, PartialEq, Eq)]
enum ContextOption {
    Export,
    Import,
    Clear,
}

enum Event {
    Leave,
    AddSwitchDrive,
    RemoveSwitchDrive,
    ApplyFilter(Option<SwitchDriveSlot>),
    Clear,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum State {
    ListSelection,
    EquipmentSelection,
}

struct SlotUi {
    name_text: Text,
    body_part_sprite: Sprite,
    sprite: Sprite,
    base_state: &'static str,
    selected_state: &'static str,
    highlighted_state: &'static str,
    selected: bool,
    time: FrameTime,
    package_id: Option<PackageId>,
}

impl SlotUi {
    fn new_left(game_io: &GameIO, animator: &mut Animator, part_name: &str, offset: Vec2) -> Self {
        Self::new(
            game_io,
            animator,
            "LEFT_SLOT",
            "LEFT_SLOT_SELECTED",
            "LEFT_SLOT_HIGHLIGHTED",
            part_name,
            offset,
        )
    }

    fn new_right(game_io: &GameIO, animator: &mut Animator, part_name: &str, offset: Vec2) -> Self {
        Self::new(
            game_io,
            animator,
            "RIGHT_SLOT",
            "RIGHT_SLOT_SELECTED",
            "RIGHT_SLOT_HIGHLIGHTED",
            part_name,
            offset,
        )
    }

    fn new(
        game_io: &GameIO,
        animator: &mut Animator,
        base_state: &'static str,
        selected_state: &'static str,
        highlighted_state: &'static str,
        part_name: &str,
        offset: Vec2,
    ) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::SWITCH_DRIVE_UI);

        let point = animator.point_or_zero(part_name) + offset;

        // save state
        let old_state = animator.current_state().unwrap_or_default().to_string();

        // body sprite
        let mut body_part_sprite = sprite.clone();
        body_part_sprite.set_position(point);
        animator.set_state(part_name);
        animator.apply(&mut body_part_sprite);

        // apply base state
        animator.set_state(base_state);
        animator.apply(&mut sprite);
        sprite.set_position(point);

        // resolve name position
        let name_bounds = Rect::from_corners(
            animator.point_or_zero("NAME_START"),
            animator.point_or_zero("NAME_END"),
        ) - animator.origin()
            + point;

        let mut name_text = Text::new_monospace(game_io, FontName::ThinSmall);
        name_text.style.bounds = name_bounds;

        // revert state
        animator.set_state(&old_state);

        Self {
            name_text,
            body_part_sprite,
            sprite,
            base_state,
            selected_state,
            highlighted_state,
            selected: false,
            time: 0,
            package_id: None,
        }
    }

    fn package_name(&self) -> &str {
        &self.name_text.text
    }

    fn set_selected(&mut self, selected: bool) {
        if self.selected != selected {
            self.selected = selected;
            self.time = 0;
        }
    }

    fn set_package(&mut self, package: Option<&AugmentPackage>) {
        if let Some(package) = package {
            self.package_id = Some(package.package_info.id.clone());
            self.name_text.text.clear();
            self.name_text.text.push_str(&package.name);
        } else {
            self.package_id = None;
            self.name_text.text.clear();
        }
    }

    fn update(&mut self, scene_state: State, animator: &mut Animator) {
        let state = if self.selected {
            if scene_state == State::EquipmentSelection {
                self.selected_state
            } else {
                self.highlighted_state
            }
        } else {
            self.base_state
        };

        animator.set_state(state);
        animator.sync_time(self.time);
        animator.apply(&mut self.sprite);

        let color = Color::WHITE.multiply_alpha((self.time as f32 * 0.08).sin() * 0.5 + 0.5);
        self.body_part_sprite.set_color(color);

        self.time += 1;
    }

    fn draw_body_part(&self, sprite_queue: &mut SpriteColorQueue) {
        if self.selected {
            sprite_queue.draw_sprite(&self.body_part_sprite);
        }
    }

    fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        sprite_queue.draw_sprite(&self.sprite);
        self.name_text.draw(game_io, sprite_queue);
    }
}

pub struct SwitchDrivesScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    scene_title: SceneTitle,
    animator: Animator,
    cursor_time: FrameTime,
    cursor_sprite: Sprite,
    equipment_sprite: Sprite,
    info_box_sprite: Sprite,
    info_text_style: TextStyle,
    equipment_map: EnumMap<SwitchDriveSlot, SlotUi>,
    input_tracker: UiInputTracker,
    list_scroll_tracker: ScrollTracker,
    equipment_scroll_tracker: ScrollTracker,
    tracked_invalid: HashSet<(Cow<'static, PackageId>, Option<SwitchDriveSlot>)>,
    state: State,
    textbox: Textbox,
    time: FrameTime,
    package_ids: Vec<PackageId>,
    filtered_packages: bool,
    context_menu: ContextMenu<ContextOption>,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    next_scene: NextScene,
}

impl SwitchDrivesScene {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = Globals::from_resources(game_io);
        let assets = &globals.assets;
        let global_save = &globals.global_save;

        // load drive part packages
        let package_ids = collect_drive_package_ids(game_io, globals, None);

        let mut animator = Animator::load_new(assets, ResourcePaths::SWITCH_DRIVE_UI_ANIMATION);

        // layout
        animator.set_state("DEFAULT");
        let equipment_position = animator.point_or_zero("EQUIPMENT");
        let info_box_position = animator.point_or_zero("TEXTBOX");

        let mut equipment_sprite = assets.new_sprite(game_io, ResourcePaths::SWITCH_DRIVE_UI);

        animator.set_state("EQUIPMENT");
        animator.apply(&mut equipment_sprite);
        equipment_sprite.set_position(equipment_position);

        let mut slot_map = enum_map! {
            SwitchDriveSlot::Head => SlotUi::new_left(game_io, &mut animator, "HEAD", equipment_position),
            SwitchDriveSlot::Body => SlotUi::new_left(game_io, &mut animator, "BODY", equipment_position),
            SwitchDriveSlot::Arms => SlotUi::new_right(game_io, &mut animator, "ARMS", equipment_position),
            SwitchDriveSlot::Legs => SlotUi::new_right(game_io, &mut animator, "LEGS", equipment_position),
        };

        for drive_part in global_save.active_drive_parts() {
            let package = get_package(globals, &drive_part.package_id);
            slot_map[drive_part.get_slot()].set_package(package);
        }

        let mut info_box_sprite = equipment_sprite.clone();
        animator.set_state("TEXTBOX");
        animator.apply(&mut info_box_sprite);
        info_box_sprite.set_position(info_box_position);

        let info_bounds = Rect::from_corners(
            animator.point_or_zero("TEXT_START"),
            animator.point_or_zero("TEXT_END"),
        ) - animator.origin()
            + info_box_position;

        let info_text_style = TextStyle::new(game_io, FontName::Thin)
            .with_bounds(info_bounds)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
            .with_color(Color::WHITE);

        // cursor sprite
        let cursor_sprite = equipment_sprite.clone();

        // scroll tracker
        let list_scroll_tracker = ScrollTracker::new(game_io, 4)
            .with_view_margin(1)
            .with_total_items(package_ids.len())
            .with_custom_cursor(
                game_io,
                ResourcePaths::TEXTBOX_CURSOR_ANIMATION,
                ResourcePaths::TEXTBOX_CURSOR,
            );

        let equipment_scroll_tracker = ScrollTracker::new(game_io, 4)
            .with_view_margin(1)
            .with_total_items(4)
            .with_custom_cursor(
                game_io,
                ResourcePaths::TEXTBOX_CURSOR_ANIMATION,
                ResourcePaths::TEXTBOX_CURSOR,
            );

        let mut context_menu =
            ContextMenu::new_translated(game_io, "switch-drives-scene-title", Vec2::ZERO)
                .with_translated_options(
                    game_io,
                    &[
                        ("augments-option-export", ContextOption::Export),
                        ("augments-option-import", ContextOption::Import),
                        ("augments-option-clear", ContextOption::Clear),
                    ],
                );
        context_menu.recalculate_layout(game_io);
        context_menu.set_top_center(RESOLUTION_F * Vec2::new(0.5, 0.25));

        let (event_sender, event_receiver) = flume::unbounded();

        let mut scene = Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_character_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_everything(true),
            scene_title: SceneTitle::new(game_io, "switch-drives-scene-title"),
            animator,
            cursor_sprite,
            cursor_time: 0,
            equipment_sprite,
            info_box_sprite,
            info_text_style,
            equipment_map: slot_map,
            input_tracker: UiInputTracker::new(),
            list_scroll_tracker,
            equipment_scroll_tracker,
            tracked_invalid: HashSet::new(),
            state: if package_ids.is_empty() {
                State::EquipmentSelection
            } else {
                State::ListSelection
            },
            textbox: Textbox::new_navigation(game_io).with_position(RESOLUTION_F * 0.5),
            time: 0,
            package_ids,
            filtered_packages: false,
            context_menu,
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
        };

        scene.update_selection_ui(game_io);

        scene
    }

    fn add_drive_part(&mut self, game_io: &mut GameIO, index: usize) -> bool {
        if index >= self.package_ids.len() {
            return false;
        }

        let mut add_part = || {
            let package_id = self.package_ids.remove(index);

            // actual placement
            let globals = Globals::from_resources(game_io);

            let Some(package) = globals
                .augment_packages
                .package(PackageNamespace::Local, &package_id)
            else {
                return false;
            };

            let Some(slot) = package.slot else {
                return false;
            };

            let slot_ui = &mut self.equipment_map[slot];

            // move drive into package_ids list
            if let Some(prev_package_id) = slot_ui.package_id.take()
                && let Err(index) = self.package_ids.binary_search(&prev_package_id)
            {
                self.package_ids.insert(index, prev_package_id);
            }

            slot_ui.set_package(Some(package));

            self.state = State::EquipmentSelection;
            self.equipment_scroll_tracker
                .set_selected_index(slot as usize);

            true
        };

        let result = add_part();

        // update list tracking
        let list_size = self.package_ids.len();
        self.list_scroll_tracker.set_total_items(list_size);

        result
    }

    fn request_leave(&mut self, game_io: &GameIO) {
        let event_sender = self.event_sender.clone();

        let globals = Globals::from_resources(game_io);

        let question = TextboxQuestion::new(
            game_io,
            globals.translate("switch-drives-leave-question"),
            move |yes| {
                if yes {
                    event_sender.send(Event::Leave).unwrap();
                }
            },
        );

        self.textbox.push_interface(question);
        self.textbox.open();
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        let globals = Globals::from_resources(game_io);

        // clipboard isn't available on android
        #[cfg(not(target_os = "android"))]
        if self.input_tracker.pulsed(Input::Option2) {
            self.context_menu.open();
            globals.audio.play_sound(&globals.sfx.cursor_select);
            return;
        }

        let prev_state = self.state;
        let prev_slot_index = self.equipment_scroll_tracker.selected_index();
        let prev_list_index = self.list_scroll_tracker.selected_index();

        match self.state {
            State::ListSelection => {
                if self.input_tracker.pulsed(Input::Cancel) {
                    if self.filtered_packages {
                        self.event_sender.send(Event::ApplyFilter(None)).unwrap();
                        globals.audio.play_sound(&globals.sfx.cursor_cancel);
                    } else {
                        self.request_leave(game_io);
                    }
                } else if self.input_tracker.pulsed(Input::Left) {
                    self.state = State::EquipmentSelection;

                    let index = self.equipment_scroll_tracker.selected_index();
                    let slot = SwitchDriveSlot::from_usize(index);
                    self.equipment_map[slot].set_selected(true);

                    let globals = Globals::from_resources(game_io);

                    globals.audio.play_sound(&globals.sfx.cursor_select);
                } else if self.input_tracker.pulsed(Input::Confirm) {
                    // handle confirm
                    let success = self.add_drive_part(game_io, prev_list_index);
                    let globals = Globals::from_resources(game_io);

                    if success {
                        globals.audio.play_sound(&globals.sfx.cursor_select);
                    } else {
                        globals.audio.play_sound(&globals.sfx.cursor_error);
                    }
                } else {
                    // handle scrolling
                    self.list_scroll_tracker
                        .handle_vertical_input(&self.input_tracker);

                    if self.input_tracker.pulsed(Input::End) {
                        let last_index = self.list_scroll_tracker.total_items() - 1;
                        self.list_scroll_tracker.set_selected_index(last_index);
                    }
                }
            }
            State::EquipmentSelection => {
                if self.input_tracker.pulsed(Input::Confirm) {
                    let event_sender = self.event_sender.clone();

                    let slot = SwitchDriveSlot::from_usize(prev_slot_index);
                    let slot_ui = &self.equipment_map[slot];

                    let package_selected = slot_ui.package_id.is_some();

                    let question_string = if package_selected {
                        globals.translate_with_args(
                            "switch-drives-unequip-question",
                            vec![("name", slot_ui.package_name().into())],
                        )
                    } else {
                        let slot_name = globals.translate(slot.translation_key());

                        globals.translate_with_args(
                            "switch-drives-filter-slot-question",
                            vec![("slot", slot_name.into())],
                        )
                    };

                    let question = TextboxQuestion::new(game_io, question_string, move |yes| {
                        if !yes {
                            return;
                        }

                        let event = if package_selected {
                            Event::RemoveSwitchDrive
                        } else {
                            Event::ApplyFilter(Some(slot))
                        };

                        event_sender.send(event).unwrap();
                    });

                    self.textbox.push_interface(question);
                    self.textbox.open();
                } else if self.input_tracker.pulsed(Input::Right) {
                    let selected_index = self.equipment_scroll_tracker.selected_index();

                    if selected_index > 1 {
                        if !self.package_ids.is_empty() {
                            self.state = State::ListSelection;
                            globals.audio.play_sound(&globals.sfx.cursor_cancel);
                        }
                    } else {
                        self.equipment_scroll_tracker
                            .set_selected_index(selected_index + 2);
                    }
                } else if self.input_tracker.pulsed(Input::Left) {
                    let selected_index = self.equipment_scroll_tracker.selected_index();

                    if selected_index > 1 {
                        self.equipment_scroll_tracker
                            .set_selected_index(selected_index - 2);
                    }
                } else if self.input_tracker.pulsed(Input::Cancel) {
                    if self.package_ids.is_empty() {
                        if self.filtered_packages {
                            // remove filter if there's no packages to return to
                            self.event_sender.send(Event::ApplyFilter(None)).unwrap();
                            globals.audio.play_sound(&globals.sfx.cursor_cancel);
                        } else {
                            self.request_leave(game_io);
                        }
                    } else {
                        self.state = State::ListSelection;
                        globals.audio.play_sound(&globals.sfx.cursor_cancel);
                    }
                } else {
                    // handle scrolling
                    self.equipment_scroll_tracker
                        .handle_vertical_input(&self.input_tracker);

                    if self.input_tracker.pulsed(Input::End) {
                        let last_index = self.equipment_scroll_tracker.total_items() - 1;
                        self.equipment_scroll_tracker.set_selected_index(last_index);
                    }
                }
            }
        }

        if self.state == State::ListSelection && self.package_ids.is_empty() {
            // return to equipment selection
            self.state = State::EquipmentSelection;
        }

        // sfx + updating selected slot ui
        let selected_slot_index = self.equipment_scroll_tracker.selected_index();
        let list_index = self.list_scroll_tracker.selected_index();

        let state_changed = prev_state != self.state;
        let selection_changed = state_changed
            || prev_slot_index != selected_slot_index
            || prev_list_index != list_index;

        if selection_changed {
            self.update_selection_ui(game_io);

            if !state_changed {
                // changing state already plays sfx
                let globals = Globals::from_resources(game_io);
                globals.audio.play_sound(&globals.sfx.cursor_move);
            }
        }
    }

    fn update_selection_ui(&mut self, game_io: &GameIO) {
        let selected_slot_index = self.equipment_scroll_tracker.selected_index();
        let globals = Globals::from_resources(game_io);

        let selected_slot = match self.state {
            State::ListSelection => {
                let index = self.list_scroll_tracker.selected_index();
                self.package_ids
                    .get(index)
                    .and_then(|id| get_package(globals, id))
                    .and_then(|package| package.slot)
            }
            State::EquipmentSelection => Some(SwitchDriveSlot::from_usize(selected_slot_index)),
        };

        if let Some(slot) = selected_slot {
            let index = slot as usize;
            self.equipment_scroll_tracker.set_selected_index(index);
        }

        for (slot, slot_ui) in &mut self.equipment_map {
            slot_ui.set_selected(selected_slot == Some(slot));
        }
    }

    fn handle_context_selection(&mut self, game_io: &mut GameIO, selection: ContextOption) {
        const EXPORT_ORDER: &[SwitchDriveSlot] = &[
            SwitchDriveSlot::Head,
            SwitchDriveSlot::Arms,
            SwitchDriveSlot::Body,
            SwitchDriveSlot::Legs,
        ];

        match selection {
            ContextOption::Export => {
                let text = EXPORT_ORDER
                    .iter()
                    .map(|&slot| self.equipment_map[slot].package_id.as_ref())
                    .map(|id| {
                        id.map(|id| id.to_string())
                            .unwrap_or_else(|| String::from("---"))
                    })
                    .join("\n");

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
            ContextOption::Import => {
                let text = game_io.input_mut().request_clipboard_text();
                let globals = Globals::from_resources_mut(game_io);

                if !text.is_empty() {
                    // move drives back into the list
                    for ui in self.equipment_map.values_mut() {
                        let Some(package_id) = ui.package_id.take() else {
                            continue;
                        };
                        ui.set_package(None);

                        if let Err(index) = self.package_ids.binary_search(&package_id) {
                            self.package_ids.insert(index, package_id);
                        }
                    }

                    // import drives
                    let augment_packages = &globals.augment_packages;

                    for (slot, line) in EXPORT_ORDER.iter().zip(text.lines()) {
                        let package_id = PackageId::from(line);

                        let Some(package) =
                            augment_packages.package(PackageNamespace::Local, &package_id)
                        else {
                            continue;
                        };

                        if let Ok(index) = self.package_ids.binary_search(&package_id) {
                            self.package_ids.remove(index);
                            self.equipment_map[*slot].set_package(Some(package));
                        }
                    }

                    let list_size = self.package_ids.len();
                    self.list_scroll_tracker.set_total_items(list_size);
                } else {
                    let globals = Globals::from_resources(game_io);

                    let message = globals.translate("clipboard-read-failed");
                    let interface = TextboxMessage::new(message);

                    self.textbox.push_interface(interface);
                    self.textbox.open();
                }
            }
            ContextOption::Clear => {
                let globals = Globals::from_resources(game_io);
                let question = globals.translate("switch-drives-clear-question");

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
    }

    fn handle_events(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::Leave => {
                    let transition = crate::transitions::new_sub_scene_pop(game_io);
                    self.next_scene = NextScene::new_pop().with_transition(transition);

                    let globals = Globals::from_resources_mut(game_io);
                    let global_save = &mut globals.global_save;

                    // mark as updated for syncing
                    global_save.character_update_times.insert(
                        global_save.selected_character.clone(),
                        GlobalSave::current_time(),
                    );

                    // save parts
                    let part_list = global_save
                        .installed_drive_parts
                        .entry(global_save.selected_character.clone())
                        .or_default();

                    part_list.clear();

                    for (slot, ui) in &self.equipment_map {
                        if let Some(package_id) = ui.package_id.clone() {
                            part_list.push(InstalledSwitchDrive { package_id, slot });
                        }
                    }

                    // avoid storing package ids with empty lists for smaller file sizes
                    if part_list.is_empty() {
                        global_save
                            .installed_drive_parts
                            .remove(&global_save.selected_character);
                    }

                    global_save.save();
                }
                Event::AddSwitchDrive => {
                    // Not sure if I should move add_drive_part here or not.
                }
                Event::RemoveSwitchDrive => {
                    let selected_index = self.equipment_scroll_tracker.selected_index();
                    let slot = SwitchDriveSlot::from_usize(selected_index);
                    let slot_ui = &mut self.equipment_map[slot];

                    // move drive into package_ids list
                    if let Some(package_id) = slot_ui.package_id.take()
                        && let Err(index) = self.package_ids.binary_search(&package_id)
                    {
                        self.package_ids.insert(index, package_id);
                    }

                    slot_ui.set_package(None);
                }
                Event::ApplyFilter(filter) => {
                    let globals = Globals::from_resources(game_io);
                    self.package_ids = collect_drive_package_ids(game_io, globals, filter);

                    self.state = State::ListSelection;
                    self.list_scroll_tracker
                        .set_total_items(self.package_ids.len());
                    self.list_scroll_tracker.set_selected_index(0);

                    self.filtered_packages = filter.is_some();
                }
                Event::Clear => {
                    for slot_ui in self.equipment_map.values_mut() {
                        let Some(package_id) = slot_ui.package_id.take() else {
                            continue;
                        };

                        if let Err(index) = self.package_ids.binary_search(&package_id) {
                            self.package_ids.insert(index, package_id);
                        }

                        slot_ui.set_package(None);
                    }

                    let list_size = self.package_ids.len();
                    self.list_scroll_tracker.set_total_items(list_size);
                }
            }
        }
    }
}

impl Scene for SwitchDrivesScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.input_tracker.update(game_io);
        self.time += 1;

        self.textbox.update(game_io);

        if self.textbox.is_complete() {
            self.textbox.close();
        }

        if !game_io.is_in_transition() && !self.textbox.is_open() && !self.context_menu.is_open() {
            self.handle_input(game_io);
        }

        if let Some(selection) = self.context_menu.update(game_io, &self.input_tracker) {
            self.context_menu.close();
            self.handle_context_selection(game_io, selection);
        }

        self.handle_events(game_io);

        // update slot ui
        for (_, slot_ui) in &mut self.equipment_map {
            slot_ui.update(self.state, &mut self.animator);
        }

        // update cursor animation
        self.animator.set_state("OPTION_CURSOR");
        self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        self.animator.sync_time(self.cursor_time);
        self.cursor_time += 1;
        self.animator.apply(&mut self.cursor_sprite);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = Globals::from_resources(game_io);

        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw package list
        let mut recycled_sprite = Sprite::new(game_io, self.info_box_sprite.texture().clone());

        let selected_index = if self.state == State::ListSelection {
            Some(self.list_scroll_tracker.selected_index())
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

        if self.list_scroll_tracker.top_index() == 0 {
            offset += offset_jump;
        }

        text_style.bounds += offset;

        for i in self.list_scroll_tracker.view_range() {
            recycled_sprite.set_position(offset);

            if self.state == State::ListSelection && selected_index == Some(i) {
                self.animator.set_state("OPTION_BLINK");
                self.animator.set_loop_mode(AnimatorLoopMode::Loop);
                self.animator.sync_time(self.time);

                self.cursor_sprite.set_position(offset);
                sprite_queue.draw_sprite(&self.cursor_sprite);
            } else {
                self.animator.set_state("OPTION");
            }

            self.animator.apply(&mut recycled_sprite);
            sprite_queue.draw_sprite(&recycled_sprite);

            if let Some(package) = get_package(globals, &self.package_ids[i]) {
                text_style.draw(game_io, &mut sprite_queue, &package.name);
            }

            text_style.bounds += offset_jump;
            offset += offset_jump;
        }

        // draw information text
        sprite_queue.draw_sprite(&self.info_box_sprite);

        let selected_package_id = match self.state {
            State::ListSelection => self
                .package_ids
                .get(self.list_scroll_tracker.selected_index()),
            State::EquipmentSelection => {
                let index = self.equipment_scroll_tracker.selected_index();
                let slot = SwitchDriveSlot::from_usize(index);

                self.equipment_map[slot].package_id.as_ref()
            }
        };

        if let Some(package) = selected_package_id.and_then(|id| get_package(globals, id)) {
            self.info_text_style
                .draw(game_io, &mut sprite_queue, &package.description);
        }

        // main UI sprite
        sprite_queue.draw_sprite(&self.equipment_sprite);

        // body part preview
        for (_, slot_ui) in &self.equipment_map {
            slot_ui.draw_body_part(&mut sprite_queue);
        }

        // slots
        for (_, slot_ui) in &self.equipment_map {
            slot_ui.draw(game_io, &mut sprite_queue);
        }

        // draw frame
        self.frame.draw(&mut sprite_queue);
        self.scene_title.draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        // draw context menu
        self.context_menu.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn get_package<'a>(globals: &'a Globals, id: &PackageId) -> Option<&'a AugmentPackage> {
    let ns = PackageNamespace::Local;
    globals.augment_packages.package(ns, id)
}

fn collect_drive_package_ids(
    game_io: &GameIO,
    globals: &Globals,
    filter: Option<SwitchDriveSlot>,
) -> Vec<PackageId> {
    let restrictions = &globals.restrictions;
    let active_drive_parts = globals.global_save.active_drive_parts();

    let mut package_ids: Vec<_> = globals
        .augment_packages
        .packages(PackageNamespace::Local)
        .filter(|package| {
            let Some(slot) = package.slot else {
                return false;
            };

            filter.is_none() || filter == Some(slot)
        })
        .filter(|package| {
            restrictions.validate_package_tree(game_io, package.package_info.triplet())
        })
        .filter(|package| {
            !active_drive_parts
                .iter()
                .any(|drive| package.package_info.id == drive.package_id)
        })
        .map(|package| package.package_info.id.clone())
        .collect();

    package_ids.sort();

    package_ids
}
