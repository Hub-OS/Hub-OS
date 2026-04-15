use super::LuaApi;
use super::lua_errors::create_actor_error;
use crate::plugins::lua::api::colors::*;
use crate::plugins::lua::api::lua_helpers::optional_lua_string_to_str;
use packets::structures::{ActorId, Direction};

pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "is_actor", |api_ctx, lua, params| {
        let actor_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow_mut();

        lua.pack_multi(net.get_actor(actor_id).is_some())
    });

    lua_api.add_dynamic_function("Net", "get_actor_area", |api_ctx, lua, params| {
        let actor_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(actor) = net.get_actor(actor_id) {
            lua.pack_multi(actor.area_id.as_str())
        } else {
            Err(create_actor_error(actor_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_actor_name", |api_ctx, lua, params| {
        let actor_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(actor) = net.get_actor(actor_id) {
            lua.pack_multi(actor.name.as_str())
        } else {
            Err(create_actor_error(actor_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_actor_name", |api_ctx, lua, params| {
        let (actor_id, name): (ActorId, mlua::String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_actor_name(actor_id, name.to_str()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_actor_direction", |api_ctx, lua, params| {
        let actor_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(actor) = net.get_actor(actor_id) {
            let direction_str: &str = actor.direction.into();

            lua.pack_multi(direction_str)
        } else {
            Err(create_actor_error(actor_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_actor_direction", |api_ctx, lua, params| {
        let (actor_id, direction_string): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_actor_direction(actor_id, Direction::from(&direction_string));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_actor_position", |api_ctx, lua, params| {
        let actor_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(actor) = net.get_actor(actor_id) {
            let table = lua.create_table()?;
            table.set("x", actor.x)?;
            table.set("y", actor.y)?;
            table.set("z", actor.z)?;

            lua.pack_multi(table)
        } else {
            Err(create_actor_error(actor_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_actor_position_multi", |api_ctx, lua, params| {
        let actor_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(actor) = net.get_actor(actor_id) {
            lua.pack_multi((actor.x, actor.y, actor.z))
        } else {
            Err(create_actor_error(actor_id))
        }
    });

    lua_api.add_dynamic_function("Net", "animate_actor", |api_ctx, lua, params| {
        let (actor_id, name, loop_option): (ActorId, mlua::String, Option<bool>) =
            lua.unpack_multi(params)?;
        let name_str = name.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let loop_animation = loop_option.unwrap_or_default();

        net.animate_actor(actor_id, name_str, loop_animation);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "animate_actor_properties", |api_ctx, lua, params| {
        use super::actor_property_animation::parse_animation;

        let (actor_id, keyframe_tables): (ActorId, Vec<mlua::Table>) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let animation = parse_animation(keyframe_tables)?;
        net.animate_actor_properties(actor_id, animation);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_actor_avatar", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(actor) = net.get_actor(player_id) {
            let table = lua.create_table()?;
            table.set("texture_path", actor.texture_path.as_str())?;
            table.set("animation_path", actor.animation_path.as_str())?;

            lua.pack_multi(table)
        } else {
            Err(create_actor_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_actor_avatar", |api_ctx, lua, params| {
        let (actor_id, texture_path, animation_path): (ActorId, mlua::String, mlua::String) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_actor_avatar(actor_id, texture_path.to_str()?, animation_path.to_str()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_actor_emote", |api_ctx, lua, params| {
        let (actor_id, emote_id): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_actor_emote(actor_id, emote_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_actor_map_color", |api_ctx, lua, params| {
        let (actor_id, color_table): (ActorId, mlua::Table) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let color = parse_rgba_table(color_table)?;

        net.set_actor_map_color(actor_id, color);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "move_actor", |api_ctx, lua, params| {
        let (actor_id, x, y, z, direction_option): (ActorId, f32, f32, f32, Option<mlua::String>) =
            lua.unpack_multi(params)?;
        let direction = Direction::from(optional_lua_string_to_str(&direction_option)?);

        let mut net = api_ctx.net_ref.borrow_mut();

        net.move_actor(actor_id, false, x, y, z, direction);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "warp_actor", |api_ctx, lua, params| {
        let (actor_id, x, y, z, direction_option): (ActorId, f32, f32, f32, Option<mlua::String>) =
            lua.unpack_multi(params)?;
        let direction = Direction::from(optional_lua_string_to_str(&direction_option)?);

        let mut net = api_ctx.net_ref.borrow_mut();

        net.move_actor(actor_id, true, x, y, z, direction);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "transfer_actor", |api_ctx, lua, params| {
        #[allow(clippy::type_complexity)]
        let (actor_id, area_id, warp_in_option, x_option, y_option, z_option, direction_option): (
            ActorId,
            String,
            Option<bool>,
            Option<f32>,
            Option<f32>,
            Option<f32>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        let warp_in = warp_in_option.unwrap_or(true);
        let x;
        let y;
        let z;

        if let Some(actor) = net.get_actor(actor_id) {
            x = x_option.unwrap_or(actor.x);
            y = y_option.unwrap_or(actor.y);
            z = z_option.unwrap_or(actor.z);
        } else {
            return Err(create_actor_error(actor_id));
        }

        let direction = Direction::from(optional_lua_string_to_str(&direction_option)?);

        net.transfer_actor(actor_id, &area_id, warp_in, x, y, z, direction);

        lua.pack_multi(())
    });
}
