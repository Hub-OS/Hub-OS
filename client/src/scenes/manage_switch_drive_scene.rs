use crate::bindable::SpriteColorMode;
use crate::packages::{AugmentPackage, PackageId, PackageNamespace};
use crate::render::ui::{
    FontName, SceneTitle, ScrollTracker, SubSceneFrame, Text, TextStyle, Textbox, TextboxQuestion,
    UiInputTracker,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, FrameTime, SpriteColorQueue};
use crate::resources::*;
use crate::saves::InstalledSwitchDrive;
use enum_map::{enum_map, Enum, EnumMap};
use framework::prelude::*;
use packets::structures::SwitchDriveSlot;
use std::borrow::Cow;
use std::collections::HashSet;

enum Event {
    Leave,
    AddSwitchDrive,
    RemoveSwitchDrive,
}

#[derive(Clone, Copy, PartialEq, Eq)]
enum State {
    ListSelection,
    EquipmentSelection,
}

struct SlotUi {
    name_text: Text,
    sprite: Sprite,
    base_state: &'static str,
    selected_state: &'static str,
    selected: bool,
    time: FrameTime,
    package_id: Option<PackageId>,
}

impl SlotUi {
    fn new_left(game_io: &GameIO, animator: &mut Animator, point_name: &str, offset: Vec2) -> Self {
        Self::new(
            game_io,
            animator,
            "LEFT_SLOT",
            "LEFT_SLOT_SELECTED",
            point_name,
            offset,
        )
    }

    fn new_right(
        game_io: &GameIO,
        animator: &mut Animator,
        point_name: &str,
        offset: Vec2,
    ) -> Self {
        Self::new(
            game_io,
            animator,
            "RIGHT_SLOT",
            "RIGHT_SLOT_SELECTED",
            point_name,
            offset,
        )
    }

    fn new(
        game_io: &GameIO,
        animator: &mut Animator,
        base_state: &'static str,
        selected_state: &'static str,
        point_name: &str,
        offset: Vec2,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut sprite = assets.new_sprite(game_io, ResourcePaths::SWITCH_DRIVE_UI);

        let point = animator.point(point_name).unwrap_or_default() + offset;

        // save state
        let old_state = animator.current_state().unwrap_or_default().to_string();

        // apply base state
        animator.set_state(base_state);
        animator.apply(&mut sprite);
        sprite.set_position(point);

        // resolve name position
        let name_bounds = Rect::from_corners(
            animator.point("NAME_START").unwrap_or_default(),
            animator.point("NAME_END").unwrap_or_default(),
        ) - animator.origin()
            + point;

        let mut name_text = Text::new_monospace(game_io, FontName::ThinSmall);
        name_text.style.bounds = name_bounds;

        // revert state
        animator.set_state(&old_state);

        Self {
            name_text,
            sprite,
            base_state,
            selected_state,
            selected: false,
            time: 0,
            package_id: None,
        }
    }

    fn package_name(&self) -> &str {
        &self.name_text.text
    }

    fn set_selected(&mut self, selected: bool) {
        self.selected = selected;
        self.time = 0;
    }

    fn set_package(&mut self, package: Option<&AugmentPackage>) {
        if let Some(package) = package {
            self.package_id = Some(package.package_info.id.clone());
            self.name_text.text = package.name.clone();
        } else {
            self.package_id = None;
            self.name_text.text.clear();
        }
    }

    fn update(&mut self, animator: &mut Animator) {
        let state = if self.selected {
            self.selected_state
        } else {
            self.base_state
        };

        animator.set_state(state);
        animator.sync_time(self.time);
        animator.apply(&mut self.sprite);
    }

    fn draw(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        sprite_queue.draw_sprite(&self.sprite);
        self.name_text.draw(game_io, sprite_queue);
    }
}

pub struct ManageSwitchDriveScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
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
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    next_scene: NextScene,
}

impl ManageSwitchDriveScene {
    pub fn new(game_io: &GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let global_save = &globals.global_save;

        let restrictions = &globals.restrictions;

        // load drive part packages
        let mut package_ids: Vec<_> = globals
            .augment_packages
            .packages_with_override(PackageNamespace::Local)
            .filter(|package| package.slot.is_some())
            .filter(|package| {
                restrictions.validate_package_tree(game_io, package.package_info.triplet())
            })
            .map(|package| package.package_info.id.clone())
            .collect();

        package_ids.sort();

        let mut animator = Animator::load_new(assets, ResourcePaths::SWITCH_DRIVE_UI_ANIMATION);

        // layout
        animator.set_state("DEFAULT");
        let equipment_position = animator.point("EQUIPMENT").unwrap_or_default();
        let info_box_position = animator.point("TEXTBOX").unwrap_or_default();

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

        if let Some(drive_parts_iter) = global_save.active_drive_parts() {
            for drive_part in drive_parts_iter {
                let package = get_package(globals, &drive_part.package_id);
                slot_map[drive_part.get_slot()].set_package(package);
            }
        };

        let mut info_box_sprite = equipment_sprite.clone();
        animator.set_state("TEXTBOX");
        animator.apply(&mut info_box_sprite);
        info_box_sprite.set_position(info_box_position);

        let info_bounds = Rect::from_corners(
            animator.point("TEXT_START").unwrap_or_default(),
            animator.point("TEXT_END").unwrap_or_default(),
        ) - animator.origin()
            + info_box_position;

        let info_text_style = TextStyle::new(game_io, FontName::Thin)
            .with_bounds(info_bounds)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
            .with_color(Color::WHITE);

        // cursor sprite
        let cursor_sprite = equipment_sprite.clone();

        // scroll tracker
        let scroll_tracker = ScrollTracker::new(game_io, 4)
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

        let (event_sender, event_receiver) = flume::unbounded();

        Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_character_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_everything(true),
            animator,
            cursor_sprite,
            cursor_time: 0,
            equipment_sprite,
            info_box_sprite,
            info_text_style,
            equipment_map: slot_map,
            input_tracker: UiInputTracker::new(),
            list_scroll_tracker: scroll_tracker,
            equipment_scroll_tracker,
            tracked_invalid: HashSet::new(),
            state: State::ListSelection,
            textbox: Textbox::new_navigation(game_io).with_position(RESOLUTION_F * 0.5),
            time: 0,
            package_ids,
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
        }
    }

    fn add_drive_part(&mut self, game_io: &mut GameIO, package_id: PackageId) -> bool {
        // TODO: validate the drive slot
        // let mut conflicts = Vec::new();

        // if !conflicts.is_empty() {
        //     return Some(conflicts);
        // }

        // actual placement
        // save drive parts
        let globals = game_io.resource_mut::<Globals>().unwrap();
        let global_save = &mut globals.global_save;

        let package = globals
            .augment_packages
            .package_or_override(PackageNamespace::Local, &package_id)
            .unwrap();

        let installed_drive = InstalledSwitchDrive {
            package_id,
            slot: package.slot.unwrap(),
        };

        global_save
            .installed_drive_parts
            .entry(global_save.selected_character.clone())
            .and_modify(|list| {
                // remove part installed in the same slot
                list.retain(|equipped_slot| equipped_slot.slot != package.slot.unwrap());
                list.push(installed_drive.clone());
            })
            .or_insert_with(|| vec![installed_drive.clone()]);

        global_save.save();

        true
    }

    fn handle_input(&mut self, game_io: &mut GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();

        let prev_state = self.state;
        let prev_slot_index = self.equipment_scroll_tracker.selected_index();
        let prev_list_index = self.list_scroll_tracker.selected_index();

        match self.state {
            State::ListSelection => {
                if self.input_tracker.is_active(Input::Cancel) {
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
                    self.state = State::EquipmentSelection;

                    let index = self.equipment_scroll_tracker.selected_index();
                    let slot = SwitchDriveSlot::from_usize(index);
                    self.equipment_map[slot].set_selected(true);

                    let globals = game_io.resource::<Globals>().unwrap();

                    globals.audio.play_sound(&globals.sfx.cursor_select);
                } else if self.input_tracker.is_active(Input::Confirm) {
                    // handle confirm
                    if let Some(package_id) = self.package_ids.get(prev_list_index).cloned() {
                        let success = self.add_drive_part(game_io, package_id.clone());

                        let globals = game_io.resource::<Globals>().unwrap();

                        if success {
                            self.list_scroll_tracker
                                .set_total_items(self.package_ids.len());

                            globals.audio.play_sound(&globals.sfx.cursor_select);

                            let package = get_package(globals, &package_id).unwrap();
                            let slot = package.slot.unwrap();

                            self.equipment_map[slot].set_package(Some(package));
                        } else {
                            globals.audio.play_sound(&globals.sfx.cursor_error);
                        }
                    }
                } else {
                    // handle scrolling
                    let prev_index = self.list_scroll_tracker.selected_index();

                    self.list_scroll_tracker
                        .handle_vertical_input(&self.input_tracker);

                    if self.input_tracker.is_active(Input::End) {
                        let last_index = self.list_scroll_tracker.total_items() - 1;
                        self.list_scroll_tracker.set_selected_index(last_index);
                    }

                    // sfx
                    let selected_index = self.list_scroll_tracker.selected_index();

                    if prev_index != selected_index {
                        let globals = game_io.resource::<Globals>().unwrap();
                        globals.audio.play_sound(&globals.sfx.cursor_move);
                    }
                }
            }
            State::EquipmentSelection => {
                if self.input_tracker.is_active(Input::Confirm) {
                    let event_sender = self.event_sender.clone();

                    let slot = SwitchDriveSlot::from_usize(prev_list_index);
                    let slot_ui = &self.equipment_map[slot];

                    let question_string = format!("Unequip {} ?", slot_ui.package_name());

                    let question = TextboxQuestion::new(question_string, move |yes| {
                        if yes {
                            event_sender.send(Event::RemoveSwitchDrive).unwrap();
                        }
                    });

                    self.textbox.push_interface(question);
                    self.textbox.open();
                } else if self.input_tracker.is_active(Input::Right) {
                    let selected_index = self.equipment_scroll_tracker.selected_index();

                    if selected_index > 1 {
                        self.state = State::ListSelection;
                    } else {
                        self.equipment_scroll_tracker
                            .set_selected_index(selected_index + 2);
                    }
                } else if self.input_tracker.is_active(Input::Left) {
                    let selected_index = self.equipment_scroll_tracker.selected_index();

                    if selected_index > 1 {
                        self.equipment_scroll_tracker
                            .set_selected_index(selected_index - 2);
                    }
                } else if self.input_tracker.is_active(Input::Cancel) {
                    globals.audio.play_sound(&globals.sfx.cursor_cancel);

                    self.state = State::ListSelection;
                } else {
                    // handle scrolling
                    self.equipment_scroll_tracker
                        .handle_vertical_input(&self.input_tracker);

                    if self.input_tracker.is_active(Input::End) {
                        let last_index = self.equipment_scroll_tracker.total_items() - 1;
                        self.equipment_scroll_tracker.set_selected_index(last_index);
                    }
                }
            }
        }

        // sfx + updating selected slot ui
        let selected_slot_index = self.equipment_scroll_tracker.selected_index();
        let list_index = self.list_scroll_tracker.selected_index();

        let selection_changed = prev_state != self.state
            || prev_slot_index != selected_slot_index
            || prev_list_index != list_index;

        if selection_changed {
            let globals = game_io.resource::<Globals>().unwrap();

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

            for (slot, slot_ui) in &mut self.equipment_map {
                slot_ui.set_selected(selected_slot == Some(slot));
            }

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }
    }

    fn handle_events(&mut self, game_io: &mut GameIO) {
        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::Leave => {
                    let transition = crate::transitions::new_sub_scene_pop(game_io);
                    self.next_scene = NextScene::new_pop().with_transition(transition);
                }
                Event::AddSwitchDrive => {
                    // Not sure if I should move add_drive_part here or not.
                }
                Event::RemoveSwitchDrive => {
                    let selected_index = self.equipment_scroll_tracker.selected_index();

                    let globals = game_io.resource_mut::<Globals>().unwrap();
                    let global_save = &mut globals.global_save;

                    let slot = SwitchDriveSlot::from_usize(selected_index);

                    global_save
                        .installed_drive_parts
                        .entry(global_save.selected_character.clone())
                        .and_modify(|list| {
                            if let Some(index) = list.iter().position(|drive| drive.slot == slot) {
                                // remove drive
                                list.remove(index);

                                // update ui
                                let slot_ui = &mut self.equipment_map[slot];
                                slot_ui.set_package(None);
                            }
                        });
                }
            }
        }
    }
}

impl Scene for ManageSwitchDriveScene {
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

        if !game_io.is_in_transition() && !self.textbox.is_open() {
            self.handle_input(game_io);
        }

        self.handle_events(game_io);

        // update slot ui
        for (_, slot_ui) in &mut self.equipment_map {
            slot_ui.update(&mut self.animator);
        }

        // update cursor animation
        self.animator.set_state("OPTION_CURSOR");
        self.animator.set_loop_mode(AnimatorLoopMode::Loop);
        self.animator.sync_time(self.cursor_time);
        self.cursor_time += 1;
        self.animator.apply(&mut self.cursor_sprite);
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let globals = game_io.resource::<Globals>().unwrap();

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
        let mut offset = self.animator.point("START").unwrap_or_default();
        let offset_jump = self.animator.point("STEP").unwrap_or_default();

        let text_bounds = Rect::from_corners(
            self.animator.point("TEXT_START").unwrap_or_default(),
            self.animator.point("TEXT_END").unwrap_or_default(),
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

        for (_, slot_ui) in &self.equipment_map {
            slot_ui.draw(game_io, &mut sprite_queue);
        }

        // draw frame
        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("DRIVES").draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn get_package<'a>(globals: &'a Globals, id: &PackageId) -> Option<&'a AugmentPackage> {
    let ns = PackageNamespace::Local;
    globals.augment_packages.package_or_override(ns, id)
}
