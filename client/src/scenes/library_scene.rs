use crate::bindable::CardClass;
use crate::bindable::SpriteColorMode;
use crate::packages::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use framework::prelude::*;

pub struct LibraryScene {
    camera: Camera,
    background: Background,
    frame: SubSceneFrame,
    card_preview: FullCard,
    ui_input_tracker: UiInputTracker,
    page_tracker: PageTracker,
    dock_scissor: Rect,
    docks: Vec<Dock>,
    next_scene: NextScene,
}

impl LibraryScene {
    pub fn new(game_io: &GameIO) -> Box<Self> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // background
        let mut ui_animator = Animator::load_new(assets, ResourcePaths::LIBRARY_UI_ANIMATION);
        ui_animator.set_state("DEFAULT");

        // docks
        let dock_offset = ui_animator.point_or_zero("DOCK");
        let mut package_ids: Vec<_> = globals
            .card_packages
            .package_ids(PackageNamespace::Local)
            .cloned()
            .collect();

        package_ids.sort();

        let docks = vec![
            Dock::new(game_io, dock_offset, &package_ids, CardClass::Standard),
            Dock::new(game_io, dock_offset, &package_ids, CardClass::Mega),
            Dock::new(game_io, dock_offset, &package_ids, CardClass::Giga),
            Dock::new(game_io, dock_offset, &package_ids, CardClass::Dark),
        ];

        // page tracker
        let dock_sprite = &docks[0].dock_sprite;
        let dock_animator = &docks[0].dock_animator;

        let dock_scissor = dock_sprite.bounds() / RESOLUTION_F;
        let page_width = dock_sprite.size().x;
        let page_arrow_point = dock_animator.point_or_zero("PAGE_ARROWS");

        let mut page_tracker = PageTracker::new(game_io, docks.len())
            .with_page_width(page_width)
            .with_page_start(dock_sprite.bounds().top_left());

        for i in 0..docks.len() - 1 {
            page_tracker = page_tracker.with_page_arrow_offset(i, page_arrow_point);
        }

        // card
        let card_position = ui_animator.point_or_zero("CARD");

        let mut scene = Box::new(Self {
            camera,
            background: Background::new_sub_scene(game_io),
            frame: SubSceneFrame::new(game_io).with_top_bar(true),
            ui_input_tracker: UiInputTracker::new(),
            card_preview: FullCard::new(game_io, card_position),
            page_tracker,
            dock_scissor,
            docks,
            next_scene: NextScene::None,
        });

        scene.update_preview();

        scene
    }

    fn handle_input(&mut self, game_io: &GameIO) {
        self.ui_input_tracker.update(game_io);

        // dock scrolling
        let page = self.page_tracker.active_page();

        let active_dock = &mut self.docks[page];
        let scroll_tracker = &mut active_dock.scroll_tracker;

        let original_index = scroll_tracker.selected_index();

        scroll_tracker.handle_vertical_input(&self.ui_input_tracker);

        if original_index != scroll_tracker.selected_index() {
            self.update_preview();

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_move);
        }

        // flip
        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Special) {
            self.card_preview.toggle_flipped();

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_select);
        }

        // switching docks
        self.page_tracker.handle_input(game_io);

        // leaving scene
        if input_util.was_just_pressed(Input::Cancel) {
            let transition = crate::transitions::new_scene_pop(game_io);
            self.next_scene = NextScene::new_pop().with_transition(transition);

            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(&globals.sfx.cursor_cancel);
        }
    }

    fn update_preview(&mut self) {
        let page = self.page_tracker.active_page();

        let active_dock = &mut self.docks[page];

        let selected_index = active_dock.scroll_tracker.selected_index();
        let card = active_dock.cards.get(selected_index).cloned();

        self.card_preview.set_card(card);
    }
}

impl Scene for LibraryScene {
    fn next_scene(&mut self) -> &mut NextScene {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO) {
        // update camera
        self.camera.update();
        self.background.update();

        // update page_tracker
        let previously_scrolling = self.page_tracker.animating();
        self.page_tracker.update();

        if previously_scrolling && !self.page_tracker.animating() {
            self.update_preview();
        }

        // input
        if !game_io.is_in_transition() {
            self.handle_input(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw title
        self.frame.draw(&mut sprite_queue);
        SceneTitle::new("LIBRARY").draw(game_io, &mut sprite_queue);

        // draw docks
        sprite_queue.set_scissor(self.dock_scissor);

        for (i, offset_x) in self.page_tracker.visible_pages() {
            self.docks[i].draw(game_io, &mut sprite_queue, offset_x);
        }

        self.page_tracker.draw_page_arrows(&mut sprite_queue);

        sprite_queue.set_scissor(Rect::UNIT);

        // draw card preview
        self.card_preview.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

struct Dock {
    cards: Vec<Card>,
    scroll_tracker: ScrollTracker,
    dock_sprite: Sprite,
    dock_animator: Animator,
    label: Text,
    list_position: Vec2,
    context_menu_position: Vec2,
}

impl Dock {
    fn new(
        game_io: &GameIO,
        dock_offset: Vec2,
        available_packages: &[PackageId],
        card_class: CardClass,
    ) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // cards
        let cards: Vec<_> = available_packages
            .iter()
            .flat_map(|id| globals.card_packages.package(PackageNamespace::Local, id))
            .filter(|package| !package.hidden && package.card_properties.card_class == card_class)
            .map(|package| Card {
                package_id: package.package_info.id.clone(),
                code: String::new(),
            })
            .collect();

        // dock
        let mut dock_sprite = assets.new_sprite(game_io, ResourcePaths::LIBRARY_UI);
        let mut dock_animator = Animator::load_new(assets, ResourcePaths::LIBRARY_UI_ANIMATION);
        dock_animator.set_state("DOCK");
        dock_animator.set_loop_mode(AnimatorLoopMode::Loop);
        dock_animator.apply(&mut dock_sprite);

        dock_sprite.set_position(dock_offset);

        let list_point = dock_animator.point_or_zero("LIST");
        let list_position = dock_offset + list_point;

        let context_menu_point = dock_animator.point_or_zero("CONTEXT_MENU");
        let context_menu_position = dock_offset + context_menu_point;

        // label
        let label_text = match card_class {
            CardClass::Standard => "Standard",
            CardClass::Mega => "Mega",
            CardClass::Giga => "Giga",
            CardClass::Dark => "Dark",
            CardClass::Recipe => "Recipe",
        };

        let mut label = Text::new(game_io, FontName::Thick)
            .with_shadow_color(TEXT_DARK_SHADOW_COLOR)
            .with_str(label_text);

        let label_position = dock_animator.point_or_zero("LABEL") - dock_sprite.origin();
        label.style.bounds.set_position(label_position);

        // scroll tracker
        let mut scroll_tracker = ScrollTracker::new(game_io, 7);
        scroll_tracker.set_total_items(cards.len());

        let scroll_start_point = dock_animator.point_or_zero("SCROLL_START");
        let scroll_end_point = dock_animator.point_or_zero("SCROLL_END");

        let scroll_start = dock_offset + scroll_start_point;
        let scroll_end = dock_offset + scroll_end_point;

        scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        let cursor_start = list_position + Vec2::new(-7.0, 3.0);
        scroll_tracker.define_cursor(cursor_start, 16.0);

        Self {
            cards,
            scroll_tracker,
            dock_sprite,
            dock_animator,
            label,
            list_position,
            context_menu_position,
        }
    }

    fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, offset_x: f32) {
        let offset = Vec2::new(offset_x, 0.0);

        // draw dock
        self.dock_animator.update();
        self.dock_animator.apply(&mut self.dock_sprite);

        let dock_position = self.dock_sprite.position();
        self.dock_sprite.set_position(dock_position + offset);

        sprite_queue.draw_sprite(&self.dock_sprite);

        self.dock_sprite.set_position(dock_position);

        // draw label
        let label_bounds = self.label.style.bounds;
        self.label.style.bounds += offset;

        self.label.draw(game_io, sprite_queue);

        self.label.style.bounds = label_bounds;

        // draw cards
        for (i, card) in self.cards[self.scroll_tracker.view_range()]
            .iter()
            .enumerate()
        {
            let relative_index = i;

            let mut position = self.list_position + offset;
            position.y += relative_index as f32 * self.scroll_tracker.cursor_multiplier();

            card.draw_list_item(game_io, sprite_queue, position, Color::WHITE);
        }

        // draw scrollbar
        let (scroll_start, scroll_end) = self.scroll_tracker.scrollbar_definition();
        self.scroll_tracker
            .define_scrollbar(scroll_start + offset, scroll_end + offset);

        self.scroll_tracker.draw_scrollbar(sprite_queue);

        self.scroll_tracker
            .define_scrollbar(scroll_start, scroll_end);

        // draw cursor
        let (cursor_start, cursor_multiplier) = self.scroll_tracker.cursor_definition();
        self.scroll_tracker
            .define_cursor(cursor_start + offset, cursor_multiplier);

        self.scroll_tracker.draw_cursor(sprite_queue);

        self.scroll_tracker
            .define_cursor(cursor_start, cursor_multiplier);
    }

    fn draw_cursor(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.scroll_tracker.draw_cursor(sprite_queue);
    }
}
