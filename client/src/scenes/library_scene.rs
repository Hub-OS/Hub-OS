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
    card_preview: FullCard,
    ui_input_tracker: UiInputTracker,
    active_dock: usize,
    docks: Vec<Dock>,
    next_scene: NextScene<Globals>,
}

impl LibraryScene {
    pub fn new(game_io: &mut GameIO<Globals>) -> Box<Self> {
        let globals = game_io.globals();
        let assets = &globals.assets;

        let mut camera = Camera::new(game_io);
        camera.snap(RESOLUTION_F * 0.5);

        // background
        let mut background_animator =
            Animator::load_new(assets, ResourcePaths::LIBRARY_BG_ANIMATION);
        background_animator.set_state("default");

        let background_sprite = assets.new_sprite(game_io, ResourcePaths::LIBRARY_BG);

        // docks
        let docks = vec![Dock::new(game_io, CardClass::Standard)];

        // card
        let card_position = background_animator.point("card").unwrap_or_default();

        let mut scene = Box::new(Self {
            camera,
            background: Background::new(background_animator, background_sprite),
            ui_input_tracker: UiInputTracker::new(),
            card_preview: FullCard::new(game_io, card_position),
            active_dock: 0,
            docks,
            next_scene: NextScene::None,
        });

        scene.update_preview();

        scene
    }

    fn handle_input(&mut self, game_io: &mut GameIO<Globals>) {
        self.ui_input_tracker.update(game_io);

        // dock scrolling
        let active_dock = &mut self.docks[self.active_dock];
        let scroll_tracker = &mut active_dock.scroll_tracker;

        let original_index = scroll_tracker.selected_index();

        scroll_tracker.handle_vertical_input(&self.ui_input_tracker);

        if original_index != scroll_tracker.selected_index() {
            self.update_preview();

            let globals = game_io.globals();
            globals.audio.play_sound(&globals.cursor_move_sfx);
        }

        let input_util = InputUtil::new(game_io);

        if input_util.was_just_pressed(Input::Right) && self.active_dock + 1 < self.docks.len() {
            self.active_dock += 1;
        }

        if input_util.was_just_pressed(Input::Left) && self.active_dock > 0 {
            self.active_dock -= 1;
        }

        // cancelling
        if input_util.was_just_pressed(Input::Cancel) {
            use crate::transitions::*;

            let transition = PushTransition::new(
                game_io,
                game_io.globals().default_sampler.clone(),
                Direction::Left,
                DEFAULT_PUSH_DURATION,
            );

            self.next_scene = NextScene::new_pop().with_transition(transition);

            let globals = game_io.globals();
            globals.audio.play_sound(&globals.menu_close_sfx);
        }
    }

    fn update_preview(&mut self) -> Option<()> {
        let active_dock = &mut self.docks[self.active_dock];

        let selected_index = active_dock.scroll_tracker.selected_index();
        let card = active_dock.cards.get(selected_index)?;

        self.card_preview.set_card(Some(card.clone()));

        None
    }
}

impl Scene<Globals> for LibraryScene {
    fn next_scene(&mut self) -> &mut NextScene<Globals> {
        &mut self.next_scene
    }

    fn update(&mut self, game_io: &mut GameIO<Globals>) {
        // update camera
        self.camera.update(game_io);

        // input
        if !game_io.is_in_transition() {
            self.handle_input(game_io);
        }
    }

    fn draw(&mut self, game_io: &mut GameIO<Globals>, render_pass: &mut RenderPass) {
        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::Multiply);

        // draw title
        SceneTitle::new("LIBRARY").draw(game_io, &mut sprite_queue);

        // draw docks
        self.docks[0].draw(game_io, &mut sprite_queue);

        self.card_preview.draw(game_io, &mut sprite_queue);

        render_pass.consume_queue(sprite_queue);
    }
}

struct Dock {
    cards: Vec<Card>,
    scroll_tracker: ScrollTracker,
    dock_sprite: Sprite,
    dock_animator: Animator,
    list_position: Vec2,
    context_menu_position: Vec2,
    page_arrows: Option<PageArrows>,
}

impl Dock {
    fn new(game_io: &GameIO<Globals>, card_class: CardClass) -> Self {
        let globals = game_io.globals();
        let assets = &globals.assets;

        // todo: display server card packages as well?
        let available_packages = globals.card_packages.local_packages();

        // cards
        // todo: filter using card_class
        let cards: Vec<_> = available_packages
            .filter(|id| {
                globals
                    .card_packages
                    .package_or_fallback(PackageNamespace::Server, id)
                    .is_some()
            })
            .map(|id| Card {
                package_id: id.clone(),
                code: String::new(),
            })
            .collect();

        // dock
        let mut dock_sprite = assets.new_sprite(game_io, ResourcePaths::LIBRARY_DOCK);
        let mut dock_animator = Animator::load_new(assets, ResourcePaths::LIBRARY_DOCK_ANIMATION);
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
        scroll_tracker.set_total_items(cards.len());

        let scroll_start_point = dock_animator.point("scroll_start").unwrap_or_default();
        let scroll_end_point = dock_animator.point("scroll_end").unwrap_or_default();

        let scroll_start = dock_offset + scroll_start_point;
        let scroll_end = dock_offset + scroll_end_point;

        scroll_tracker.define_scrollbar(scroll_start, scroll_end);

        let cursor_start = list_position + Vec2::new(-7.0, 3.0);
        scroll_tracker.define_cursor(cursor_start, 16.0);

        // page_arrows
        // let page_arrows = dock_animator
        //     .point("page_arrows")
        //     .map(|point| PageArrows::new(game_io, (dock_offset + point)));

        Self {
            cards,
            scroll_tracker,
            dock_sprite,
            dock_animator,
            list_position,
            context_menu_position,
            page_arrows: None,
        }
    }

    fn draw(&mut self, game_io: &GameIO<Globals>, sprite_queue: &mut SpriteColorQueue) {
        self.dock_animator.update();
        self.dock_animator.apply(&mut self.dock_sprite);
        sprite_queue.draw_sprite(&self.dock_sprite);

        for (i, card) in self.cards[self.scroll_tracker.view_range()]
            .iter()
            .enumerate()
        {
            let relative_index = i;

            let mut position = self.list_position;
            position.y += relative_index as f32 * self.scroll_tracker.cursor_multiplier();

            card.draw_list_item(game_io, sprite_queue, position);
        }

        self.scroll_tracker.draw_scrollbar(sprite_queue);
        self.scroll_tracker.draw_cursor(sprite_queue);

        if let Some(page_arrows) = self.page_arrows.as_mut() {
            page_arrows.draw(sprite_queue);
        }
    }

    fn draw_cursor(&mut self, sprite_queue: &mut SpriteColorQueue) {
        self.scroll_tracker.draw_cursor(sprite_queue);
    }
}
