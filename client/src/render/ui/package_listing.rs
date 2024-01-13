use super::{ElementSprite, FontName, TextStyle, UiNode};
use crate::render::ui::PackagePreviewData;
use crate::render::{Animator, SpriteColorQueue};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::*;
use packets::structures::{FileHash, PackageCategory, PackageId};
use serde_json as json;

#[derive(Clone)]
pub struct PackageListing {
    pub id: PackageId,
    pub name: String,
    pub description: String,
    pub creator: String,
    pub hash: FileHash,
    pub preview_data: PackagePreviewData,
    pub dependencies: Vec<(PackageCategory, PackageId)>,
}

impl From<&json::Value> for PackageListing {
    fn from(value: &json::Value) -> Self {
        let Some(package_table) = value.get("package") else {
            return Self {
                id: PackageId::new_blank(),
                name: String::from("Broken package"),
                description: String::from("???"),
                creator: String::new(),
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
                codes: get_string_vec(package_table, "codes"),
                element: get_str(package_table, "element").into(),
                secondary_element: get_str(package_table, "secondary_element").into(),
                damage: get_i32(package_table, "damage"),
            },
            "augment" => PackagePreviewData::Augment {
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
                    let bools: Vec<_> = map_array_values(package_table, "shape", |value| {
                        value
                            .as_array()
                            .map(|v| v.iter().map(|v| v.as_i64().unwrap_or_default() != 0))
                    })
                    .flatten()
                    .flatten()
                    .collect();

                    bools.try_into().unwrap_or_default()
                },
            },
            "encounter" => PackagePreviewData::Encounter,
            "library" => PackagePreviewData::Library,
            "pack" => PackagePreviewData::Pack,
            "resource" => PackagePreviewData::Resource,
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
            dependencies.extend(map_array_values(dependencies_table, "battles", |id| {
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

        Self {
            id: get_str(package_table, "id").into(),
            name: get_str(package_table, "name").to_string(),
            description: description.to_string(),
            creator: get_unknown_as_string(value, "creator"),
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
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.bounds = bounds + Vec2::ONE;
        text_style.draw(game_io, sprite_queue, &self.name);

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
                    let damage_space = text_style.measure("0000").size;
                    let damage_text = format!("{damage}");
                    text_style.monospace = true;

                    text_style.bounds.x =
                        position.x - text_style.measure(&damage_text).size.x - 2.0;
                    text_style.bounds.set_size(damage_space);
                    text_style.draw(game_io, sprite_queue, &damage_text);
                }
            }
            PackagePreviewData::Player { element, .. } => {
                let mut sprite = ElementSprite::new(game_io, *element);
                let mut position = bounds.top_right();
                position.x -= sprite.size().x;

                sprite.set_position(position);
                sprite_queue.draw_sprite(&sprite);
            }
            PackagePreviewData::Augment { colors, flat, .. } => {
                let assets = &game_io.resource::<Globals>().unwrap().assets;
                let mut sprite = assets.new_sprite(game_io, ResourcePaths::BLOCKS_UI);
                sprite.set_scale(Vec2::new(2.0, 2.0));

                let mut animator =
                    Animator::load_new(assets, ResourcePaths::BLOCKS_PREVIEW_ANIMATION);

                let mut position = bounds.center_right();
                position.x -= 3.0;

                for color in colors.iter().rev() {
                    animator.set_state(if *flat {
                        color.flat_state()
                    } else {
                        color.plus_state()
                    });

                    animator.apply(&mut sprite);

                    sprite.set_position(position - sprite.size() * 0.5);
                    sprite_queue.draw_sprite(&sprite);

                    position.x -= sprite.size().x;
                    position.x -= 1.0;
                }
            }
            _ => {}
        }
    }

    fn measure_ui_size(&mut self, _: &GameIO) -> Vec2 {
        Vec2::ZERO
    }
}
