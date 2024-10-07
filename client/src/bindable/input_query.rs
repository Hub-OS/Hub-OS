use crate::resources::Input;

#[derive(Clone, Copy)]
pub enum InputQuery {
    JustPressed(Input),
    Pulsed(Input),
    Held(Input),
}

impl InputQuery {
    pub fn input(self) -> Input {
        match self {
            InputQuery::JustPressed(input) => input,
            InputQuery::Pulsed(input) => input,
            InputQuery::Held(input) => input,
        }
    }
}

const VARIANT_SHIFT: u8 = 8 - 2;
const INPUT_FLAGS: u8 = 0b111111;

impl<'lua> rollback_mlua::FromLua<'lua> for InputQuery {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        use num_traits::FromPrimitive;

        let query_flags = match lua_value {
            rollback_mlua::Value::Integer(number) => number as u8,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "Input",
                    message: None,
                })
            }
        };

        let input = Input::from_u8(query_flags & INPUT_FLAGS).ok_or(
            rollback_mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "Input",
                message: None,
            },
        )?;

        let query = match query_flags >> VARIANT_SHIFT {
            0 => Self::JustPressed(input),
            1 => Self::Pulsed(input),
            _ => Self::Held(input),
        };

        Ok(query)
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for InputQuery {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let mut query_flags = self.input() as u8;

        let variant_flags = match self {
            InputQuery::JustPressed(_) => 0,
            InputQuery::Pulsed(_) => 1,
            InputQuery::Held(_) => 2,
        };

        query_flags |= variant_flags << VARIANT_SHIFT;

        Ok(rollback_mlua::Value::Integer(query_flags as i64))
    }
}
