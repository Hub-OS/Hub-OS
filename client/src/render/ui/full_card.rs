use crate::bindable::CardClass;
use crate::packages::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use framework::prelude::*;

use super::TextStyle;

pub struct FullCard {
    card_sprite: Sprite,
    card_animator: Animator,
    position: Vec2,
    preview_position: Vec2,
    previous_card: Option<Card>,
    current_card: Option<Card>,
    description_style: TextStyle,
    time: FrameTime,
}

impl FullCard {
    pub fn new(game_io: &GameIO, position: Vec2) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // card
        let mut card_sprite = assets.new_sprite(game_io, ResourcePaths::FULL_CARD);
        card_sprite.set_position(position);

        let mut card_animator = Animator::load_new(assets, ResourcePaths::FULL_CARD_ANIMATION);
        card_animator.set_state("standard");
        card_animator.set_loop_mode(AnimatorLoopMode::Loop);

        // preview
        let preview_point = card_animator.point("preview").unwrap_or_default();

        // description_style
        let mut description_style = TextStyle::new(game_io, FontStyle::Thin);
        description_style.line_spacing = 4.0;
        description_style.color = Color::BLACK;
        description_style.shadow_color = TEXT_TRANSPARENT_SHADOW_COLOR;

        let description_start = card_animator.point("description_start").unwrap_or_default();

        let description_end = card_animator.point("description_end").unwrap_or_default();

        let bounds_position = position + description_start;
        let bounds_size = description_end - description_start;
        description_style.bounds.set_position(bounds_position);
        description_style.bounds.set_size(bounds_size);

        Self {
            card_sprite,
            card_animator,
            position,
            preview_position: position + preview_point,
            previous_card: None,
            current_card: None,
            description_style,
            time: 0,
        }
    }

    pub fn position(&self) -> Vec2 {
        self.position
    }

    pub fn set_position(&mut self, position: Vec2) {
        let preview_offset = self.preview_position - self.position;
        let description_offset = self.description_style.bounds.position() - self.position;

        self.position = position;
        self.card_sprite.set_position(position);
        self.preview_position = position + preview_offset;
        (self.description_style.bounds).set_position(description_offset + position);
    }

    pub fn card(&self) -> Option<&Card> {
        self.current_card.as_ref()
    }

    pub fn set_card(&mut self, card: Option<Card>) {
        self.previous_card = self.current_card.take();
        self.current_card = card;
        self.time = 0;
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.time += 1;

        let progress = (self.time as f32 / 5.0).clamp(0.0, 1.0);

        let card = match progress > 0.5 {
            true => self.current_card.as_ref(),
            false => self.previous_card.as_ref(),
        };

        let package = card.as_ref().and_then(|card| package(game_io, card));

        // draw card sprite
        if let Some(package) = package {
            let state = state_for_package(package);
            self.card_animator.set_state(state);
        } else {
            self.card_animator.set_state("standard");
        }

        self.card_animator.sync_time(self.time);
        self.card_animator.apply(&mut self.card_sprite);
        sprite_queue.draw_sprite(&self.card_sprite);

        // draw preview info
        if let Some(card) = card {
            let scale = (0.5 - progress).abs() * 2.0;

            card.draw_preview(game_io, sprite_queue, self.preview_position, scale);
        }

        // draw description
        if let Some(package) = package {
            let description = &package.description;

            self.description_style
                .draw(game_io, sprite_queue, description);
        }
    }
}

fn package<'a>(game_io: &'a GameIO, card: &Card) -> Option<&'a CardPackage> {
    let globals = game_io.resource::<Globals>().unwrap();
    let package_manager = &globals.card_packages;

    package_manager.package_or_override(PackageNamespace::Local, &card.package_id)
}

fn state_for_package(package: &CardPackage) -> &'static str {
    match package.card_properties.card_class {
        CardClass::Standard => "standard",
        CardClass::Mega => "mega",
        CardClass::Giga => "giga",
        CardClass::Dark => "dark",
    }
}
