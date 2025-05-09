use packets::structures::{ActorId, BattleId};

use super::lua_errors::{create_area_error, create_player_error};
use super::lua_helpers::*;
use super::LuaApi;
use crate::net::Direction;

#[allow(clippy::type_complexity)]
pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "list_players", |api_ctx, lua, params| {
        let area_id: mlua::String = lua.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let connected_players_iter = area
                .connected_players()
                .iter()
                .enumerate()
                .map(|(i, id)| (i + 1, *id));

            let result = lua.create_table_from(connected_players_iter);

            lua.pack_multi(result)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "is_player", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let player_exists = net.get_player(player_id).is_some();

        lua.pack_multi(player_exists)
    });

    lua_api.add_dynamic_function("Net", "get_player_area", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(player) = net.get_player(player_id) {
            lua.pack_multi(player.area_id.as_str())
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_ip", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(addr) = net.get_player_addr(player_id) {
            lua.pack_multi(addr.ip().to_string())
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_name", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow_mut();

        if let Some(player) = net.get_player(player_id) {
            lua.pack_multi(player.name.as_str())
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_name", |api_ctx, lua, params| {
        let (player_id, name): (ActorId, mlua::String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_name(player_id, name.to_str()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_direction", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player) = net.get_player(player_id) {
            let direction_str: &str = player.direction.into();

            lua.pack_multi(direction_str)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_position", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player) = net.get_player(player_id) {
            let table = lua.create_table()?;
            table.set("x", player.x)?;
            table.set("y", player.y)?;
            table.set("z", player.z)?;

            lua.pack_multi(table)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function(
        "Net",
        "get_player_position_multi",
        |api_ctx, lua, params| {
            let player_id: ActorId = lua.unpack_multi(params)?;

            let net = api_ctx.net_ref.borrow();

            if let Some(player) = net.get_player(player_id) {
                lua.pack_multi((player.x, player.y, player.z))
            } else {
                Err(create_player_error(player_id))
            }
        },
    );

    lua_api.add_dynamic_function("Net", "get_player_mugshot", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player) = net.get_player(player_id) {
            let table = lua.create_table()?;
            table.set("texture_path", player.mugshot_texture_path.as_str())?;
            table.set("animation_path", player.mugshot_animation_path.as_str())?;

            lua.pack_multi(table)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "get_player_avatar", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player) = net.get_player(player_id) {
            let table = lua.create_table()?;
            table.set("texture_path", player.texture_path.as_str())?;
            table.set("animation_path", player.animation_path.as_str())?;

            lua.pack_multi(table)
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_avatar", |api_ctx, lua, params| {
        let (player_id, texture_path, animation_path): (ActorId, mlua::String, mlua::String) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_avatar(player_id, texture_path.to_str()?, animation_path.to_str()?);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_player_avatar_name", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        if let Some(player_data) = net.get_player_data(player_id) {
            lua.pack_multi(player_data.avatar_name.as_str())
        } else {
            Err(create_player_error(player_id))
        }
    });

    lua_api.add_dynamic_function("Net", "set_player_emote", |api_ctx, lua, params| {
        let (player_id, emote_id): (ActorId, String) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_emote(player_id, emote_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "exclusive_player_emote", |api_ctx, lua, params| {
        let (target_id, emoter_id, emote_id): (ActorId, ActorId, String) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.exclusive_player_emote(target_id, emoter_id, emote_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_player_map_color", |api_ctx, lua, params| {
        let (player_id, color_table): (ActorId, mlua::Table) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let color = (
            color_table.get("r")?,
            color_table.get("g")?,
            color_table.get("b")?,
            color_table.get("a").unwrap_or(255),
        );

        net.set_player_map_color(player_id, color);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "animate_player", |api_ctx, lua, params| {
        let (player_id, name, loop_option): (ActorId, mlua::String, Option<bool>) =
            lua.unpack_multi(params)?;
        let name_str = name.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let loop_animation = loop_option.unwrap_or_default();

        net.animate_player(player_id, name_str, loop_animation);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        "Net",
        "animate_player_properties",
        |api_ctx, lua, params| {
            use super::actor_property_animation::parse_animation;

            let (player_id, keyframe_tables): (ActorId, Vec<mlua::Table>) =
                lua.unpack_multi(params)?;

            let mut net = api_ctx.net_ref.borrow_mut();

            let animation = parse_animation(keyframe_tables)?;
            net.animate_player_properties(player_id, animation);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function("Net", "is_player_busy", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let is_in_widget = net.is_player_in_widget(player_id);

        lua.pack_multi(is_in_widget)
    });

    lua_api.add_dynamic_function("Net", "play_sound_for_player", |api_ctx, lua, params| {
        let (player_id, asset_path): (ActorId, mlua::String) = lua.unpack_multi(params)?;
        let asset_path_str = asset_path.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.play_sound_for_player(player_id, asset_path_str);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        "Net",
        "exclude_object_for_player",
        |api_ctx, lua, params| {
            let (player_id, object_id): (ActorId, u32) = lua.unpack_multi(params)?;

            let mut net = api_ctx.net_ref.borrow_mut();

            net.exclude_object_for_player(player_id, object_id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        "Net",
        "include_object_for_player",
        |api_ctx, lua, params| {
            let (player_id, object_id): (ActorId, u32) = lua.unpack_multi(params)?;

            let mut net = api_ctx.net_ref.borrow_mut();

            net.include_object_for_player(player_id, object_id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function("Net", "exclude_actor_for_player", |api_ctx, lua, params| {
        let (player_id, actor_id): (ActorId, ActorId) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.exclude_actor_for_player(player_id, actor_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "include_actor_for_player", |api_ctx, lua, params| {
        let (player_id, actor_id): (ActorId, ActorId) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.include_actor_for_player(player_id, actor_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "move_player_camera", |api_ctx, lua, params| {
        let (player_id, x, y, z, duration): (ActorId, f32, f32, f32, Option<f32>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.move_player_camera(player_id, x, y, z, duration.unwrap_or_default());

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "slide_player_camera", |api_ctx, lua, params| {
        let (player_id, x, y, z, duration): (ActorId, f32, f32, f32, f32) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.slide_player_camera(player_id, x, y, z, duration);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "shake_player_camera", |api_ctx, lua, params| {
        let (player_id, strength, duration): (ActorId, f32, f32) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.shake_player_camera(player_id, strength, duration);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "fade_player_camera", |api_ctx, lua, params| {
        let (player_id, color, duration): (ActorId, mlua::Table, f32) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.fade_player_camera(
            player_id,
            (
                color.get("r")?,
                color.get("g")?,
                color.get("b")?,
                color.get("a").unwrap_or(255),
            ),
            duration,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "track_with_player_camera", |api_ctx, lua, params| {
        let (player_id, actor_id): (ActorId, ActorId) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.track_with_player_camera(player_id, actor_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "enable_camera_controls", |api_ctx, lua, params| {
        let (player_id, dist_x, dist_y): (ActorId, Option<f32>, Option<f32>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.enable_camera_controls(
            player_id,
            dist_x.unwrap_or(f32::MAX),
            dist_y.unwrap_or(f32::MAX),
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "is_player_input_locked", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let is_locked = net.is_player_input_locked(player_id);

        lua.pack_multi(is_locked)
    });

    lua_api.add_dynamic_function("Net", "unlock_player_camera", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.unlock_player_camera(player_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "lock_player_input", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.lock_player_input(player_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "unlock_player_input", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.unlock_player_input(player_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "teleport_player", |api_ctx, lua, params| {
        let (player_id, warp, x, y, z, direction_option): (
            ActorId,
            bool,
            f32,
            f32,
            f32,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let direction = Direction::from(optional_lua_string_to_str(&direction_option)?);

        net.teleport_player(player_id, warp, x, y, z, direction);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "set_player_restrictions", |api_ctx, lua, params| {
        let (player_id, restrictions_path): (ActorId, Option<mlua::String>) =
            lua.unpack_multi(params)?;

        let restrictions_path_str = restrictions_path
            .as_ref()
            .map(|path| path.to_str().unwrap_or_default());

        let mut net = api_ctx.net_ref.borrow_mut();

        net.set_player_restrictions(player_id, restrictions_path_str);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "_initiate_encounter", |api_ctx, lua, params| {
        let (player_id, package_id, data): (ActorId, mlua::String, Option<mlua::Value>) =
            lua.unpack_multi(params)?;

        let package_id_str = package_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(tracker) = api_ctx.battle_tracker_ref.borrow_mut().get_mut(&player_id) {
            tracker.push_back(api_ctx.script_index);
        }

        let data_string = data.map(|v| lua_value_to_string(v, "", 0));
        let battle_id = net.initiate_encounter(player_id, package_id_str, data_string);

        lua.pack_multi(battle_id)
    });

    lua_api.add_dynamic_function("Net", "_initiate_netplay", |api_ctx, lua, params| {
        let (player_ids, package_path, data): (Vec<ActorId>, Option<String>, Option<mlua::Value>) =
            lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        let mut battle_tracker = api_ctx.battle_tracker_ref.borrow_mut();

        for player_id in &player_ids {
            if let Some(tracker) = battle_tracker.get_mut(player_id) {
                tracker.push_back(api_ctx.script_index);
            }
        }

        let data_string = data.map(|v| lua_value_to_string(v, "", 0));

        let battle_id = net
            .initiate_netplay(&player_ids, package_path, data_string)
            .unwrap_or_default();

        lua.pack_multi(battle_id)
    });

    lua_api.add_dynamic_function("Net", "send_battle_message", |api_ctx, lua, params| {
        let (battle_id, data): (BattleId, mlua::Value) = lua.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();
        net.send_battle_message(battle_id, lua_value_to_string(data, "", 0));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "is_player_battling", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let is_battling = net.is_player_battling(player_id);

        lua.pack_multi(is_battling)
    });

    lua_api.add_dynamic_function("Net", "is_player_busy", |api_ctx, lua, params| {
        let player_id: ActorId = lua.unpack_multi(params)?;

        let net = api_ctx.net_ref.borrow();

        let is_busy = net.is_player_busy(player_id);

        lua.pack_multi(is_busy)
    });

    lua_api.add_dynamic_function("Net", "transfer_player", |api_ctx, lua, params| {
        let (player_id, area_id, warp_in_option, x_option, y_option, z_option, direction_option): (
            ActorId,
            mlua::String,
            Option<bool>,
            Option<f32>,
            Option<f32>,
            Option<f32>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();
        let warp_in = warp_in_option.unwrap_or(true);
        let x;
        let y;
        let z;

        if let Some(player) = net.get_player(player_id) {
            x = x_option.unwrap_or(player.x);
            y = y_option.unwrap_or(player.y);
            z = z_option.unwrap_or(player.z);
        } else {
            return Err(create_player_error(player_id));
        }

        let direction = Direction::from(optional_lua_string_to_str(&direction_option)?);

        net.transfer_player(player_id, area_id_str, warp_in, x, y, z, direction);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "transfer_server", |api_ctx, lua, params| {
        let (player_id, address, warp_out_option, data_option): (
            ActorId,
            mlua::String,
            Option<bool>,
            Option<mlua::String>,
        ) = lua.unpack_multi(params)?;
        let address_str = address.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let warp = warp_out_option.unwrap_or_default();
        let data = optional_lua_string_to_str(&data_option)?;

        net.transfer_server(player_id, address_str, data, warp);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "request_authorization", |api_ctx, lua, params| {
        let (player_id, address, data_option): (ActorId, mlua::String, Option<mlua::String>) =
            lua.unpack_multi(params)?;
        let address_str = address.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let data = data_option
            .as_ref()
            .map(|lua_str| lua_str.as_bytes())
            .unwrap_or(&[]);

        net.request_authorization(player_id, address_str, data);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "kick_player", |api_ctx, lua, params| {
        let (player_id, reason, warp_out_option): (ActorId, mlua::String, Option<bool>) =
            lua.unpack_multi(params)?;
        let reason_str = reason.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        let warp_out = warp_out_option.unwrap_or(true);

        net.kick_player(player_id, reason_str, warp_out);

        lua.pack_multi(())
    });
}
