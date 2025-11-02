use super::{CardClass, Element, HitFlag, HitFlags};
use crate::battle::StatusRegistry;
use crate::bindable::SpriteColorMode;
use crate::packages::{CardPackage, CardPackageStatusDuration, PackageId, PackageNamespace};
use crate::render::ui::{ElementSprite, FontName, TextStyle};
use crate::render::{FrameTime, SpriteColorQueue, SpriteNode};
use crate::resources::BATTLE_INFO_SHADOW_COLOR;
use crate::structures::{Tree, VecMap};
use framework::prelude::{Color, GameIO, Vec2};
use std::borrow::Cow;

#[derive(Clone, PartialEq, Eq)]
pub struct CardProperties<L = HitFlags, D = VecMap<HitFlags, FrameTime>> {
    pub package_id: PackageId,
    pub namespace: Option<PackageNamespace>,
    pub code: String,
    pub short_name: Cow<'static, str>,
    pub damage: i32,
    pub boosted_damage: i32,
    pub recover: i32,
    pub element: Element,
    pub secondary_element: Element,
    pub card_class: CardClass,
    pub hit_flags: L,
    pub status_durations: D,
    pub can_boost: bool,
    pub can_charge: bool,
    pub time_freeze: bool,
    pub skip_time_freeze_intro: bool,
    pub prevent_time_freeze_counter: bool,
    pub conceal: bool,
    pub dynamic_damage: bool,
    pub tags: Vec<String>,
}

impl<L: Default, F: Default> Default for CardProperties<L, F> {
    fn default() -> Self {
        Self {
            package_id: PackageId::new_blank(),
            namespace: None,
            code: String::new(),
            short_name: Cow::Borrowed("?????"),
            damage: 0,
            boosted_damage: 0,
            recover: 0,
            element: Element::None,
            secondary_element: Element::None,
            time_freeze: false,
            card_class: CardClass::Standard,
            hit_flags: Default::default(),
            status_durations: Default::default(),
            can_boost: true,
            can_charge: true,
            skip_time_freeze_intro: false,
            prevent_time_freeze_counter: false,
            conceal: false,
            dynamic_damage: false,
            tags: Vec::new(),
        }
    }
}

impl<L, F> CardProperties<L, F> {
    pub fn draw_summary(
        &self,
        game_io: &GameIO,
        sprite_queue: &mut SpriteColorQueue,
        position: Vec2,
        scale: Vec2,
        center: bool,
    ) -> TextStyle {
        const NAME_SPACING: f32 = 2.0;
        const DAMAGE_SPACING: f32 = 1.0;

        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.monospace = true;
        text_style.bounds.set_position(position);

        // if we're scaling vertically, we want to do so from the center
        let original_line_height = text_style.line_height();
        text_style.bounds.y += (original_line_height - original_line_height * scale.y) * 0.5;

        // set the scale after we've adjusted the offset for vertical scaling
        text_style.scale = scale;

        let name_text = &self.short_name;
        let damage_text = if self.damage == 0 {
            String::new()
        } else if self.boosted_damage == 0 {
            format!("{}", self.damage)
        } else {
            // negative sign will be provided from negative boosted_damage
            let sign_str = if self.boosted_damage > 0 { "+" } else { "" };

            format!(
                "{}{}{}",
                self.damage - self.boosted_damage,
                sign_str,
                self.boosted_damage
            )
        };

        // measure text
        text_style.letter_spacing = NAME_SPACING;
        let name_width = text_style.measure(name_text).size.x;

        text_style.font = FontName::Damage;
        text_style.letter_spacing = DAMAGE_SPACING;
        let damage_width = text_style.measure(&damage_text).size.x;

        if center {
            let final_width = name_width + NAME_SPACING + damage_width;
            text_style.bounds.x -= final_width * 0.5;
        }

        // draw name
        text_style.letter_spacing = NAME_SPACING;
        text_style.shadow_color = BATTLE_INFO_SHADOW_COLOR;
        text_style.font = FontName::Thick;
        text_style.draw(game_io, sprite_queue, name_text);

        // draw damage
        text_style.letter_spacing = DAMAGE_SPACING;
        text_style.shadow_color = Color::TRANSPARENT;
        text_style.font = FontName::Damage;
        text_style.bounds.x += name_width + NAME_SPACING;
        text_style.draw(game_io, sprite_queue, &damage_text);

        text_style.bounds.x += damage_width + text_style.letter_spacing;

        text_style.letter_spacing = NAME_SPACING;
        text_style
    }

    pub fn create_preview_tree(&self, game_io: &GameIO) -> Tree<SpriteNode> {
        const PREVIEW_OFFSET: Vec2 = Vec2::new(0.0, 24.0);
        const ELEMENT_OFFSET: Vec2 = Vec2::new(-23.0, 49.0);
        const ELEMENT2_OFFSET: Vec2 = Vec2::new(-8.0, 49.0);
        const CODE_OFFSET: Vec2 = Vec2::new(-31.0, 51.0);
        const DAMAGE_OFFSET: Vec2 = Vec2::new(31.0, 52.0);

        // preview
        let (preview_texture, _) = CardPackage::preview_texture(game_io, &self.package_id);

        let mut root_node = SpriteNode::new(game_io, SpriteColorMode::Multiply);
        root_node.set_origin(-PREVIEW_OFFSET + preview_texture.size().as_vec2() * 0.5);
        root_node.set_texture_direct(preview_texture);

        let mut sprite_tree = Tree::<SpriteNode>::new(root_node);

        // secondary_element
        if self.secondary_element != Element::None {
            let mut element_node = ElementSprite::new_node(game_io, self.secondary_element);
            element_node.set_origin(-ELEMENT2_OFFSET);
            sprite_tree.insert_root_child(element_node);
        }

        // element
        let mut element_node = ElementSprite::new_node(game_io, self.element);
        element_node.set_origin(-ELEMENT_OFFSET);
        sprite_tree.insert_root_child(element_node);

        // code
        let mut text_style = TextStyle::new(game_io, FontName::Thick);
        text_style.letter_spacing = 2.0;

        text_style.color = Color::YELLOW;

        let code_index = sprite_tree.insert_root_text_child(game_io, &text_style, &self.code);
        let code_node = sprite_tree.get_mut(code_index).unwrap();
        code_node.set_offset(CODE_OFFSET);

        // damage
        if self.damage > 0 {
            text_style.color = Color::WHITE;
            let text = format!("{:>3}", self.damage);

            let damage_width = text_style.measure(&text).size.x;
            let damage_offset = DAMAGE_OFFSET + Vec2::new(-damage_width, 0.0);

            let damage_index = sprite_tree.insert_root_text_child(game_io, &text_style, &text);
            let damage_node = sprite_tree.get_mut(damage_index).unwrap();
            damage_node.set_offset(damage_offset);
        }

        sprite_tree
    }
}

impl CardProperties<Vec<String>, VecMap<String, CardPackageStatusDuration>> {
    pub fn to_bindable(&self, registry: &StatusRegistry) -> CardProperties<HitFlags> {
        CardProperties::<HitFlags> {
            package_id: self.package_id.clone(),
            namespace: self.namespace,
            code: self.code.clone(),
            short_name: self.short_name.clone(),
            damage: self.damage,
            boosted_damage: self.boosted_damage,
            recover: self.recover,
            element: self.element,
            secondary_element: self.secondary_element,
            card_class: self.card_class,
            hit_flags: self
                .hit_flags
                .iter()
                .map(|flag| HitFlag::from_str(registry, flag))
                .fold(0, |acc, flag| acc | flag),
            status_durations: VecMap::from_unique_vec(
                self.status_durations
                    .iter()
                    .map(|(name, duration)| {
                        let flag = HitFlag::from_str(registry, name);

                        let duration = match duration {
                            CardPackageStatusDuration::Level(level) => {
                                registry.duration_for(flag, *level)
                            }
                            CardPackageStatusDuration::Duration(frames) => *frames,
                        };

                        (flag, duration)
                    })
                    .collect(),
            ),
            can_boost: self.can_boost,
            can_charge: self.can_charge,
            time_freeze: self.time_freeze,
            skip_time_freeze_intro: self.skip_time_freeze_intro,
            prevent_time_freeze_counter: self.prevent_time_freeze_counter,
            conceal: self.conceal,
            dynamic_damage: self.dynamic_damage,
            tags: self.tags.clone(),
        }
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
            namespace: table.get("namespace").unwrap_or_default(),
            code: table.get("code").unwrap_or_default(),
            short_name: table
                .get::<_, String>("short_name")
                .map(Cow::Owned)
                .unwrap_or_else(|_| Cow::Borrowed("?????")),
            damage: table.get("damage").unwrap_or_default(),
            boosted_damage: table.get("boosted_damage").unwrap_or_default(),
            recover: table.get("recover").unwrap_or_default(),
            element: table.get("element").unwrap_or_default(),
            secondary_element: table.get("secondary_element").unwrap_or_default(),
            card_class: table.get("card_class").unwrap_or_default(),
            hit_flags: table.get("hit_flags").unwrap_or_default(),
            status_durations: table
                .get("status_durations")
                .map(VecMap::from_lua_table)
                .unwrap_or_default(),
            can_boost: table.get("can_boost").unwrap_or_default(),
            can_charge: table.get("can_charge").unwrap_or_default(),
            time_freeze: table.get("time_freeze").unwrap_or_default(),
            skip_time_freeze_intro: table.get("skip_time_freeze_intro").unwrap_or_default(),
            prevent_time_freeze_counter: table
                .get("prevent_time_freeze_counter")
                .unwrap_or_default(),
            conceal: table.get("conceal").unwrap_or_default(),
            dynamic_damage: table.get("dynamic_damage").unwrap_or_default(),
            tags: table.get("tags").unwrap_or_default(),
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for CardProperties {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        <&CardProperties>::into_lua(&self, lua)
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for &CardProperties {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;
        table.set("package_id", self.package_id.as_str())?;
        table.set("namespace", self.namespace)?;
        table.set("code", self.code.as_str())?;
        table.set("short_name", self.short_name.as_ref())?;
        table.set("damage", self.damage)?;
        table.set("boosted_damage", self.boosted_damage)?;
        table.set("recover", self.recover)?;
        table.set("element", self.element)?;
        table.set("secondary_element", self.secondary_element)?;
        table.set("card_class", self.card_class)?;
        table.set("hit_flags", self.hit_flags)?;
        table.set("status_durations", self.status_durations.to_lua_table(lua)?)?;

        if self.can_boost {
            table.set("can_boost", true)?;
        }

        if self.can_charge {
            table.set("can_charge", true)?;
        }

        if self.time_freeze {
            table.set("time_freeze", true)?;
        }

        if self.skip_time_freeze_intro {
            table.set("skip_time_freeze_intro", true)?;
        }

        if self.prevent_time_freeze_counter {
            table.set("prevent_time_freeze_counter", true)?;
        }

        if self.conceal {
            table.set("conceal", true)?;
        }

        if self.dynamic_damage {
            table.set("dynamic_damage", true)?;
        }

        table.set(
            "tags",
            lua.create_sequence_from(self.tags.iter().map(String::as_str))?,
        )?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
