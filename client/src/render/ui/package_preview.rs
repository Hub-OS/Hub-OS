use super::{BlockPreview, ElementSprite, FontStyle, Text};
use crate::bindable::Element;
use crate::render::ui::PackageListing;
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::async_task::AsyncTask;
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use packets::structures::{BlockColor, PackageCategory};

#[derive(Clone)]
pub enum PackagePreviewData {
    Card {
        codes: Vec<String>,
        element: Element,
        secondary_element: Element,
        damage: i32,
    },
    Player {
        element: Element,
        health: i32,
    },
    Augment {
        flat: bool,
        colors: Vec<BlockColor>,
        shape: [bool; 5 * 5],
    },
    Encounter,
    Pack,
    Library,
    Resources,
    Unknown,
}

impl PackagePreviewData {
    fn has_image(&self) -> bool {
        matches!(
            self,
            Self::Card { .. } | Self::Player { .. } | Self::Encounter | Self::Pack
        )
    }

    pub fn category(&self) -> Option<PackageCategory> {
        match self {
            PackagePreviewData::Card { .. } => Some(PackageCategory::Card),
            PackagePreviewData::Player { .. } => Some(PackageCategory::Player),
            PackagePreviewData::Augment { .. } => Some(PackageCategory::Augment),
            PackagePreviewData::Encounter => Some(PackageCategory::Encounter),
            PackagePreviewData::Library | PackagePreviewData::Pack => {
                Some(PackageCategory::Library)
            }
            PackagePreviewData::Resources => Some(PackageCategory::Resource),
            _ => None,
        }
    }
}

pub struct PackagePreview {
    listing: PackageListing,
    position: Vec2,
    image_sprite: Option<Sprite>,
    image_task: Option<AsyncTask<Vec<u8>>>,
    sprites: Vec<Sprite>,
    text: Vec<Text>,
    dirty: bool,
}

impl PackagePreview {
    pub fn new(listing: PackageListing) -> Self {
        Self {
            listing,
            position: Vec2::ZERO,
            image_sprite: None,
            image_task: None,
            sprites: Vec::new(),
            text: Vec::new(),
            dirty: true,
        }
    }

    pub fn with_position(mut self, position: Vec2) -> Self {
        self.set_position(position);
        self
    }

    pub fn listing(&self) -> &PackageListing {
        &self.listing
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.position = position;
        self.dirty = true;
    }

    fn request_image(&mut self, game_io: &GameIO) {
        if !self.listing.preview_data.has_image() {
            let assets = &game_io.resource::<Globals>().unwrap().assets;
            self.image_sprite = Some(assets.new_sprite(game_io, ResourcePaths::BLANK));
        }

        let globals = game_io.resource::<Globals>().unwrap();

        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(self.listing.id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}/preview");

        let task = game_io
            .spawn_local_task(async move { crate::http::request(&uri).await.unwrap_or_default() });

        self.image_task = Some(task);
    }

    fn manage_image(&mut self, game_io: &GameIO) {
        let finished_request = matches!(&self.image_task, Some(task) if task.is_finished());

        if self.image_sprite.is_none() && self.image_task.is_none() {
            self.request_image(game_io);
        } else if finished_request {
            let task = self.image_task.take().unwrap();
            let bytes = task.join().unwrap();

            let sprite = if let Ok(texture) = Texture::load_from_memory(game_io, &bytes) {
                Sprite::new(game_io, texture)
            } else {
                let assets = &game_io.resource::<Globals>().unwrap().assets;
                assets.new_sprite(game_io, ResourcePaths::BLANK)
            };

            self.image_sprite = Some(sprite);
            self.dirty = true;
        }
    }

    fn regenerate(&mut self, game_io: &GameIO) {
        self.sprites.clear();
        self.text.clear();
        self.dirty = false;

        let assets = &game_io.resource::<Globals>().unwrap().assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::PACKAGES_PREVIEW_ANIMATION)
            .with_state("DEFAULT");

        let image_bounds = Rect::from_corners(
            animator.point("IMAGE_START").unwrap_or_default(),
            animator.point("IMAGE_END").unwrap_or_default(),
        );

        // background
        let mut background = assets.new_sprite(game_io, ResourcePaths::PACKAGES_PREVIEW);
        animator.apply(&mut background);
        self.sprites.push(background);

        // scale preview
        if !matches!(&self.listing.preview_data, PackagePreviewData::Card { .. }) {
            if let Some(sprite) = &mut self.image_sprite {
                // ratio preserving scale
                // todo: create helper method in Sprite?
                let size = sprite.frame().size();
                let scale = (image_bounds / size).size().min_element();
                sprite.set_scale(Vec2::new(scale, scale));
            }
        }

        match &self.listing.preview_data {
            PackagePreviewData::Card {
                element,
                secondary_element,
                damage,
                ..
            } => {
                // image
                if let Some(mut sprite) = self.image_sprite.clone() {
                    sprite.set_origin(Vec2::new(sprite.size().x * 0.5, 0.0));
                    sprite.set_position(image_bounds.top_center());
                    self.sprites.push(sprite);
                }

                const ITEM_MARGIN: f32 = 1.0;

                // primary element
                let mut element_sprite = ElementSprite::new(game_io, *element);

                let element_position = animator.point("PRIMARY_ELEMENT").unwrap_or_default();
                element_sprite.set_position(element_position);

                self.sprites.push(element_sprite);

                // secondary element
                if *secondary_element != Element::None {
                    let mut element_sprite = ElementSprite::new(game_io, *secondary_element);

                    let element_position = animator.point("SECONDARY_ELEMENT").unwrap_or_default();
                    element_sprite.set_position(element_position);

                    self.sprites.push(element_sprite);
                }

                // damage
                let mut text = Text::new(game_io, FontStyle::Thick);
                text.text = format!("{damage}");

                let text_anchor = animator.point("DAMAGE_END").unwrap_or_default();

                let text_size = text.measure().size;
                text.style.bounds += text_anchor - text_size;
                self.text.push(text);
            }
            PackagePreviewData::Player { element, health } => {
                if let Some(mut sprite) = self.image_sprite.clone() {
                    sprite.set_origin(sprite.frame().size() * 0.5);
                    sprite.set_position(image_bounds.center());
                    self.sprites.push(sprite);
                }

                // element
                let mut element_sprite = ElementSprite::new(game_io, *element);

                let element_position = animator.point("PRIMARY_ELEMENT").unwrap_or_default();
                element_sprite.set_position(element_position);
                self.sprites.push(element_sprite);

                // health bg
                let mut health_sprite = assets.new_sprite(game_io, ResourcePaths::PACKAGES_PREVIEW);

                animator.set_state("HEALTH");
                animator.apply(&mut health_sprite);

                self.sprites.push(health_sprite);

                // health
                let mut text = Text::new(game_io, FontStyle::Thick);
                text.text = format!("{health}");

                let text_anchor =
                    animator.point("TEXT_END").unwrap_or_default() - animator.origin();

                let text_size = text.measure().size;
                text.style.bounds += text_anchor - text_size;
                self.text.push(text);
            }
            PackagePreviewData::Augment {
                flat,
                colors,
                shape,
            } => {
                // generate shape preview
                let preview_color = colors.first().cloned().unwrap_or_default();
                let block_preview = BlockPreview::new(game_io, preview_color, *flat, *shape);
                let position = image_bounds.center() - block_preview.size();

                let sprites: Vec<Sprite> =
                    block_preview.with_position(position).with_scale(2.0).into();

                self.sprites.extend(sprites);

                // generate color list preview
                let mut sprite = assets.new_sprite(game_io, ResourcePaths::CUSTOMIZE_UI);
                sprite.set_scale(Vec2::new(2.0, 2.0));

                let mut animator =
                    Animator::load_new(assets, ResourcePaths::CUSTOMIZE_PREVIEW_ANIMATION);
                animator.set_state(preview_color.flat_state());
                animator.apply(&mut sprite);

                let mut position = image_bounds.bottom_left();
                position.x += 2.0;
                position.y -= sprite.size().y + 2.0;

                for color in colors {
                    animator.set_state(color.flat_state());
                    animator.apply(&mut sprite);

                    sprite.set_position(position);
                    self.sprites.push(sprite.clone());

                    position.x += sprite.size().x + 2.0;
                }
            }
            _ => {
                if let Some(mut sprite) = self.image_sprite.clone() {
                    sprite.set_origin(sprite.frame().size() * 0.5);
                    sprite.set_position(image_bounds.center());
                    self.sprites.push(sprite);
                }
            }
        }
    }

    pub fn update(&mut self, game_io: &GameIO) {
        self.manage_image(game_io);
    }

    pub fn draw(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        if self.dirty {
            self.regenerate(game_io);
        }

        for sprite in &mut self.sprites {
            let position = sprite.position();

            sprite.set_position(self.position + position);
            sprite_queue.draw_sprite(sprite);

            sprite.set_position(position);
        }

        for text in &mut self.text {
            let bounds = text.style.bounds;

            text.style.bounds += self.position;
            text.draw(game_io, sprite_queue);

            text.style.bounds = bounds;
        }
    }
}
