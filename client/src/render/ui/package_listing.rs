use super::{ElementSprite, FontName, TextStyle, UiNode};
use crate::bindable::CardClass;
use crate::packages::PackageNamespace;
use crate::render::ui::PackagePreviewData;
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths, TEXT_DARK_SHADOW_COLOR};
use framework::prelude::*;
use packets::structures::{FileHash, PackageCategory, PackageId, SwitchDriveSlot};
use serde_json as json;
use std::sync::Arc;

#[derive(Clone)]
pub struct PackageListing {
    pub local: bool,
    pub id: PackageId,
    pub name: Arc<str>,
    pub long_name: Arc<str>,
    pub description: Arc<str>,
    pub creator: Arc<str>,
    pub hash: FileHash,
    pub preview_data: PackagePreviewData,
    pub dependencies: Vec<(PackageCategory, PackageId)>,
}

impl From<&json::Value> for PackageListing {
    fn from(value: &json::Value) -> Self {
        let Some(package_table) = value.get("package") else {
            let name: Arc<str> = "Broken package".into();
            return Self {
                local: false,
                id: PackageId::new_blank(),
                name: name.clone(),
                long_name: name,
                description: "???".into(),
                creator: Default::default(),
                hash: FileHash::ZERO,
                preview_data: PackagePreviewData::Unknown,
                dependencies: Vec::new(),
            };
        };

        let category = get_str(package_table, "category");

        let preview_data = match category {
            "player" => PackagePreviewData::Player {
                element: get_str(package_table, "element").into(),
                health: get_i32(package_table, "health"),
            },
            "card" => PackagePreviewData::Card {
                class: CardClass::from(get_str(package_table, "card_class")),
                codes: get_string_vec(package_table, "codes"),
                element: get_str(package_table, "element").into(),
                secondary_element: get_str(package_table, "secondary_element").into(),
                damage: get_i32(package_table, "damage"),
                can_boost: get_bool(package_table, "can_boost").unwrap_or(true),
            },
            "augment" => PackagePreviewData::Augment {
                slot: package_table
                    .get("slot")
                    .and_then(|value| value.as_str())
                    .map(SwitchDriveSlot::from),
                flat: package_table
                    .get("flat")
                    .and_then(|value| value.as_bool())
                    .unwrap_or_default(),
                colors: {
                    // convert list of strings into BlockColor
                    map_array_values(package_table, "colors", |value| {
                        value.as_str().unwrap_or_default().into()
                    })
                    .collect()
                },
                shape: {
                    // convert list of list i64 into [bool; 25]
                    let bools: Option<Vec<bool>> = package_table
                        .get("shape")
                        .and_then(|value| value.as_array())
                        .map(|value| {
                            value
                                .iter()
                                .flat_map(|v| {
                                    Some(
                                        v.as_array()?
                                            .iter()
                                            .map(|v| v.as_i64().unwrap_or_default() != 0),
                                    )
                                })
                                .flatten()
                                .collect()
                        });

                    bools.and_then(|bools| bools.try_into().ok())
                },
            },
            "encounter" => PackagePreviewData::Encounter {
                recording: !get_str(package_table, "recording_path").is_empty(),
            },
            "library" => PackagePreviewData::Library,
            "pack" => PackagePreviewData::Pack,
            "resource" => PackagePreviewData::Resource,
            "status" => PackagePreviewData::Status,
            "tile_state" => PackagePreviewData::TileState,
            _ => PackagePreviewData::Unknown,
        };

        let mut description = get_str(package_table, "long_description");

        if description.is_empty() {
            description = get_str(package_table, "description");
        }

        let mut dependencies = Vec::new();

        if let Some(dependencies_table) = value.get("dependencies") {
            let into_id = |id: &json::Value| id.as_str().unwrap_or_default().into();

            dependencies.extend(map_array_values(dependencies_table, "augments", |id| {
                (PackageCategory::Augment, into_id(id))
            }));
            dependencies.extend(map_array_values(dependencies_table, "encounters", |id| {
                (PackageCategory::Encounter, into_id(id))
            }));
            dependencies.extend(map_array_values(dependencies_table, "cards", |id| {
                (PackageCategory::Card, into_id(id))
            }));
            dependencies.extend(map_array_values(dependencies_table, "libraries", |id| {
                (PackageCategory::Library, into_id(id))
            }));
            dependencies.extend(map_array_values(dependencies_table, "statuses", |id| {
                (PackageCategory::Status, into_id(id))
            }));
            dependencies.extend(map_array_values(dependencies_table, "tile_states", |id| {
                (PackageCategory::TileState, into_id(id))
            }));
        }

        let mut long_name = get_str(package_table, "long_name");
        let name = get_str(package_table, "name");

        if long_name.is_empty() {
            long_name = name;
        }

        Self {
            local: false,
            id: get_str(package_table, "id").into(),
            name: name.into(),
            long_name: long_name.into(),
            description: description.into(),
            creator: get_unknown_as_string(value, "creator").into(),
            hash: FileHash::from_hex(get_str(value, "hash")).unwrap_or_default(),
            preview_data,
            dependencies,
        }
    }
}

fn get_unknown_as_string(table: &json::Value, key: &str) -> String {
    let Some(value) = table.get(key) else {
        return String::new();
    };

    (value.as_str().map(|s| s.to_string()))
        .or_else(|| Some(format!("{}", value.as_i64()?)))
        .unwrap_or_default()
}

fn get_str<'a>(table: &'a json::Value, key: &str) -> &'a str {
    table
        .get(key)
        .and_then(|value| value.as_str())
        .unwrap_or_default()
}

fn get_i32(table: &json::Value, key: &str) -> i32 {
    table
        .get(key)
        .and_then(|value| value.as_i64())
        .unwrap_or_default() as i32
}

fn get_bool(table: &json::Value, key: &str) -> Option<bool> {
    table.get(key).and_then(|value| value.as_bool())
}

fn map_array_values<'a, V, F>(
    table: &'a json::Value,
    key: &str,
    callback: F,
) -> impl Iterator<Item = V> + 'a
where
    F: Fn(&'a json::Value) -> V + 'a,
{
    table
        .get(key)
        .and_then(move |value| value.as_array().map(|v| v.iter().map(callback)))
        .into_iter()
        .flatten()
}

fn get_string_vec(table: &json::Value, key: &str) -> Vec<String> {
    map_array_values(table, key, |value| {
        value.as_str().unwrap_or_default().to_string()
    })
    .collect()
}

impl UiNode for PackageListing {
    fn draw_bounded(
        &mut self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        bounds: Rect,
    ) {
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

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
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
