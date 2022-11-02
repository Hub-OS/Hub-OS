use super::{CardClass, Element, HitFlags};

#[derive(Clone)]
pub struct CardProperties {
    pub package_id: String,
    // action: String, // ????
    pub short_name: String,
    pub description: String,
    pub long_description: String,
    pub damage: i32,
    pub element: Element,
    pub secondary_element: Element,
    pub card_class: CardClass,
    pub limit: usize,
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
            package_id: String::new(),
            short_name: String::from("?????"),
            description: String::new(),
            long_description: String::new(),
            damage: 0,
            element: Element::None,
            secondary_element: Element::None,
            time_freeze: false,
            card_class: CardClass::Standard,
            hit_flags: 0,
            limit: 5,
            can_boost: true,
            counterable: true,
            skip_time_freeze_intro: false,
            meta_classes: Vec::new(),
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
            short_name: table.get("shortname").unwrap_or_default(),
            description: table.get("description").unwrap_or_default(),
            long_description: table.get("long_description").unwrap_or_default(),
            damage: table.get("damage").unwrap_or_default(),
            element: table.get("element").unwrap_or_default(),
            secondary_element: table.get("secondary_element").unwrap_or_default(),
            card_class: table.get("card_class").unwrap_or_default(),
            limit: table.get("limit").unwrap_or_default(),
            hit_flags: table.get("hit_flags").unwrap_or_default(),
            can_boost: table.get("can_boost").unwrap_or_default(),
            counterable: table.get("counterable").unwrap_or_default(),
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
        table.set("shortname", self.short_name.as_str())?;
        table.set("description", self.description.as_str())?;
        table.set("long_description", self.long_description.as_str())?;
        table.set("damage", self.damage)?;
        table.set("element", self.element)?;
        table.set("secondary_element", self.secondary_element)?;
        table.set("card_class", self.card_class)?;
        table.set("limit", self.limit)?;
        table.set("hit_flags", self.hit_flags)?;
        table.set("can_boost", self.can_boost)?;
        table.set("counterable", self.counterable)?;
        table.set("time_freeze", self.time_freeze)?;
        table.set("skip_time_freeze_intro", self.skip_time_freeze_intro)?;
        table.set("meta_classes", self.meta_classes.clone())?;

        Ok(rollback_mlua::Value::Table(table))
    }
}
