pub fn unexpected_nil(expected: &'static str) -> rollback_mlua::Error {
    rollback_mlua::Error::FromLuaConversionError {
        from: "Nil",
        to: expected,
        message: None,
    }
}
