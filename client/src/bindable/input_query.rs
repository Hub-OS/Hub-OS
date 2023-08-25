use crate::resources::Input;

#[derive(Clone, Copy)]
pub enum InputQuery {
    JustPressed(Input),
    Held(Input),
}

impl InputQuery {
    pub fn input(self) -> Input {
        match self {
            InputQuery::JustPressed(input) => input,
            InputQuery::Held(input) => input,
        }
    }
}

const PRESSED_FLAG: u8 = 1 << 7;

impl<'lua> rollback_mlua::FromLua<'lua> for InputQuery {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        use num_traits::FromPrimitive;

        let input_flags = match lua_value {
            rollback_mlua::Value::Integer(number) => number as u8,
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "Input",
                    message: None,
                })
            }
        };

        let is_pressed = input_flags & PRESSED_FLAG != 0;
        let input = Input::from_u8(input_flags & !PRESSED_FLAG).ok_or(
            rollback_mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "Input",
                message: None,
            },
        )?;

        if is_pressed {
            Ok(Self::JustPressed(input))
        } else {
            Ok(Self::Held(input))
        }
    }
}

impl<'lua> rollback_mlua::IntoLua<'lua> for InputQuery {
    fn into_lua(
        self,
        _lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<rollback_mlua::Value<'lua>> {
        let mut flags = self.input() as u8;

        if matches!(self, InputQuery::JustPressed(_)) {
            flags |= PRESSED_FLAG;
        };

        Ok(rollback_mlua::Value::Integer(flags as i64))
    }
}
