use crate::resources::ResourcePaths;

pub fn absolute_path(
    lua: &rollback_mlua::Lua,
    mut path_string: String,
) -> rollback_mlua::Result<String> {
    if path_string == ResourcePaths::BLANK {
        return Ok(path_string);
    }

    if ResourcePaths::is_absolute(&path_string) {
        return Ok(path_string);
    }

    let folder_path: String = lua.environment()?.get("_folder_path")?;

    if !path_string.starts_with(&folder_path) {
        path_string = folder_path + &path_string;
    }

    let absolute_path = ResourcePaths::clean(&path_string);

    Ok(absolute_path)
}

pub fn inherit_metatable(
    lua: &rollback_mlua::Lua,
    parent: &str,
    table: &rollback_mlua::Table,
) -> rollback_mlua::Result<()> {
    let parent_table: rollback_mlua::Table = lua.named_registry_value(parent)?;
    let metatable = parent_table
        .get_metatable()
        .ok_or_else(|| rollback_mlua::Error::RuntimeError(String::from("metatable deleted")))?;

    table.set_metatable(Some(metatable));

    Ok(())
}
