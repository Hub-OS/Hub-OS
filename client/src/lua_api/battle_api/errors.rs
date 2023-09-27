use packets::structures::PackageId;

pub fn package_not_loaded(package_id: &PackageId) -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(format!("{package_id:?} is not loaded"))
}

pub fn entity_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("entity deleted or incompatible"))
}

pub fn form_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("form invalid"))
}

pub fn too_many_forms() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("too many forms"))
}

pub fn augment_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("augment deleted"))
}

pub fn sprite_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("sprite deleted"))
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

pub fn action_aready_queued() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("action already queued"))
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

pub fn mismatched_entity() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("mismatched entity"))
}

pub fn aux_prop_already_bound() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("auxprop already bound"))
}
