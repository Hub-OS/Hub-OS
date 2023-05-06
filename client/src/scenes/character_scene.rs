use super::{CharacterSelectScene, CustomizeScene};
use crate::bindable::SpriteColorMode;
use crate::packages::{PackageNamespace, PlayerPackage};
use crate::render::ui::{
    ElementSprite, FontStyle, PageTracker, PlayerHealthUi, SceneTitle, ScrollableList,
    SubSceneFrame, Text, Textbox, TextboxCharacterNavigation, UiInputTracker, UiNode,
};
use crate::render::{Animator, AnimatorLoopMode, Background, Camera, SpriteColorQueue};
use crate::resources::*;
use crate::saves::BlockGrid;
use framework::prelude::*;

enum Event {
    BlockCustomization,
    CharacterSelect,
}

pub struct CharacterScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    textbox: Textbox,
    event_sender: flume::Sender<Event>,
    event_receiver: flume::Receiver<Event>,
    page_tracker: PageTracker,
    pages: Vec<StatusPage>,
    input_tracker: UiInputTracker,
    next_scene: NextScene,
}

impl CharacterScene {
    pub fn new(game_io: &GameIO) -> Box<Self> {
        let (event_sender, event_receiver) = flume::unbounded();

        Box::new(Self {
            camera: Camera::new_ui(game_io),
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            textbox: Textbox::new_navigation(game_io).begin_open(),
            event_sender,
            event_receiver,
            page_tracker: PageTracker::new(game_io, 2),
            pages: Vec::new(),
            input_tracker: UiInputTracker::new(),
            next_scene: NextScene::None,
        })
    }

    fn load_pages(&mut self, game_io: &GameIO) {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut page_animator =
            Animator::load_new(assets, ResourcePaths::CHARACTER_PAGES_ANIMATION);
        let page_sprite = assets.new_sprite(game_io, ResourcePaths::CHARACTER_PAGES);

        self.pages.clear();

        let data = StatusData::new(game_io);

        for (i, state) in ["PAGE_1", "PAGE_2"].into_iter().enumerate() {
            page_animator.set_state(state);
            self.pages.push(StatusPage::new(
                game_io,
                &page_animator,
                page_sprite.clone(),
                &data,
            ));

            if let Some(point) = page_animator.point("PAGE_ARROWS") {
                self.page_tracker.set_page_arrow_offset(i - 1, point)
            }
        }
    }

    fn reload_textbox(&mut self, game_io: &GameIO) {
        let event_sender = self.event_sender.clone();
        let interface = TextboxCharacterNavigation::new(move |i| {
            let event = if i == 0 {
                Event::BlockCustomization
            } else {
                Event::CharacterSelect
            };

            event_sender.send(event).unwrap();
        });

        self.textbox = Textbox::new_navigation(game_io)
            .begin_open()
            .with_interface(interface);
        self.textbox.skip_animation(game_io);
    }

    fn handle_input(&mut self, game_io: &GameIO) {
        self.input_tracker.update(game_io);
        self.page_tracker.handle_input(game_io);

        if !self.page_tracker.animating() {
            let page = &mut self.pages[self.page_tracker.active_page()];
            page.handle_input(game_io, &self.input_tracker);
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Cancel) {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);

            let transition = crate::transitions::new_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);
        }
    }
}

impl Scene for CharacterScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn enter(&mut self, game_io: &mut GameIO) {
        self.load_pages(game_io);
        self.reload_textbox(game_io);
    }

    fn update(&mut self, game_io: &mut GameIO) {
        self.background.update();
        self.textbox.update(game_io);
        self.page_tracker.update();

        for page in &mut self.pages {
            page.update();
        }

        while let Ok(event) = self.event_receiver.try_recv() {
            match event {
                Event::BlockCustomization => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(CustomizeScene::new(game_io))
                        .with_transition(transition)
                }
                Event::CharacterSelect => {
                    let transition = crate::transitions::new_sub_scene(game_io);
                    self.next_scene = NextScene::new_push(CharacterSelectScene::new(game_io))
                        .with_transition(transition)
                }
            }
        }

        if !game_io.is_in_transition() {
            self.handle_input(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw page
        for (index, offset) in self.page_tracker.visible_pages() {
            self.pages[index].draw(game_io, &mut sprite_queue, offset);
        }

        self.page_tracker.draw_page_arrows(&mut sprite_queue);

        // draw title
        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("STATUS").draw(game_io, &mut sprite_queue);

        // draw textbox
        self.textbox.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

struct StatusData<'a> {
    player_package: &'a PlayerPackage,
    block_grid: BlockGrid,
    health: i32,
    charge_level: i8,
    attack_level: i8,
    rapid_level: i8,
    mega_limit: isize,
    giga_limit: isize,
}

impl<'a> StatusData<'a> {
    fn new(game_io: &'a GameIO) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let global_save = &globals.global_save;
        let player_package = global_save.player_package(game_io).unwrap();

        let blocks = global_save.active_blocks().cloned().unwrap_or_default();
        let block_grid = BlockGrid::new(PackageNamespace::Local).with_blocks(game_io, blocks);

        let mut health = player_package.health;
        let mut attack_level = 1;
        let mut rapid_level = 1;
        let mut charge_level = 1;
        let mut mega_limit = MAX_MEGA as isize;
        let mut giga_limit = MAX_GIGA as isize;

        for (package, level) in block_grid.augments(game_io) {
            health += package.health_boost * level as i32;
            attack_level += package.attack_boost * level as i8;
            rapid_level += package.rapid_boost * level as i8;
            charge_level += package.charge_boost * level as i8;
            mega_limit += package.mega_boost * level as isize;
            giga_limit += package.giga_boost * level as isize;
        }

        Self {
            player_package,
            block_grid,
            health,
            attack_level: attack_level.clamp(1, 5),
            rapid_level: rapid_level.clamp(1, 5),
            charge_level: charge_level.clamp(1, 5),
            mega_limit: mega_limit.max(0),
            giga_limit: giga_limit.max(0),
        }
    }
}

struct StatusPage {
    text: Vec<Text>,
    sprites: Vec<Sprite>,
    animators: Vec<(Animator, usize)>,
    lists: Vec<ScrollableList>,
    player_health_ui: Vec<PlayerHealthUi>,
}

impl StatusPage {
    fn new(
        game_io: &GameIO,
        layout_animator: &Animator,
        mut page_sprite: Sprite,
        data: &StatusData,
    ) -> Self {
        let mut page = Self {
            text: Vec::new(),
            sprites: Vec::new(),
            animators: Vec::new(),
            lists: Vec::new(),
            player_health_ui: Vec::new(),
        };

        layout_animator.apply(&mut page_sprite);
        page.sprites.push(page_sprite);

        if let Some(point) = layout_animator.point("ELEMENT") {
            let mut sprite = ElementSprite::new(game_io, data.player_package.element);
            sprite.set_position(point);
            page.sprites.push(sprite);
        }

        if let Some(point) = layout_animator.point("HEALTH") {
            let mut player_health_ui = PlayerHealthUi::new(game_io);
            player_health_ui.snap_health(data.health);
            player_health_ui.set_position(point);

            page.player_health_ui.push(player_health_ui);
        }

        if let Some(point) = layout_animator.point("PLAYER") {
            let (texture, mut animator) = data.player_package.resolve_battle_sprite(game_io);

            let mut sprite = Sprite::new(game_io, texture);
            sprite.set_position(point);

            animator.set_state("PLAYER_IDLE");
            animator.set_loop_mode(AnimatorLoopMode::Loop);
            animator.apply(&mut sprite);

            let sprite_index = page.sprites.len();
            page.sprites.push(sprite);

            page.animators.push((animator, sprite_index));
        }

        let stat_text_associations = [
            ("ATTACK_TEXT", "Attack LV ", data.attack_level),
            ("RAPID_TEXT", "Rapid  LV ", data.rapid_level),
            ("CHARGE_TEXT", "Charge LV ", data.charge_level),
        ];

        for (label, prefix, value) in stat_text_associations {
            if let Some(point) = layout_animator.point(label) {
                let mut text = Text::new(game_io, FontStyle::Thick);
                text.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
                text.style.letter_spacing = 2.0;
                text.style.bounds.set_position(point);

                text.text = format!("{prefix}{value}");

                page.text.push(text);
            }
        }

        if let Some(point) = layout_animator.point("POWER_CHARGE_TEXT") {
            let mut text = Text::new(game_io, FontStyle::Thick);
            text.style.shadow_color = TEXT_DARK_SHADOW_COLOR;
            text.style.letter_spacing = 2.0;
            text.style.bounds.set_position(point);

            text.text = String::from("PwrCharge S");

            page.text.push(text);
        }

        if let Some(start_point) = layout_animator.point("DECK_START") {
            let end_point = layout_animator.point("DECK_END").unwrap_or_default();
            let bounds = Rect::from_corners(start_point, end_point);

            let list = ScrollableList::new(game_io, bounds, 15.0)
                .with_label_str("FOLDER")
                .with_focus(false)
                .with_children(vec![
                    Box::new(
                        Text::new_monospace(game_io, FontStyle::Thin)
                            .with_string(format!("MegaLimit {}", data.mega_limit))
                            .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
                    ),
                    Box::new(
                        Text::new_monospace(game_io, FontStyle::Thin)
                            .with_string(format!("GigaLimit {}", data.giga_limit))
                            .with_shadow_color(TEXT_DARK_SHADOW_COLOR),
                    ),
                ]);

            page.lists.push(list);
        }

        if let Some(start_point) = layout_animator.point("BLOCKS_START") {
            let end_point = layout_animator.point("BLOCKS_END").unwrap_or_default();
            let bounds = Rect::from_corners(start_point, end_point);

            let list = ScrollableList::new(game_io, bounds, 15.0)
                .with_label_str("BLOCKS")
                .with_focus(false)
                .with_children(
                    data.block_grid
                        .installed_packages(game_io)
                        .map(|package| -> Box<dyn UiNode> {
                            Box::new(
                                Text::new(game_io, FontStyle::Thin)
                                    .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
                                    .with_str(&package.name),
                            )
                        })
                        .collect(),
                );

            page.lists.push(list);
        }

        page
    }

    fn handle_input(&mut self, game_io: &GameIO, input_tracker: &UiInputTracker) {
        let mut scrolled = false;

        for list in &mut self.lists {
            let prev_index = list.selected_index();

            if input_tracker.is_active(Input::ShoulderL) {
                list.page_up();
            }

            if input_tracker.is_active(Input::ShoulderR) {
                list.page_down();
            }

            scrolled |= list.selected_index() != prev_index;
        }

        if scrolled {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }
    }

    fn update(&mut self) {
        for (animator, i) in &mut self.animators {
            animator.update();
            animator.apply(&mut self.sprites[*i]);
        }
    }

    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, offset_x: f32) {
        let offset = Vec2::new(offset_x, 0.0);

        for player_health_ui in &mut self.player_health_ui {
            let position = player_health_ui.position();
            player_health_ui.set_position(position + offset);

            player_health_ui.draw(game_io, sprite_queue);

            player_health_ui.set_position(position);
        }

        for sprite in &mut self.sprites {
            let position = sprite.position();
            sprite.set_position(position + offset);

            sprite_queue.draw_sprite(sprite);

            sprite.set_position(position);
        }

        for text in &mut self.text {
            let bounds = text.style.bounds;
            text.style.bounds = bounds + offset;

            text.draw(game_io, sprite_queue);

            text.style.bounds = bounds;
        }

        for list in &mut self.lists {
            let bounds = list.bounds();
            list.set_bounds(bounds + offset);

            list.draw(game_io, sprite_queue);

            list.set_bounds(bounds);
        }
    }
}
