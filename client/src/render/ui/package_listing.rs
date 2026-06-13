use crate::packages::PackageNamespace;
use crate::render::ui::{ElementSprite, FontName, ImmutableUiNode, PackagePreviewData, TextStyle};
use crate::render::{Animator, SpriteColorQueue};
use crate::requests::PackageResponse;
use crate::resources::{AssetManager, Globals, ResourcePaths, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;
use packets::structures::{FileHash, PackageCategory, PackageId, SwitchDriveSlot};
use std::sync::Arc;

#[derive(Clone)]
pub struct PackageListing {
    pub local: bool,
    pub id: PackageId,
    pub past_ids: Vec<PackageId>,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub creator: Arc<str>,
    pub hash: FileHash,
    pub preview_data: PackagePreviewData,
    pub dependencies: Vec<(PackageCategory, PackageId)>,
}

impl From<PackageResponse> for PackageListing {
    fn from(meta: PackageResponse) -> Self {
        let preview_data = match meta.package.category.as_str() {
            "player" => PackagePreviewData::Player {
                element: meta.package.element,
                health: meta.package.health,
            },
            "card" => PackagePreviewData::Card {
                class: meta.package.card_class,
                codes: meta.package.codes,
                element: meta.package.element,
                secondary_element: meta.package.secondary_element,
                damage: meta.package.damage,
                can_boost: meta.package.can_boost,
            },
            "augment" => PackagePreviewData::Augment {
                slot: (!meta.package.slot.is_empty())
                    .then(|| SwitchDriveSlot::from(meta.package.slot)),
                flat: meta.package.flat,
                colors: meta.package.colors,
                shape: meta.package.shape,
            },
            "encounter" => PackagePreviewData::Encounter {
                recording: !meta.package.recording_path.is_empty(),
            },
            "library" => PackagePreviewData::Library,
            "pack" => PackagePreviewData::Pack,
            "resource" => PackagePreviewData::Resource,
            "status" => PackagePreviewData::Status,
            "tile_state" => PackagePreviewData::TileState,
            _ => PackagePreviewData::Unknown,
        };

        let mut dependencies = Vec::new();

        let mut extend_dependencies = |category: PackageCategory, ids: Vec<PackageId>| {
            dependencies.extend(ids.into_iter().map(|id| (category, id)))
        };

        extend_dependencies(PackageCategory::Augment, meta.dependencies.augments);
        extend_dependencies(PackageCategory::Encounter, meta.dependencies.encounters);
        extend_dependencies(PackageCategory::Card, meta.dependencies.cards);
        extend_dependencies(PackageCategory::Library, meta.dependencies.libraries);
        extend_dependencies(PackageCategory::Status, meta.dependencies.statuses);
        extend_dependencies(PackageCategory::TileState, meta.dependencies.tile_states);

        let mut long_name = meta.package.long_name;
        let name = meta.package.name;

        if long_name.is_empty() {
            long_name = name.clone();
        }

        let mut description = meta.package.long_description;

        if description.is_empty() {
            description = meta.package.description;
        }

        Self {
            local: false,
            id: meta.package.id,
            past_ids: meta.package.past_ids,
            name: name.into(),
            long_name: long_name.into(),
            description: description.into(),
            creator: meta.creator.into(),
            hash: meta.hash,
            preview_data,
            dependencies,
        }
    }
}

impl ImmutableUiNode for PackageListing {
    fn draw_bounded(&self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue, bounds: Rect) {
        let mut used_x = 1.0;

        match &self.preview_data {
            PackagePreviewData::Card {
                element,
                secondary_element,
                damage,
                ..
            } => {
                let mut position = bounds.top_right();

                // draw secondary element
                let mut sprite = ElementSprite::new(game_io, *secondary_element);
                let element_size = sprite.size();
                position.x -= element_size.x;
                position.y -= 1.0;

                sprite.set_position(position);
                sprite_queue.draw_sprite(&sprite);

                // draw element
                let mut sprite = ElementSprite::new(game_io, *element);
                position.x -= element_size.x + 1.0;
                sprite.set_position(position);

                sprite_queue.draw_sprite(&sprite);

                // draw damage
                if *damage != 0 {
                    let mut text_style = TextStyle::new(game_io, FontName::Thick);
                    text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;

                    let damage_space = text_style.measure("0000").size;
                    let damage_text = format!("{damage}");
                    text_style.monospace = true;

                    text_style.bounds.x =
                        position.x - text_style.measure(&damage_text).size.x - 2.0;
                    text_style.bounds.y = bounds.y + 1.0;
                    text_style.bounds.set_size(damage_space);
                    text_style.draw(game_io, sprite_queue, &damage_text);

                    used_x += damage_space.x;
                }

                used_x += sprite.size().x * 2.0;
            }
            PackagePreviewData::Player { element, .. } => {
                let category_sprite = create_category_sprite(game_io, bounds, "PLAYER");
                sprite_queue.draw_sprite(&category_sprite);

                let mut element_sprite = ElementSprite::new(game_io, *element);
                let mut element_position = category_sprite.position();
                element_position.x -= element_sprite.size().x + 1.0;
                element_sprite.set_position(element_position);
                sprite_queue.draw_sprite(&element_sprite);

                used_x += element_sprite.size().x + 1.0 + category_sprite.size().x;
            }
            PackagePreviewData::Augment { shape, slot, .. } => {
                let mut offset = Vec2::ZERO;

                if let Some(slot) = slot {
                    let state = match slot {
                        SwitchDriveSlot::Head => "SWITCH_DRIVE_HEAD",
                        SwitchDriveSlot::Body => "SWITCH_DRIVE_BODY",
                        SwitchDriveSlot::Arms => "SWITCH_DRIVE_ARMS",
                        SwitchDriveSlot::Legs => "SWITCH_DRIVE_LEGS",
                    };

                    let mut sprite = create_category_sprite(game_io, bounds, state);
                    sprite.set_position(sprite.position() + offset);
                    sprite_queue.draw_sprite(&sprite);

                    used_x += sprite.size().x;
                    offset.x -= sprite.size().x;
                }

                if shape.is_some() {
                    let mut sprite = create_category_sprite(game_io, bounds, "BLOCKS");
                    sprite.set_position(sprite.position() + offset);
                    sprite_queue.draw_sprite(&sprite);

                    used_x += sprite.size().x;
                    // offset.x -= sprite.size().x;
                }

                if shape.is_none() && slot.is_none() {
                    let sprite = create_category_sprite(game_io, bounds, "LIBRARY");
                    sprite_queue.draw_sprite(&sprite);

                    used_x += sprite.size().x;
                }
            }
            PackagePreviewData::Encounter { recording } => {
                let state = if *recording { "RECORDING" } else { "ENCOUNTER" };
                let category_sprite = create_category_sprite(game_io, bounds, state);
                sprite_queue.draw_sprite(&category_sprite);

                used_x += category_sprite.size().x;
            }
            PackagePreviewData::Pack => {
                let category_sprite = create_category_sprite(game_io, bounds, "PACK");
                sprite_queue.draw_sprite(&category_sprite);

                used_x += category_sprite.size().x;
            }
            PackagePreviewData::Resource => {
                let category_sprite = create_category_sprite(game_io, bounds, "RESOURCE");
                sprite_queue.draw_sprite(&category_sprite);

                used_x += category_sprite.size().x;
            }
            _ => {
                let category_sprite = create_category_sprite(game_io, bounds, "LIBRARY");
                sprite_queue.draw_sprite(&category_sprite);

                used_x += category_sprite.size().x;
            }
        }

        // render an indicator for mods we haven't installed
        let is_installed = self
            .preview_data
            .category()
            .map(|category| {
                let globals = Globals::from_resources(game_io);
                globals
                    .package_info(category, PackageNamespace::Local, &self.id)
                    .is_some()
            })
            .unwrap_or_default();

        let state = if is_installed { "INSTALLED" } else { "INSTALL" };

        let assets = &Globals::from_resources(game_io).assets;
        let mut status_sprite = assets.new_sprite(game_io, ResourcePaths::INSTALL_STATUS);
        Animator::load_new(assets, ResourcePaths::INSTALL_STATUS_ANIMATION)
            .with_state(state)
            .apply(&mut status_sprite);

        status_sprite.set_position(bounds.top_left() + Vec2::new(-1.0, 0.0));
        sprite_queue.draw_sprite(&status_sprite);

        // render the name
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.shadow_color = TEXT_DARK_SHADOW_COLOR;
        text_style.ellipsis = true;

        let status_space = status_sprite.size().x;
        text_style.bounds = bounds + Vec2::new(status_space, 1.0);
        text_style.bounds.width -= used_x + status_space;
        text_style.draw(game_io, sprite_queue, &self.long_name);
    }

    fn measure_ui_size(&self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }
}

fn create_category_sprite(game_io: &GameIO, bounds: Rect, state: &str) -> Sprite {
    let assets = &Globals::from_resources(game_io).assets;

    let mut category_sprite = assets.new_sprite(game_io, ResourcePaths::PACKAGE_ICON);

    Animator::load_new(assets, ResourcePaths::PACKAGE_ICON_ANIMATION)
        .with_state(state)
        .apply(&mut category_sprite);

    let mut position = bounds.top_right();
    position.x -= category_sprite.size().x;
    position.y -= 1.0;
    category_sprite.set_position(position);

    category_sprite
}
