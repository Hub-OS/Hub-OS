use packets::structures::ActorId;

pub fn create_area_error(id: &str) -> mlua::Error {
    mlua::Error::RuntimeError(format!("No area matching {id:?} found."))
}

pub fn create_bot_error(id: ActorId) -> mlua::Error {
    mlua::Error::RuntimeError(format!("No bot matching {id:?} found."))
}

pub fn create_player_error(id: ActorId) -> mlua::Error {
    mlua::Error::RuntimeError(format!("No player matching {id:?} found."))
}
