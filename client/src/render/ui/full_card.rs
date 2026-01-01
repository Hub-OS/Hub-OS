use crate::bindable::CardClass;
use crate::packages::*;
use crate::render::ui::*;
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use framework::prelude::*;

const MAX_ART_TIME: FrameTime = 5;

pub struct FullCard {
    card_sprite: Sprite,
    card_animator: Animator,
    card_property_preview: CardPropertyPreview,
    next_recipe_sprite: Sprite,
    next_recipe_animator: Animator,
    position: Vec2,
    preview_position: Vec2,
    previous_card: Option<Card>,
    current_card: Option<Card>,
    description_style: TextStyle,
    art_time: FrameTime,
    flipped: bool,
    display_recipes: bool,
    recipe_index: usize,
}

impl FullCard {
    pub fn new(game_io: &GameIO, position: Vec2) -> Self {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        // card
        let mut card_sprite = assets.new_sprite(game_io, ResourcePaths::FULL_CARD);
        card_sprite.set_position(position);

        let mut card_animator = Animator::load_new(assets, ResourcePaths::FULL_CARD_ANIMATION);
        card_animator.set_state("STANDARD");
        card_animator.set_loop_mode(AnimatorLoopMode::Loop);

        // preview
        let preview_point = card_animator.point_or_zero("PREVIEW");

        // description_style
        let mut description_style = TextStyle::new(game_io, FontName::Thin);
        description_style.line_spacing = 4.0;
        description_style.color = TEXTBOX_TEXT_COLOR;
        description_style.shadow_color = TEXT_TRANSPARENT_SHADOW_COLOR;

        let description_start = card_animator.point_or_zero("DESCRIPTION_START");

        let description_end = card_animator.point_or_zero("DESCRIPTION_END");

        let bounds_position = position + description_start;
        let bounds_size = description_end - description_start;
        description_style.bounds.set_position(bounds_position);
        description_style.bounds.set_size(bounds_size);

        // recipe cycling
        let mut next_recipe_sprite = assets.new_sprite(game_io, ResourcePaths::TEXTBOX_NEXT);
        let mut next_recipe_animator =
            Animator::load_new(assets, ResourcePaths::TEXTBOX_NEXT_ANIMATION);
        next_recipe_animator.set_state("DEFAULT");
        next_recipe_animator.set_loop_mode(AnimatorLoopMode::Loop);

        next_recipe_sprite
            .set_position(position + card_animator.point_or_zero("RECIPE_CYCLE_INDICATOR"));

        Self {
            card_sprite: card_sprite.clone(),
            card_animator,
            card_property_preview: CardPropertyPreview::new(game_io),
            next_recipe_sprite,
            next_recipe_animator,
            position,
            preview_position: position + preview_point,
            previous_card: None,
            current_card: None,
            description_style,
            art_time: 0,
            flipped: false,
            display_recipes: false,
            recipe_index: 0,
        }
    }

    pub fn set_display_recipes(&mut self, enabled: bool) {
        self.display_recipes = enabled;
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

    pub fn set_card(&mut self, game_io: &GameIO, card: Option<Card>) {
        let package = card.as_ref().and_then(|card| package(game_io, card));
        self.card_property_preview.set_package(package);

        self.previous_card = self.current_card.take();
        self.current_card = card;
        self.art_time = 0;
        self.recipe_index = 0;
    }

    pub fn cycle_recipe(&mut self) {
        self.previous_card.clone_from(&self.current_card);
        self.art_time = 0;
        self.recipe_index += 1;
        self.next_recipe_animator.sync_time(0);
    }

    pub fn set_flipped(&mut self, flipped: bool) {
        self.flipped = flipped;
    }

    pub fn toggle_flipped(&mut self) {
        self.set_flipped(!self.flipped);
    }

    pub fn flipped(&mut self) -> bool {
        self.flipped
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.art_time += 1;

        let art_progress = (self.art_time as f32 / MAX_ART_TIME as f32).clamp(0.0, 1.0);

        let card = match art_progress > 0.5 {
            true => self.current_card.as_ref(),
            false => self.previous_card.as_ref(),
        };

        let package = card.as_ref().and_then(|card| package(game_io, card));

        // draw card sprite
        if self.flipped {
            if let Some(package) = package {
                let state = back_state_for_package(package);
                self.card_animator.set_state(state);
            } else {
                self.card_animator.set_state("STANDARD_BACK");
            }
        } else if let Some(package) = package {
            let state = state_for_package(package);
            self.card_animator.set_state(state);
        } else {
            self.card_animator.set_state("STANDARD");
        }

        self.card_animator.sync_time(self.art_time);
        self.card_animator.apply(&mut self.card_sprite);
        sprite_queue.draw_sprite(&self.card_sprite);

        if self.flipped {
            self.card_property_preview.draw(sprite_queue, self.position);
        } else {
            // draw preview info
            if let Some(card) = card {
                let scale = (0.5 - art_progress).abs() * 2.0;

                if self.display_recipes && package.is_some_and(|p| !p.recipes.is_empty()) {
                    card.draw_recipe_preview(
                        game_io,
                        sprite_queue,
                        PackageNamespace::Local,
                        self.preview_position,
                        scale,
                        self.recipe_index,
                    );

                    // render cycle recipe indicator
                    if package.is_some_and(|p| p.recipes.len() > 1) {
                        self.next_recipe_animator
                            .apply(&mut self.next_recipe_sprite);

                        if self.art_time > MAX_ART_TIME / 2 {
                            self.next_recipe_animator.update();
                        }

                        sprite_queue.draw_sprite(&self.next_recipe_sprite);
                    }
                } else {
                    let card_preview = CardPreview::new(card, self.preview_position, scale);
                    card_preview.draw(game_io, sprite_queue);
                }
            }

            // draw description
            if let Some(package) = package {
                let description = &package.description;

                self.description_style
                    .draw(game_io, sprite_queue, description);
            }
        }
    }
}

fn package<'a>(game_io: &'a GameIO, card: &Card) -> Option<&'a CardPackage> {
    let globals = game_io.resource::<Globals>().unwrap();
    let package_manager = &globals.card_packages;

    package_manager.package_or_fallback(PackageNamespace::Local, &card.package_id)
}

fn state_for_package(package: &CardPackage) -> &'static str {
    match package.card_properties.card_class {
        CardClass::Standard => "STANDARD",
        CardClass::Mega => "MEGA",
        CardClass::Giga => "GIGA",
        CardClass::Dark => "DARK",
        CardClass::Recipe => "STANDARD",
    }
}

fn back_state_for_package(package: &CardPackage) -> &'static str {
    match package.card_properties.card_class {
        CardClass::Standard => "STANDARD_BACK",
        CardClass::Mega => "MEGA_BACK",
        CardClass::Giga => "GIGA_BACK",
        CardClass::Dark => "DARK_BACK",
        CardClass::Recipe => "STANDARD_BACK",
    }
}
