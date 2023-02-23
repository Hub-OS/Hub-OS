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

pub fn card_action_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("card action deleted"))
}

pub fn card_action_step_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("card action step deleted"))
}

pub fn action_aready_used() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("action already used"))
}

pub fn attachment_not_found() -> rollback_mlua::Error {
    rollback_mlua::Error::RuntimeError(String::from("card action attachment deleted"))
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
