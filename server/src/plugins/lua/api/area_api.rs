use super::lua_errors::create_area_error;
use super::LuaApi;
use crate::net::map::{Map, Tile};
use crate::net::Direction;

#[allow(clippy::type_complexity)]
pub fn inject_dynamic(lua_api: &mut LuaApi) {
    lua_api.add_dynamic_function("Net", "list_areas", |api_ctx, lua_ctx, _| {
        let net = api_ctx.net_ref.borrow();

        let area_ids: mlua::Result<Vec<mlua::String>> = net
            .get_areas()
            .map(|area| lua_ctx.create_string(area.get_id()))
            .collect();

        lua_ctx.pack_multi(area_ids?)
    });

    lua_api.add_dynamic_function("Net", "update_area", |api_ctx, lua_ctx, params| {
        let (area_id, data): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;
        let (area_id_str, data_str) = (area_id.to_str()?, data.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();
        let map = Map::from(data_str);

        if let Some(area) = net.get_area_mut(area_id_str) {
            area.set_map(map);
        } else {
            net.add_area(area_id_str.to_string(), map);
        }

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "clone_area", |api_ctx, lua_ctx, params| {
        let (area_id, new_id): (mlua::String, String) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area(area_id_str) {
            let map = area.get_map().clone();

            net.add_area(new_id, map);

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "map_to_string", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let map = area.get_map_mut();
            lua_ctx.pack_multi(map.render())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "remove_area", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.remove_area(area_id_str);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "get_width", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            lua_ctx.pack_multi(area.get_map().get_width())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_height", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            lua_ctx.pack_multi(area.get_map().get_height())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_layer_count", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            lua_ctx.pack_multi(area.get_map().get_layer_count())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_tile_width", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            lua_ctx.pack_multi(area.get_map_mut().get_tile_width())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_tile_height", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            lua_ctx.pack_multi(area.get_map_mut().get_tile_height())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function(
        "Net",
        "get_area_custom_properties",
        |api_ctx, lua_ctx, params| {
            let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
            let area_id_str = area_id.to_str()?;

            let net = api_ctx.net_ref.borrow();

            if let Some(area) = net.get_area(area_id_str) {
                lua_ctx.pack_multi(area.get_map().get_custom_properties().clone())
            } else {
                Err(create_area_error(area_id_str))
            }
        },
    );

    lua_api.add_dynamic_function(
        "Net",
        "get_area_custom_property",
        |api_ctx, lua_ctx, params| {
            let (area_id, name): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;
            let (area_id_str, name_str) = (area_id.to_str()?, name.to_str()?);

            let net = api_ctx.net_ref.borrow();

            if let Some(area) = net.get_area(area_id_str) {
                let map = area.get_map();
                let value = map.get_custom_property(name_str).cloned();

                lua_ctx.pack_multi(value)
            } else {
                Err(create_area_error(area_id_str))
            }
        },
    );

    lua_api.add_dynamic_function(
        "Net",
        "set_area_custom_property",
        |api_ctx, lua_ctx, params| {
            let (area_id, name, value): (mlua::String, mlua::String, String) =
                lua_ctx.unpack_multi(params)?;
            let (area_id_str, name_str) = (area_id.to_str()?, name.to_str()?);

            let mut net = api_ctx.net_ref.borrow_mut();

            if let Some(area) = net.get_area_mut(area_id_str) {
                area.get_map_mut().set_custom_property(name_str, value);

                lua_ctx.pack_multi(())
            } else {
                Err(create_area_error(area_id_str))
            }
        },
    );

    lua_api.add_dynamic_function("Net", "get_area_name", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            lua_ctx.pack_multi(area.get_map().get_name().as_str())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_area_name", |api_ctx, lua_ctx, params| {
        let (area_id, name): (mlua::String, String) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let map = area.get_map_mut();

            map.set_name(name);

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_song", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            lua_ctx.pack_multi(area.get_map().get_song_path().as_str())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_song", |api_ctx, lua_ctx, params| {
        let (area_id, path): (mlua::String, String) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let map = area.get_map_mut();

            map.set_song_path(path);

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_background", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let map = area.get_map();

            let table = lua_ctx.create_table()?;

            table.set("texture_path", map.get_background_texture_path().as_str())?;

            table.set(
                "animation_path",
                map.get_background_animation_path().as_str(),
            )?;

            lua_ctx.pack_multi(table)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function(
        "Net",
        "get_background_velocity",
        |api_ctx, lua_ctx, params| {
            let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
            let area_id_str = area_id.to_str()?;

            let net = api_ctx.net_ref.borrow();

            if let Some(area) = net.get_area(area_id_str) {
                let map = area.get_map();

                let (vel_x, vel_y) = map.get_background_velocity();

                let table = lua_ctx.create_table()?;
                table.set("x", vel_x)?;
                table.set("y", vel_y)?;

                lua_ctx.pack_multi(table)
            } else {
                Err(create_area_error(area_id_str))
            }
        },
    );

    lua_api.add_dynamic_function("Net", "set_background", |api_ctx, lua_ctx, params| {
        let (area_id, texture_path, animation_path, vel_x, vel_y, parallax): (
            mlua::String,
            String,
            Option<String>,
            Option<f32>,
            Option<f32>,
            Option<f32>,
        ) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let map = area.get_map_mut();

            map.set_background_texture_path(texture_path);
            map.set_background_animation_path(animation_path.unwrap_or_default());
            map.set_background_velocity(vel_x.unwrap_or_default(), vel_y.unwrap_or_default());
            map.set_background_parallax(parallax.unwrap_or_default());

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function(
        "Net",
        "get_background_parallax",
        |api_ctx, lua_ctx, params| {
            let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
            let area_id_str = area_id.to_str()?;

            let net = api_ctx.net_ref.borrow();

            if let Some(area) = net.get_area(area_id_str) {
                let map = area.get_map();

                let parallax = map.get_background_parallax();

                lua_ctx.pack_multi(parallax)
            } else {
                Err(create_area_error(area_id_str))
            }
        },
    );

    lua_api.add_dynamic_function("Net", "get_foreground", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let map = area.get_map();

            let table = lua_ctx.create_table()?;

            table.set("texture_path", map.get_foreground_texture_path().as_str())?;

            table.set(
                "animation_path",
                map.get_foreground_animation_path().as_str(),
            )?;

            lua_ctx.pack_multi(table)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function(
        "Net",
        "get_foreground_velocity",
        |api_ctx, lua_ctx, params| {
            let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
            let area_id_str = area_id.to_str()?;

            let net = api_ctx.net_ref.borrow();

            if let Some(area) = net.get_area(area_id_str) {
                let map = area.get_map();

                let (vel_x, vel_y) = map.get_foreground_velocity();

                let table = lua_ctx.create_table()?;
                table.set("x", vel_x)?;
                table.set("y", vel_y)?;

                lua_ctx.pack_multi(table)
            } else {
                Err(create_area_error(area_id_str))
            }
        },
    );

    lua_api.add_dynamic_function(
        "Net",
        "get_foreground_parallax",
        |api_ctx, lua_ctx, params| {
            let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
            let area_id_str = area_id.to_str()?;

            let net = api_ctx.net_ref.borrow();

            if let Some(area) = net.get_area(area_id_str) {
                let map = area.get_map();

                let parallax = map.get_foreground_parallax();

                lua_ctx.pack_multi(parallax)
            } else {
                Err(create_area_error(area_id_str))
            }
        },
    );

    lua_api.add_dynamic_function("Net", "set_foreground", |api_ctx, lua_ctx, params| {
        let (area_id, texture_path, animation_path, vel_x, vel_y, parallax): (
            mlua::String,
            String,
            Option<String>,
            Option<f32>,
            Option<f32>,
            Option<f32>,
        ) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let map = area.get_map_mut();

            map.set_foreground_texture_path(texture_path);
            map.set_foreground_animation_path(animation_path.unwrap_or_default());
            map.set_foreground_velocity(vel_x.unwrap_or_default(), vel_y.unwrap_or_default());
            map.set_foreground_parallax(parallax.unwrap_or_default());

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_spawn_position", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let (x, y, z) = area.get_map().get_spawn();

            let table = lua_ctx.create_table()?;
            table.set("x", x)?;
            table.set("y", y)?;
            table.set("z", z)?;

            lua_ctx.pack_multi(table)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_spawn_position", |api_ctx, lua_ctx, params| {
        let (area_id, x, y, z): (mlua::String, f32, f32, f32) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let map = area.get_map_mut();

            map.set_spawn(x, y, z);

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_spawn_direction", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let direction = area.get_map().get_spawn_direction();

            let direction_str: &str = direction.into();
            lua_ctx.pack_multi(direction_str)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_spawn_direction", |api_ctx, lua_ctx, params| {
        let (area_id, direction_string): (mlua::String, mlua::String) =
            lua_ctx.unpack_multi(params)?;
        let (area_id_str, direction_str) = (area_id.to_str()?, direction_string.to_str()?);

        let mut net = api_ctx.net_ref.borrow_mut();

        if let Some(area) = net.get_area_mut(area_id_str) {
            let map = area.get_map_mut();

            let direction = Direction::from(direction_str);
            map.set_spawn_direction(direction);

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "list_tilesets", |api_ctx, lua_ctx, params| {
        let area_id: mlua::String = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let map = area.get_map();
            let tilesets = map.get_tilesets();
            let tileset_paths: mlua::Result<Vec<mlua::String>> = tilesets
                .iter()
                .map(|tileset| lua_ctx.create_string(&tileset.path))
                .collect();

            lua_ctx.pack_multi(tileset_paths?)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_tileset", |api_ctx, lua_ctx, params| {
        let (area_id, path): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;
        let (area_id_str, path_str) = (area_id.to_str()?, path.to_str()?);

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let map = area.get_map();
            let tilesets = map.get_tilesets();
            let optional_tileset = tilesets.iter().find(|tileset| tileset.path == path_str);

            if let Some(tileset) = optional_tileset {
                let table = lua_ctx.create_table()?;
                table.set("path", tileset.path.as_str())?;
                table.set("first_gid", tileset.first_gid)?;

                return lua_ctx.pack_multi(Some(table));
            }

            lua_ctx.pack_multi(mlua::Nil)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_tileset_for_tile", |api_ctx, lua_ctx, params| {
        let (area_id, tile_gid): (mlua::String, u32) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let map = area.get_map();
            let tilesets = map.get_tilesets();
            let optional_tileset = tilesets
                .iter()
                .take_while(|tileset| tileset.first_gid <= tile_gid)
                .last();

            if let Some(tileset) = optional_tileset {
                let table = lua_ctx.create_table()?;
                table.set("path", tileset.path.as_str())?;
                table.set("first_gid", tileset.first_gid)?;

                return lua_ctx.pack_multi(Some(table));
            }

            lua_ctx.pack_multi(mlua::Nil)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "get_tile", |api_ctx, lua_ctx, params| {
        let (area_id, x, y, z): (mlua::String, i32, i32, i32) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let net = api_ctx.net_ref.borrow();

        if let Some(area) = net.get_area(area_id_str) {
            let tile = if x < 0 || y < 0 || z < 0 {
                Tile::default()
            } else {
                area.get_map().get_tile(x as usize, y as usize, z as usize)
            };

            let table = lua_ctx.create_table()?;

            table.set("gid", tile.gid)?;

            if tile.flipped_anti_diagonally {
                table.set("flippedHorizontal", tile.flipped_vertically)?;
                table.set("flippedVertical", !tile.flipped_horizontally)?;
            } else {
                table.set("flippedHorizontal", tile.flipped_horizontally)?;
                table.set("flippedVertical", tile.flipped_vertically)?;
            }

            table.set("rotated", tile.flipped_anti_diagonally)?;

            lua_ctx.pack_multi(table)
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "set_tile", |api_ctx, lua_ctx, params| {
        let (area_id, x, y, z, gid, flip_horizontal, flip_vertical, rotate): (
            mlua::String,
            i32,
            i32,
            i32,
            u32,
            Option<bool>,
            Option<bool>,
            Option<bool>,
        ) = lua_ctx.unpack_multi(params)?;
        let area_id_str = area_id.to_str()?;

        let mut net = api_ctx.net_ref.borrow_mut();

        if x < 0 || y < 0 || z < 0 {
            return lua_ctx.pack_multi(());
        }

        if let Some(area) = net.get_area_mut(area_id_str) {
            let tile = Tile {
                gid,
                flipped_horizontally: flip_horizontal.unwrap_or(false),
                flipped_vertically: flip_vertical.unwrap_or(false),
                flipped_anti_diagonally: rotate.unwrap_or(false),
            };

            area.get_map_mut()
                .set_tile(x as usize, y as usize, z as usize, tile);

            lua_ctx.pack_multi(())
        } else {
            Err(create_area_error(area_id_str))
        }
    });

    lua_api.add_dynamic_function("Net", "provide_asset", |api_ctx, lua_ctx, params| {
        let (area_id, asset_path): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.require_asset(area_id.to_str()?, asset_path.to_str()?);

        lua_ctx.pack_multi(())
    });

    lua_api.add_dynamic_function("Net", "play_sound", |api_ctx, lua_ctx, params| {
        let (area_id, asset_path): (mlua::String, mlua::String) = lua_ctx.unpack_multi(params)?;

        let mut net = api_ctx.net_ref.borrow_mut();

        net.play_sound(area_id.to_str()?, asset_path.to_str()?);

        lua_ctx.pack_multi(())
    });
}
