use super::{CardClass, Element, HitFlags};
use crate::packages::PackageId;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::SpriteColorQueue;
use framework::prelude::{Color, GameIO, Vec2};

#[derive(Clone)]
pub struct CardProperties {
    pub package_id: PackageId,
    // action: String, // ????
    pub short_name: String,
    pub damage: i32,
    pub element: Element,
    pub secondary_element: Element,
    pub card_class: CardClass,
    pub hit_flags: HitFlags,
    pub can_boost: bool,
    pub counterable: bool,
    pub time_freeze: bool,
    pub skip_time_freeze_intro: bool,
    pub meta_classes: Vec<String>,
}

impl Default for CardProperties {
    fn default() -> Self {
        Self {
            package_id: PackageId::new_blank(),
            short_name: String::from("?????"),
            damage: 0,
            element: Element::None,
            secondary_element: Element::None,
            time_freeze: false,
            card_class: CardClass::Standard,
            hit_flags: 0,
            can_boost: true,
            counterable: true,
            skip_time_freeze_intro: false,
            meta_classes: Vec::new(),
        }
    }
}

impl CardProperties {
    pub fn draw_summary(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
        center: bool,
    ) {
        let mut text_style = TextStyle::new(game_io, FontStyle::Thick);
        text_style.monospace = true;
        text_style.bounds.set_position(position);

        let name_text = &self.short_name;
        let damage_text = if self.damage == 0 {
            String::new()
        } else {
            format!("{}", self.damage)
        };

        // measure text
        let name_width = text_style.measure(name_text).size.x;

        if center {
            text_style.font_style = FontStyle::GradientOrange;
            let damage_width = text_style.measure(&damage_text).size.x;
            let text_width = name_width + text_style.letter_spacing + damage_width;

            text_style.bounds.x -= text_width * 0.5;
        }

        // draw name
        text_style.shadow_color = Color::BLACK;
        text_style.font_style = FontStyle::Thick;
        text_style.draw(game_io, sprite_queue, name_text);

        // draw damage
        text_style.shadow_color = Color::TRANSPARENT;
        text_style.font_style = FontStyle::GradientOrange;
        text_style.bounds.x += name_width + text_style.letter_spacing;
        text_style.draw(game_io, sprite_queue, &damage_text);
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for CardProperties {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let table = match lua_value {
            rollback_mlua::Value::Table(table) => table,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "CardProperties",
                    message: None,
                })
            }
        };

        Ok(CardProperties {
            package_id: table.get("package_id").unwrap_or_default(),
            short_name: table
                .get("short_name")
                .unwrap_or_else(|_| String::from("?????")),
            damage: table.get("damage").unwrap_or_default(),
            element: table.get("element").unwrap_or_default(),
            secondary_element: table.get("secondary_element").unwrap_or_default(),
            card_class: table.get("card_class").unwrap_or_default(),
            hit_flags: table.get("hit_flags").unwrap_or_default(),
            can_boost: table.get("can_boost").unwrap_or(true),
            counterable: table.get("counterable").unwrap_or(true),
            time_freeze: table.get("time_freeze").unwrap_or_default(),
            skip_time_freeze_intro: table.get("skip_time_freeze_intro").unwrap_or_default(),
            meta_classes: table.get("meta_classes").unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::ToLua<'lua> for &CardProperties {
    fn to_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        table.set("short_name", self.short_name.as_str())?;
        table.set("damage", self.damage)?;
        table.set("element", self.element)?;
        table.set("secondary_element", self.secondary_element)?;
        table.set("card_class", self.card_class)?;
        table.set("hit_flags", self.hit_flags)?;
        table.set("can_boost", self.can_boost)?;
        table.set("counterable", self.counterable)?;
        table.set("time_freeze", self.time_freeze)?;
        table.set("skip_time_freeze_intro", self.skip_time_freeze_intro)?;
        table.set("meta_classes", self.meta_classes.clone())?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
