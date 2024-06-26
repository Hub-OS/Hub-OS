#[derive(Default)]
pub enum AudioBehavior {
    #[default]
    Default,
    NoOverlap,
    LoopSection(usize, usize),
    EndLoop,
}

impl<'lua> rollback_mlua::FromLua<'lua> for AudioBehavior {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let Some(table) = lua_value.as_table() else {
            return Err(rollback_mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "AudioBehavior",
                message: None,
            });
        };

        let id: usize = table.get("id")?;

        Ok(match id {
            1 => AudioBehavior::NoOverlap,
            2 => AudioBehavior::LoopSection(table.get("start")?, table.get("end")?),
            3 => AudioBehavior::EndLoop,
            _ => AudioBehavior::Default,
        })
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for AudioBehavior {
    fn into_lua(
        self,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let table = lua.create_table()?;

        match self {
            AudioBehavior::Default => {
                table.set("id", 0)?;
            }
            AudioBehavior::NoOverlap => {
                table.set("id", 1)?;
            }
            AudioBehavior::LoopSection(start, end) => {
                table.set("id", 2)?;
                table.set("start", start)?;
                table.set("end", end)?;
            }
            AudioBehavior::EndLoop => {
                table.set("id", 3)?;
            }
        };

        table.into_lua(lua)
    }
}
