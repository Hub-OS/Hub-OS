use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::{Card, Folder};
use framework::prelude::*;

enum Event {
    Leave(bool),
}

#[repr(u8)]
#[derive(Clone, Copy, PartialEq)]
enum FolderSorting {
    Id,
    Alphabetical,
    Code,
    Damage,
    Element,
    Number,
    Class,
}

pub struct FolderEditScene {
    folder_index: usize,
    camera: Camera,
    background: Background,
    ui_input_tracker: UiInputTracker,
    scene_time: FrameTime,
    camera_x: f32,
    viewing_pack: bool,
    context_menu: ContextMenu<FolderSorting>,
    last_sort: Option<FolderSorting>,
    folder_dock: Dock,
    pack_dock: Dock,
    folder_size_sprite: Sprite,
    textbox: Textbox,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    next_scene: NextScene<Globals>,
}

impl FolderEditScene {
    pub fn new(game_io: &mut GameIO<Globals>, folder_index: usize) -> Box<Self> {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // background
        let background_animator = Animator::new();
        let background_sprite = assets.new_sprite(game_io, ResourcePaths::FOLDER_BG);

        // folder_dock
        let folder = &globals.global_save.folders[folder_index];
        let mut folder_dock = Dock::new(
            game_io,
            CardListItem::vec_from_folder(folder),
            ResourcePaths::FOLDER_DOCK,
            ResourcePaths::FOLDER_DOCK_ANIMATION,
        );
        folder_dock.update_card_count();

        // pack_dock
        let pack_dock = Dock::new(
            game_io,
            CardListItem::vec_from_packages(game_io),
            ResourcePaths::FOLDER_PACK_DOCK,
            ResourcePaths::FOLDER_PACK_DOCK_ANIMATION,
        );

        // static sprites
        let mut folder_size_sprite = assets.new_sprite(game_io, ResourcePaths::FOLDER_SIZE);
        folder_size_sprite.set_origin(Vec2::new(0.0, folder_size_sprite.size().y * 0.5));
        folder_size_sprite.set_position(Vec2::new(
            RESOLUTION_F.x - folder_size_sprite.size().x,
            5.0 + folder_size_sprite.size().y * 0.5,
        ));

        let (event_sender, event_receiver) = flume::unbounded();

        Box::new(Self {
            folder_index,
            camera,
            background: Background::new(background_animator, background_sprite),
            ui_input_tracker: UiInputTracker::new(),
            scene_time: 0,
            camera_x: 0.0,
            viewing_pack: false,
            context_menu: ContextMenu::new(game_io, "SORT", Vec2::ZERO).with_options(
                game_io,
                &[
                    ("ID", FolderSorting::Id),
                    ("ABCDE", FolderSorting::Alphabetical),
                    ("Code", FolderSorting::Code),
                    ("Attack", FolderSorting::Damage),
                    ("Element", FolderSorting::Element),
                    ("No.", FolderSorting::Number),
                    ("Class", FolderSorting::Class),
                ],
            ),
            last_sort: None,
            folder_dock,
            pack_dock,
            folder_size_sprite,
            textbox: Textbox::new_navigation(game_io),
            event_sender,
            event_receiver,
            next_scene: NextScene::None,
        })
    }

    fn leave(&mut self, game_io: &mut GameIO<Globals>, equip_folder: bool) {
        use crate::transitions::{ColorFadeTransition, DEFAULT_FADE_DURATION};

        let transition = ColorFadeTransition::new(game_io, Color::BLACK, DEFAULT_FADE_DURATION);

        self.next_scene = NextScene::new_pop().with_transition(transition);

        // save
        let global_save = &mut game_io.globals_mut().global_save;

        if equip_folder {
            global_save.selected_folder = self.folder_index;
        }

        global_save.folders[self.folder_index].cards = self.clone_cards();

        global_save.save();
    }

    fn clone_cards(&self) -> Vec<Card> {
        self.folder_dock
            .card_items
            .iter()
            .flatten()
            .map(|item| item.card.clone())
            .collect()
    }
}

impl Scene<Globals> for FolderEditScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        self.scene_time += 1;

        // update camera
        if self.viewing_pack {
            self.camera_x += 1.0 / (60.0 * 0.33);
        } else {
            self.camera_x -= 1.0 / (60.0 * 0.33);
        }

        self.camera_x = self.camera_x.clamp(0.0, 1.0);
        self.camera.snap(Vec2::new(
            (self.camera_x + 0.5) * RESOLUTION_F.x,
            RESOLUTION_F.y * 0.5,
        ));
        self.camera.update(game_io);

        let textbox_position = Vec2::new(self.camera.position().x, self.textbox.position().y);
        self.textbox.set_position(textbox_position);
        self.textbox.update(game_io);

        // input
        if game_io.is_in_transition() {
            return;
        }

        handle_events(self, game_io);

        if self.textbox.is_open() {
            return;
        }

        handle_input(self, game_io);
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw title
        SceneTitle::new("FOLDER EDIT").draw(game_io, &mut sprite_queue);

        // draw folder size
        sprite_queue.draw_sprite(&self.folder_size_sprite);

        let mut count_text = Text::new(game_io, FontStyle::Wide);
        (count_text.style.bounds).set_position(self.folder_size_sprite.position());
        count_text.style.bounds.x += 10.0;
        count_text.style.bounds.y -= 3.0;

        let card_count = self.folder_dock.card_count;

        count_text.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        count_text.style.color = if card_count == MAX_CARDS {
            Color::from((173, 255, 189))
        } else {
            Color::from((255, 181, 74))
        };

        count_text.text = format!("{:>2}/{}", card_count, MAX_CARDS);
        count_text.draw(game_io, &mut sprite_queue);

        // draw docks
        self.folder_dock.draw(game_io, &mut sprite_queue);
        self.pack_dock.draw(game_io, &mut sprite_queue);

        if !self.context_menu.is_open() {
            self.folder_dock.draw_cursor(&mut sprite_queue);
            self.pack_dock.draw_cursor(&mut sprite_queue);
        }

        // draw context menu
        if self.context_menu.is_open() {
            let active_dock = match self.viewing_pack {
                true => &mut self.pack_dock,
                false => &mut self.folder_dock,
            };

            // context menu
            self.context_menu
                .set_position(active_dock.context_menu_position);

            self.context_menu.draw(game_io, &mut sprite_queue);
        }

        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

fn handle_events(scene: &mut FolderEditScene, game_io: &mut GameIO<Globals>) {
    if let Ok(Event::Leave(equip)) = scene.event_receiver.try_recv() {
        scene.leave(game_io, equip);
        scene.textbox.close();
    }
}

fn handle_input(scene: &mut FolderEditScene, game_io: &mut GameIO<Globals>) {
    scene.ui_input_tracker.update(game_io);

    if scene.context_menu.is_open() {
        handle_context_menu_input(scene, game_io);
        return;
    }

    // dock scrolling
    let active_dock = match scene.viewing_pack {
        true => &mut scene.pack_dock,
        false => &mut scene.folder_dock,
    };

    let scroll_tracker = &mut active_dock.scroll_tracker;
    let original_index = scroll_tracker.selected_index();

    scroll_tracker.handle_vertical_input(&scene.ui_input_tracker);

    if original_index != scroll_tracker.selected_index() {
        active_dock.update_preview();

        let globals = game_io.globals();
        globals.audio.play_sound(&globals.cursor_move_sfx);
    }

    // selecting dock
    let input_util = InputUtil::new(game_io);

    let was_viewing_pack = scene.viewing_pack;

    if !scene.viewing_pack && input_util.was_just_pressed(Input::Right) {
        scene.last_sort = None;
        scene.viewing_pack = true;
    }

    if scene.viewing_pack && input_util.was_just_pressed(Input::Left) {
        scene.last_sort = None;
        scene.viewing_pack = false;
    }

    if was_viewing_pack != scene.viewing_pack {
        let globals = game_io.globals();
        globals.audio.play_sound(&globals.page_turn_sfx);
    }

    if input_util.was_just_pressed(Input::Confirm) {
        let globals = game_io.globals();
        globals.audio.play_sound(&globals.cursor_select_sfx);

        if let Some(index) = active_dock.scroll_tracker.forget_index() {
            dock_internal_swap(scene, game_io, index);
        } else {
            active_dock.scroll_tracker.remember_index();
        }
    }

    // cancelling
    if input_util.was_just_pressed(Input::Cancel) {
        let globals = game_io.globals();
        globals.audio.play_sound(&globals.cursor_cancel_sfx);

        let cancel_handled = scene.folder_dock.scroll_tracker.forget_index().is_some()
            || scene.pack_dock.scroll_tracker.forget_index().is_some();

        // closing
        if !cancel_handled {
            let folder = &globals.global_save.folders[scene.folder_index];
            let is_equipped = globals.global_save.selected_folder == scene.folder_index;

            if is_equipped || folder.cards == scene.clone_cards() {
                // didn't modify the folder, leave without changing the equipped folder
                scene.leave(game_io, false);
            } else {
                let event_sender = scene.event_sender.clone();
                let callback = move |response| {
                    event_sender.send(Event::Leave(response)).unwrap();
                };

                let textbox_interface =
                    TextboxQuestion::new(format!("Equip {}?", folder.name), callback);

                scene.textbox.push_interface(textbox_interface);
                scene.textbox.open();
            }

            return;
        }
    }

    // context menu
    if input_util.was_just_pressed(Input::Option) {
        let globals = game_io.globals();
        globals.audio.play_sound(&globals.cursor_select_sfx);

        scene.context_menu.open();
    }
}

fn handle_context_menu_input(scene: &mut FolderEditScene, game_io: &mut GameIO<Globals>) {
    let selected_option = match scene.context_menu.update(game_io, &scene.ui_input_tracker) {
        Some(option) => option,
        None => return,
    };

    let card_manager = &game_io.globals().card_packages;
    let card_items = match scene.viewing_pack {
        true => &mut scene.pack_dock.card_items,
        false => &mut scene.folder_dock.card_items,
    };

    match selected_option {
        FolderSorting::Id => sort_card_items(card_items, |item: &CardListItem| {
            item.card.package_id.clone()
        }),
        FolderSorting::Alphabetical => sort_card_items(card_items, |item: &CardListItem| {
            let package = card_manager
                .package_or_fallback(PackageNamespace::Server, &item.card.package_id)
                .unwrap();

            package.card_properties.short_name.clone()
        }),
        FolderSorting::Code => {
            sort_card_items(card_items, |item: &CardListItem| item.card.code.clone())
        }
        FolderSorting::Damage => sort_card_items(card_items, |item: &CardListItem| {
            let package = card_manager
                .package_or_fallback(PackageNamespace::Server, &item.card.package_id)
                .unwrap();

            -package.card_properties.damage
        }),
        FolderSorting::Element => sort_card_items(card_items, |item: &CardListItem| {
            let package = card_manager
                .package_or_fallback(PackageNamespace::Server, &item.card.package_id)
                .unwrap();

            package.card_properties.element as u8
        }),
        FolderSorting::Number => {
            sort_card_items(card_items, |item: &CardListItem| -(item.count as isize))
        }
        FolderSorting::Class => sort_card_items(card_items, |item: &CardListItem| {
            let package = card_manager
                .package_or_fallback(PackageNamespace::Server, &item.card.package_id)
                .unwrap();

            package.card_properties.card_class as u8
        }),
    }

    if scene.last_sort.take() == Some(selected_option) {
        card_items.reverse();
    } else {
        scene.last_sort = Some(selected_option);
    }

    // blanks should always be at the bottom
    card_items.sort_by_key(|item| !item.is_some());
}

fn sort_card_items<F, K>(card_items: &mut [Option<CardListItem>], key_function: F)
where
    F: FnMut(&CardListItem) -> K + Copy,
    K: std::cmp::Ord,
{
    card_items.sort_by_cached_key(move |item| item.as_ref().map(key_function));
}

fn dock_internal_swap(scene: &mut FolderEditScene, game_io: &GameIO<Globals>, index: usize) {
    if scene.viewing_pack {
        let selected_index = scene.pack_dock.scroll_tracker.selected_index();

        if index == selected_index {
            transfer_to_folder(scene, game_io, index);
        } else {
            scene.pack_dock.card_items.swap(selected_index, index);
        }
    } else {
        let selected_index = scene.folder_dock.scroll_tracker.selected_index();

        if index == selected_index {
            transfer_to_pack(scene, index);
        } else {
            scene.folder_dock.card_items.swap(selected_index, index);
        }
    }
}

fn transfer_to_folder(
    scene: &mut FolderEditScene,
    game_io: &GameIO<Globals>,
    from_index: usize,
) -> Option<()> {
    let card_item = scene.pack_dock.card_items.get_mut(from_index)?.as_mut()?;

    let folder_card_items = &mut scene.folder_dock.card_items;

    // maintain limit requirement
    let existing_count = folder_card_items
        .iter()
        .filter(|item| {
            item.as_ref()
                .map(|item| item.card.package_id == card_item.card.package_id)
                .unwrap_or_default()
        })
        .count();

    let card_manager = &game_io.globals().card_packages;
    let package =
        card_manager.package_or_fallback(PackageNamespace::Server, &card_item.card.package_id)?;

    if existing_count >= package.card_properties.limit {
        // folder already has too many
        return None;
    }

    // search for an empty slot to insert the card into
    let empty_slot = folder_card_items.iter_mut().find(|item| item.is_none())?;

    *empty_slot = Some(CardListItem {
        card: card_item.card.clone(),
        count: 1,
        show_count: false,
    });

    card_item.count -= 1;

    if card_item.count == 0 {
        scene.pack_dock.card_items.remove(from_index);

        let pack_size = scene.pack_dock.card_items.len();
        scene.pack_dock.scroll_tracker.set_total_items(pack_size);
    }

    scene.folder_dock.update_card_count();

    None
}

fn transfer_to_pack(scene: &mut FolderEditScene, from_index: usize) -> Option<()> {
    let card = scene
        .folder_dock
        .card_items
        .get_mut(from_index)?
        .take()?
        .card;

    let pack_items = &mut scene.pack_dock.card_items;

    let pack_item = pack_items
        .iter_mut()
        .find(|item| item.as_ref().unwrap().card == card);

    match pack_item {
        Some(Some(card_item)) => {
            card_item.count += 1;
        }
        _ => {
            pack_items.push(Some(CardListItem {
                card,
                count: 1,
                show_count: true,
            }));
        }
    }

    None
}

struct Dock {
    card_items: Vec<Option<CardListItem>>,
    card_count: usize,
    scroll_tracker: ScrollTracker,
    dock_sprite: Sprite,
    dock_animator: Animator,
    card_preview: FullCard,
    list_position: Vec2,
    context_menu_position: Vec2,
    page_arrows: Option<PageArrows>,
}

impl Dock {
    fn new(
        game_io: &GameIO<Globals>,
        card_items: Vec<Option<CardListItem>>,
        texture_path: &str,
        animation_path: &str,
    ) -> Self {
        let globals = game_io.globals();
        let assets = &globals.assets;

        // dock
        let mut dock_sprite = assets.new_sprite(game_io, texture_path);
        let mut dock_animator = Animator::load_new(assets, animation_path);
        dock_animator.set_state("default");
        dock_animator.set_loop_mode(AnimatorLoopMode::Loop);
        dock_animator.apply(&mut dock_sprite);

        let dock_offset = -dock_sprite.origin();

        let list_point = dock_animator.point("list").unwrap_or_default();
        let list_position = dock_offset + list_point;

        let context_menu_point = dock_animator.point("context_menu").unwrap_or_default();
        let context_menu_position = dock_offset + context_menu_point;

        // card sprite
        let card_offset = dock_animator.point("card").unwrap_or_default();
        let card_preview = FullCard::new(game_io, dock_offset + card_offset);

        // scroll tracker
        let mut scroll_tracker = ScrollTracker::new(game_io, 7);
        scroll_tracker.set_total_items(card_items.len());

        let scroll_start_point = dock_animator.point("scroll_start").unwrap_or_default();
        let scroll_end_point = dock_animator.point("scroll_end").unwrap_or_default();

        let scroll_start = dock_offset + scroll_start_point;
        let scroll_end = dock_offset + scroll_end_point;

        scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        let cursor_start = list_position + Vec2::new(-7.0, 3.0);
        scroll_tracker.define_cursor(cursor_start, 16.0);

        // page_arrows
        let page_arrows = dock_animator
            .point("page_arrows")
            .map(|point| PageArrows::new(game_io, dock_offset + point));

        let mut dock = Self {
            card_items,
            card_count: 0,
            scroll_tracker,
            dock_sprite,
            dock_animator,
            card_preview,
            list_position,
            context_menu_position,
            page_arrows,
        };

        dock.update_preview();

        dock
    }

    fn update_card_count(&mut self) {
        use std::collections::HashMap;

        let mut counts = HashMap::new();

        for card_item in &self.card_items {
            let card = match card_item {
                Some(card_item) => &card_item.card,
                None => continue,
            };

            if let Some(count) = counts.get_mut(&card.package_id) {
                *count += 1;
            } else {
                counts.insert(card.package_id.clone(), 1);
            }
        }

        for card_item in self.card_items.iter_mut().flatten() {
            card_item.count = *counts.get(&card_item.card.package_id).unwrap();
        }

        self.card_count = self.card_items.iter().filter(|item| item.is_some()).count();
    }

    fn update_preview(&mut self) -> Option<()> {
        let selected_index = self.scroll_tracker.selected_index();

        let card_list_item = self.card_items.get(selected_index)?;
        let card = card_list_item.as_ref().map(|item| &item.card);

        self.card_preview.set_card(card.cloned());

        None
    }

    fn draw(&mut self, game_io: &GameIO<Globals>, sprite_queue: &mut SpriteColorQueue) {
        self.dock_animator.update();
        self.dock_animator.apply(&mut self.dock_sprite);
        sprite_queue.draw_sprite(&self.dock_sprite);

        self.card_preview.draw(game_io, sprite_queue);

        for i in self.scroll_tracker.view_range() {
            let card_item = match &self.card_items[i] {
                Some(card_item) => card_item,
                None => continue,
            };

            let relative_index = i - self.scroll_tracker.top_index();

            let mut position = self.list_position;
            position.y += relative_index as f32 * self.scroll_tracker.cursor_multiplier();

            card_item.draw_list_item(game_io, sprite_queue, position);
        }

        self.scroll_tracker.draw_scrollbar(sprite_queue);

        if let Some(page_arrows) = self.page_arrows.as_mut() {
            page_arrows.draw(sprite_queue);
        }
    }

    fn draw_cursor(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.scroll_tracker.draw_cursor(sprite_queue);
    }
}

struct CardListItem {
    card: Card,
    count: usize,
    show_count: bool,
}

impl CardListItem {
    pub fn vec_from_folder(folder: &Folder) -> Vec<Option<CardListItem>> {
        let mut card_items: Vec<_> = folder
            .cards
            .iter()
            .map(|card| {
                Some(CardListItem {
                    card: card.clone(),
                    count: 0,
                    show_count: false,
                })
            })
            .collect();

        // force 30 items
        card_items.resize_with(30, || None);

        card_items
    }

    pub fn vec_from_packages(game_io: &GameIO<Globals>) -> Vec<Option<CardListItem>> {
        let package_manager = &game_io.globals().card_packages;

        // todo: allow use of server packages as well?
        package_manager
            .local_packages()
            .filter_map(|id| package_manager.package_or_fallback(PackageNamespace::Server, id))
            .flat_map(|package| {
                let package_info = package.package_info();

                package.default_codes.iter().map(|code| {
                    Some(CardListItem {
                        card: Card {
                            package_id: package_info.id.clone(),
                            code: code.clone(),
                        },
                        count: package.card_properties.limit,
                        show_count: true,
                    })
                })
            })
            .collect()
    }

    pub fn draw_list_item(
        &self,
        game_io: &GameIO<Globals>,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
    ) {
        self.card.draw_list_item(game_io, sprite_queue, position);

        if !self.show_count {
            return;
        }

        const COUNT_OFFSET: Vec2 = Vec2::new(120.0, 3.0);

        let mut label = Text::new(game_io, FontStyle::Thick);
        label.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        label.style.bounds.set_position(COUNT_OFFSET + position);
        label.text = format!("{:>2}", self.count.min(99));
        label.draw(game_io, sprite_queue);
    }
}
