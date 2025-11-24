use super::{BlockPreview, ElementSprite, FontName, Text};
use crate::bindable::{CardClass, Element};
use crate::packages::PackageNamespace;
use crate::render::ui::PackageListing;
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::async_task::AsyncTask;
use framework::prelude::*;
use packets::address_parsing::uri_encode;
use packets::structures::{BlockColor, PackageCategory, SwitchDriveSlot};

#[derive(Clone)]
pub enum PackagePreviewData {
    Card {
        class: CardClass,
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
        slot: Option<SwitchDriveSlot>,
        flat: bool,
        colors: Vec<BlockColor>,
        shape: Option<[bool; 5 * 5]>,
    },
    Encounter {
        recording: bool,
    },
    Pack,
    Library,
    Status,
    Resource,
    TileState,
    Unknown,
}

impl PackagePreviewData {
    fn has_image(&self) -> bool {
        !matches!(self, Self::Augment { .. })
    }

    pub fn category(&self) -> Option<PackageCategory> {
        match self {
            PackagePreviewData::Card { .. } => Some(PackageCategory::Card),
            PackagePreviewData::Player { .. } => Some(PackageCategory::Player),
            PackagePreviewData::Augment { .. } => Some(PackageCategory::Augment),
            PackagePreviewData::Encounter { .. } => Some(PackageCategory::Encounter),
            PackagePreviewData::Library | PackagePreviewData::Pack => {
                Some(PackageCategory::Library)
            }
            PackagePreviewData::Resource => Some(PackageCategory::Resource),
            PackagePreviewData::Status => Some(PackageCategory::Status),
            PackagePreviewData::TileState => Some(PackageCategory::TileState),
            _ => None,
        }
    }
}

pub struct PackagePreview {
    listing: PackageListing,
    position: Vec2,
    image_sprite: Option<Sprite>,
    image_task: Option<AsyncTask<Option<Vec<u8>>>>,
    image_requested: bool,
    sprites: Vec<Sprite>,
    text: Vec<Text>,
    dirty: bool,
}

impl PackagePreview {
    pub fn new(game_io: &GameIO, listing: PackageListing) -> Self {
        let image_sprite = if listing.local {
            Some(
                Self::resolve_local_preview_image(game_io, &listing).unwrap_or_else(|| {
                    let globals = game_io.resource::<Globals>().unwrap();
                    let assets = &globals.assets;
                    assets.new_sprite(game_io, ResourcePaths::BLANK)
                }),
            )
        } else {
            None
        };

        Self {
            listing,
            position: Vec2::ZERO,
            image_sprite,
            image_task: None,
            image_requested: false,
            sprites: Vec::new(),
            text: Vec::new(),
            dirty: true,
        }
    }

    fn resolve_local_preview_image(game_io: &GameIO, listing: &PackageListing) -> Option<Sprite> {
        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let ns = PackageNamespace::Local;

        match listing.preview_data.category()? {
            PackageCategory::Card => {
                let package = globals.card_packages.package(ns, &listing.id)?;
                Some(assets.new_sprite(game_io, &package.preview_texture_path))
            }
            PackageCategory::Player => {
                let package = globals.player_packages.package(ns, &listing.id)?;
                Some(assets.new_sprite(game_io, &package.preview_texture_path))
            }
            PackageCategory::Encounter => {
                let package = globals.encounter_packages.package(ns, &listing.id)?;
                Some(assets.new_sprite(game_io, &package.preview_texture_path))
            }
            PackageCategory::Resource => {
                let package = globals.resource_packages.package(ns, &listing.id)?;
                Some(assets.new_sprite(game_io, &package.preview_texture_path))
            }
            PackageCategory::Library => {
                let package = globals.library_packages.package(ns, &listing.id)?;
                Some(assets.new_sprite(game_io, &package.preview_texture_path))
            }
            PackageCategory::Status => {
                let package = globals.status_packages.package(ns, &listing.id)?;
                Some(assets.new_sprite(game_io, package.icon_texture_path.as_ref()?))
            }
            _ => None,
        }
    }

    pub fn with_position(mut self, position: Vec2) -> Self {
        self.set_position(position);
        self
    }

    pub fn listing(&self) -> &PackageListing {
        &self.listing
    }

    pub fn position(&self) -> Vec2 {
        self.position
    }

    pub fn set_position(&mut self, position: Vec2) {
        self.position = position;
        self.dirty = true;
    }

    fn request_image(&mut self, game_io: &GameIO) {
        self.image_requested = true;

        if !self.listing.preview_data.has_image() {
            let assets = &game_io.resource::<Globals>().unwrap().assets;
            self.image_sprite = Some(assets.new_sprite(game_io, ResourcePaths::BLANK));
        }

        let globals = game_io.resource::<Globals>().unwrap();

        let repo = globals.config.package_repo.clone();
        let encoded_id = uri_encode(self.listing.id.as_str());

        let uri = format!("{repo}/api/mods/{encoded_id}/preview");

        let task = game_io.spawn_local_task(async move { crate::http::request(&uri).await });

        self.image_task = Some(task);
    }

    fn manage_image(&mut self, game_io: &GameIO) {
        if self.image_sprite.is_some() || !self.listing.preview_data.has_image() {
            return;
        }

        if !self.image_requested {
            self.request_image(game_io);
            return;
        }

        let finished_request = matches!(&self.image_task, Some(task) if task.is_finished());

        if !finished_request {
            return;
        }

        let task = self.image_task.take().unwrap();
        let Some(bytes) = task.join().unwrap() else {
            return;
        };

        let sprite = if let Ok(texture) = Texture::load_from_memory(game_io, &bytes) {
            Sprite::new(game_io, texture)
        } else {
            let assets = &game_io.resource::<Globals>().unwrap().assets;
            assets.new_sprite(game_io, ResourcePaths::BLANK)
        };

        self.image_sprite = Some(sprite);
        self.dirty = true;
    }

    fn regenerate(&mut self, game_io: &GameIO) {
        self.sprites.clear();
        self.text.clear();
        self.dirty = false;

        let assets = &game_io.resource::<Globals>().unwrap().assets;

        let mut animator = Animator::load_new(assets, ResourcePaths::PACKAGE_PREVIEW_ANIMATION)
            .with_state("DEFAULT");

        let image_bounds = Rect::from_corners(
            animator.point_or_zero("IMAGE_START"),
            animator.point_or_zero("IMAGE_END"),
        );

        // background
        let mut background = assets.new_sprite(game_io, ResourcePaths::PACKAGE_PREVIEW);
        animator.apply(&mut background);
        self.sprites.push(background);

        // scale preview
        if !matches!(&self.listing.preview_data, PackagePreviewData::Card { .. })
            && let Some(sprite) = &mut self.image_sprite
        {
            // ratio preserving scale
            // todo: create helper method in Sprite?
            let size = sprite.frame().size();
            let scale = (image_bounds / size).size().min_element();
            sprite.set_scale(Vec2::new(scale, scale));
        }

        match &self.listing.preview_data {
            PackagePreviewData::Card {
                class,
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
                } else if matches!(class, CardClass::Recipe) {
                    let mut text = Text::new(game_io, FontName::Thick);
                    text.text = String::from("P.A.");

                    let text_size = text.measure().size;
                    text.style.bounds += image_bounds.center() - text_size * 0.5;

                    self.text.push(text);
                }

                const ITEM_MARGIN: f32 = 1.0;

                // primary element
                let mut element_sprite = ElementSprite::new(game_io, *element);

                let element_position = animator.point_or_zero("PRIMARY_ELEMENT");
                element_sprite.set_position(element_position);

                self.sprites.push(element_sprite);

                // secondary element
                if *secondary_element != Element::None {
                    let mut element_sprite = ElementSprite::new(game_io, *secondary_element);

                    let element_position = animator.point_or_zero("SECONDARY_ELEMENT");
                    element_sprite.set_position(element_position);

                    self.sprites.push(element_sprite);
                }

                // damage
                let mut text = Text::new(game_io, FontName::Thick);
                text.text = format!("{damage}");

                let text_anchor = animator.point_or_zero("DAMAGE_END");
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

                let element_position = animator.point_or_zero("PRIMARY_ELEMENT");
                element_sprite.set_position(element_position);
                self.sprites.push(element_sprite);

                // health bg
                let mut health_sprite = assets.new_sprite(game_io, ResourcePaths::PACKAGE_PREVIEW);

                animator.set_state("HEALTH");
                animator.apply(&mut health_sprite);

                self.sprites.push(health_sprite);

                // health
                let mut text = Text::new(game_io, FontName::Thick);
                text.text = format!("{health}");

                let text_anchor = animator.point_or_zero("TEXT_END") - animator.origin();

                let text_size = text.measure().size;
                text.style.bounds += text_anchor - text_size;
                self.text.push(text);
            }
            PackagePreviewData::Augment {
                flat,
                colors,
                shape: Some(shape),
                ..
            } => {
                // generate shape preview
                let preview_color = colors.first().cloned().unwrap_or_default();
                let block_preview = BlockPreview::new(game_io, preview_color, *flat, *shape);
                let position = image_bounds.center() - block_preview.size();

                let sprites: Vec<Sprite> =
                    block_preview.with_position(position).with_scale(2.0).into();

                self.sprites.extend(sprites);

                // generate color list preview
                let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLOCKS_UI);
                sprite.set_scale(Vec2::new(2.0, 2.0));

                let mut animator =
                    Animator::load_new(assets, ResourcePaths::BLOCKS_PREVIEW_ANIMATION);
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
            PackagePreviewData::Augment {
                slot: Some(slot), ..
            } => {
                let assets = &game_io.resource::<Globals>().unwrap().assets;
                let mut sprite = assets.new_sprite(game_io, ResourcePaths::PACKAGE_PREVIEW);

                let mut animator =
                    Animator::load_new(assets, ResourcePaths::PACKAGE_PREVIEW_ANIMATION);

                let state = match slot {
                    SwitchDriveSlot::Head => "SWITCH_DRIVE_HEAD",
                    SwitchDriveSlot::Body => "SWITCH_DRIVE_BODY",
                    SwitchDriveSlot::Arms => "SWITCH_DRIVE_ARMS",
                    SwitchDriveSlot::Legs => "SWITCH_DRIVE_LEGS",
                };

                animator.set_state(state);
                animator.apply(&mut sprite);

                sprite.set_position(image_bounds.top_left());

                self.sprites.push(sprite);
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
