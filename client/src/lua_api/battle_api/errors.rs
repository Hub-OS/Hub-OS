use crate::lua_api::PACKAGE_ID_REGISTRY_KEY;
use packets::structures::PackageId;

pub fn package_not_loaded(package_id: &PackageId) -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(format!("{package_id:?} is not loaded"))
}

pub fn encounter_method_called_after_start() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from(
        "this method can only be called on the first frame",
    ))
}

pub fn entity_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("entity deleted or incompatible"))
}

pub fn form_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("form invalid"))
}

pub fn too_many_forms() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("too many visible forms"))
}

pub fn augment_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("augment deleted"))
}

pub fn button_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("button invalid"))
}

pub fn sprite_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("sprite deleted"))
}

pub fn invalid_font_name(font_name: &str) -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(format!("no font named {font_name:?}"))
}

pub fn invalid_sync_node() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("invalid sync node"))
}

pub fn component_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("component deleted"))
}

pub fn action_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("action deleted"))
}

pub fn action_step_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("action step deleted"))
}

pub fn action_aready_processed() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("action already processed"))
}

pub fn action_entity_mismatch() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("action is tied to another entity"))
}

pub fn attachment_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("action attachment deleted"))
}

pub fn animator_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("animator deleted"))
}

pub fn invalid_tile() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("invalid tile"))
}

pub fn invalid_custom_tile_state() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("invalid custom tile state"))
}

pub fn mismatched_entity() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("mismatched entity"))
}

pub fn aux_prop_already_bound() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("auxprop already bound"))
}

pub fn server_communication_closed() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("server communication closed"))
}

pub fn invalid_memory_value() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("entity can only remember numbers and strings"))
}

pub fn unmarked_dependency() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("package is not in dependency tree"))
}

pub fn warn_deprecated(lua: &rollback_mlua::Lua, name: &str) {
    log::warn!("deprecated use of {name} in {:?}", get_source_name(lua));
}

pub fn get_source_name(lua: &rollback_mlua::Lua) -> String {
    let source_name = lua
        .inspect_stack(1)
        .map(|debug| debug.source().source.unwrap_or_default().to_string())
        .unwrap_or_default();

    let package_id: String = lua
        .named_registry_value(PACKAGE_ID_REGISTRY_KEY)
        .unwrap_or_default();

    format!("{source_name} - {package_id}")
}
