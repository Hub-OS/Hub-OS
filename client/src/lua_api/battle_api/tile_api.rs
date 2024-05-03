use super::errors::{entity_not_found, invalid_tile};
use super::{create_entity_table, BattleLuaApi, TILE_CACHE_REGISTRY_KEY, TILE_TABLE};
use crate::battle::{
    AttackBox, BattleScriptContext, Character, Entity, Field, Obstacle, Player, Spell, Tile,
};
use crate::bindable::{Direction, EntityId, Team, TileHighlight};
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_tile_api(lua_api: &mut BattleLuaApi) {
    inject_tile_cache(lua_api);

    lua_api.add_static_injector(|lua| {
        let table: rollback_mlua::Table = lua.named_registry_value(TILE_TABLE)?;
        let metatable = table.get_metatable().unwrap();

        metatable.raw_set(
            "__eq",
            lua.create_function(|_, (a, b): (rollback_mlua::Table, rollback_mlua::Table)| {
                let cmp = a.raw_get("#x").unwrap_or(-1) == b.raw_get("#x").unwrap_or(-1)
                    && a.raw_get("#y").unwrap_or(-1) == b.raw_get("#y").unwrap_or(-1);

                Ok(cmp)
            })?,
        )?;

        Ok(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "x", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let x: i32 = table.raw_get("#x")?;
        lua.pack_multi(x)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "y", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let y: i32 = table.raw_get("#y")?;
        lua.pack_multi(y)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "width", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.simulation.field.tile_size().x)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "height", |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();
        lua.pack_multi(api_ctx.simulation.field.tile_size().y)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "state", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.state_index())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "set_state", |api_ctx, lua, params| {
        let (table, state_index): (rollback_mlua::Table, usize) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let simulation = &mut api_ctx.simulation;
        let resources = &mut api_ctx.resources;

        let Some(tile_state) = simulation.tile_states.get(state_index) else {
            return lua.pack_multi(());
        };

        let max_lifetime = tile_state.max_lifetime;

        let field = &mut simulation.field;
        let (x, y) = tile_position_from(table)?;
        let tile = field.tile_at_mut((x, y)).ok_or_else(invalid_tile)?;

        if tile_state.is_hole && !tile.reservations().is_empty() {
            // tile must be walkable for entities that are on or are moving to this tile
            return lua.pack_multi(());
        }

        let current_tile_state = simulation.tile_states.get(tile.state_index()).unwrap();
        let change_request_callback = current_tile_state.change_request_callback.clone();
        let change_passed =
            change_request_callback.call(game_io, resources, simulation, (x, y, state_index));

        if !change_passed {
            return lua.pack_multi(());
        }

        let field = &mut simulation.field;
        let tile = field.tile_at_mut((x, y)).ok_or_else(invalid_tile)?;

        tile.set_state_index(state_index, max_lifetime);

        // activate entity_enter_callback for every entity on this tile
        let tile_state = &simulation.tile_states[state_index];
        let tile_callback = tile_state.entity_enter_callback.clone();

        let entity_iter = simulation.entities.query_mut::<&Entity>().into_iter();
        let same_tile_entity_iter =
            entity_iter.filter(|(_, entity)| entity.x == x && entity.y == y);

        let entity_ids: Vec<EntityId> = same_tile_entity_iter.map(|(id, _)| id.into()).collect();

        for id in entity_ids {
            tile_callback.call(game_io, resources, simulation, id);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "is_edge", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let x: i32 = table.raw_get("#x")?;
        let y: i32 = table.raw_get("#y")?;

        let api_ctx = api_ctx.borrow();
        let field = &api_ctx.simulation.field;
        lua.pack_multi(field.is_edge((x, y)))
    });

    lua_api.add_dynamic_function(TILE_TABLE, "is_walkable", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        let tile_state = &api_ctx.simulation.tile_states[tile.state_index()];
        lua.pack_multi(!tile_state.is_hole)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "is_reserved", |api_ctx, lua, params| {
        let (table, exclude_list): (rollback_mlua::Table, Option<Vec<EntityId>>) =
            lua.unpack_multi(params)?;

        let exclude_list = exclude_list.unwrap_or_default();

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;

        let excluded_count = tile
            .reservations()
            .iter()
            .filter(|reservation| exclude_list.contains(reservation))
            .count();

        lua.pack_multi(tile.reservations().len() > excluded_count)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "reserve_count_for", |api_ctx, lua, params| {
        let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;

        lua.pack_multi(
            tile.reservations()
                .iter()
                .filter(|id| **id == entity_id)
                .count(),
        )
    });

    lua_api.add_dynamic_function(TILE_TABLE, "reserve_for_id", |api_ctx, lua, params| {
        let (table, id): (rollback_mlua::Table, EntityId) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        tile.reserve_for(id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "reserve_for", |api_ctx, lua, params| {
        let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        tile.reserve_for(entity_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        TILE_TABLE,
        "remove_reservation_for_id",
        |api_ctx, lua, params| {
            let (table, id): (rollback_mlua::Table, EntityId) = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
            tile.remove_reservation_for(id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        TILE_TABLE,
        "remove_reservation_for",
        |api_ctx, lua, params| {
            let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
                lua.unpack_multi(params)?;

            let entity_id: EntityId = entity_table.raw_get("#id")?;

            let mut api_ctx = api_ctx.borrow_mut();
            let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
            tile.remove_reservation_for(entity_id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(TILE_TABLE, "team", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.team())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "set_team", |api_ctx, lua, params| {
        let (table, team, direction): (rollback_mlua::Table, Team, Option<Direction>) =
            lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let tile = tile_mut_from_table(&mut simulation.field, table)?;
        let current_tile_state = simulation.tile_states.get(tile.state_index()).unwrap();

        if !current_tile_state.blocks_team_change || tile.team() == Team::Unset {
            tile.set_team(team, direction);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "facing", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.direction())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "set_facing", |api_ctx, lua, params| {
        let (table, direction): (rollback_mlua::Table, Direction) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        tile.set_direction(direction);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "set_highlight", |api_ctx, lua, params| {
        let (table, highlight): (rollback_mlua::Table, TileHighlight) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_mut_from_table(&mut api_ctx.simulation.field, table)?;
        tile.set_highlight(highlight);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "get_tile", |api_ctx, lua, params| {
        let (table, direction, count): (rollback_mlua::Table, Direction, i32) =
            lua.unpack_multi(params)?;

        let vec = direction.i32_vector();
        let x = table.get::<_, i32>("#x")? + vec.0 * count;
        let y = table.get::<_, i32>("#y")? + vec.1 * count;

        let api_ctx = api_ctx.borrow();

        if api_ctx.simulation.field.in_bounds((x, y)) {
            lua.pack_multi(create_tile_table(lua, (x, y))?)
        } else {
            lua.pack_multi(())
        }
    });

    lua_api.add_dynamic_function(TILE_TABLE, "attack_entities", |api_ctx, lua, params| {
        let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let x: i32 = table.raw_get("#x")?;
        let y: i32 = table.raw_get("#y")?;
        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let (entity, spell) = entities
            .query_one_mut::<(&Entity, &Spell)>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let queued_attacks = &mut api_ctx.simulation.queued_attacks;
        let is_same_attack =
            |attack: &AttackBox| attack.attacker_id == entity.id && attack.x == x && attack.y == y;

        if !queued_attacks.iter().any(is_same_attack) {
            let attack_box = AttackBox::new_from((x, y), entity, spell);
            queued_attacks.push(attack_box);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "contains_entity", |api_ctx, lua, params| {
        let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let x: i32 = table.raw_get("#x")?;
        let y: i32 = table.raw_get("#y")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let entity = entities
            .query_one_mut::<&Entity>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let contains_entity = entity.spawned && entity.on_field && entity.x == x && entity.y == y;

        lua.pack_multi(contains_entity)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "add_entity", |api_ctx, lua, params| {
        let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let x: i32 = table.raw_get("#x")?;
        let y: i32 = table.raw_get("#y")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let entity = entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        if !entity.spawned {
            return lua.pack_multi(());
        }

        let actions = &simulation.actions;

        let field = &mut simulation.field;
        let current_tile = field.tile_at_mut((entity.x, entity.y)).unwrap();
        current_tile.handle_auto_reservation_removal(actions, entity);

        let old_x = entity.x;
        let old_y = entity.y;
        let old_state = current_tile.state_index();
        let leave_callback = simulation.tile_states[old_state]
            .entity_leave_callback
            .clone();

        entity.on_field = true;
        entity.x = x;
        entity.y = y;

        let tile = field.tile_at_mut((x, y)).unwrap();
        tile.handle_auto_reservation_addition(actions, entity);

        let new_state = tile.state_index();
        let enter_callback = simulation.tile_states[new_state]
            .entity_enter_callback
            .clone();

        let game_io = api_ctx.game_io;
        let resources = api_ctx.resources;
        leave_callback.call(game_io, resources, simulation, (entity_id, old_x, old_y));
        enter_callback.call(game_io, resources, simulation, entity_id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "remove_entity", |api_ctx, lua, params| {
        let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = entity_table.raw_get("#id")?;
        let api_ctx = &mut *api_ctx.borrow_mut();
        remove_entity(api_ctx, table, entity_id)?;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "remove_entity_by_id", |api_ctx, lua, params| {
        let (table, entity_id): (rollback_mlua::Table, EntityId) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        remove_entity(api_ctx, table, entity_id)?;

        lua.pack_multi(())
    });

    generate_find_entity_fn::<()>(lua_api, "find_entities");
    generate_find_entity_fn::<&Character>(lua_api, "find_characters");
    generate_find_entity_fn::<&Obstacle>(lua_api, "find_obstacles");
    generate_find_entity_fn::<&Player>(lua_api, "find_players");
    generate_find_entity_fn::<hecs::Without<&Spell, &Obstacle>>(lua_api, "find_spells");
}

fn remove_entity(
    api_ctx: &mut BattleScriptContext,
    tile_table: rollback_mlua::Table,
    entity_id: EntityId,
) -> rollback_mlua::Result<()> {
    let x: i32 = tile_table.raw_get("#x")?;
    let y: i32 = tile_table.raw_get("#y")?;

    let entities = &mut api_ctx.simulation.entities;

    let entity = entities
        .query_one_mut::<&mut Entity>(entity_id.into())
        .map_err(|_| entity_not_found())?;

    if entity.spawned && entity.x == x && entity.y == y {
        entity.on_field = false;
    }

    Ok(())
}

fn generate_find_entity_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, name: &str) {
    lua_api.add_dynamic_function(TILE_TABLE, name, |api_ctx, lua, params| {
        let (tile_table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let x: i32 = tile_table.get("#x")?;
        let y: i32 = tile_table.get("#y")?;

        let tables = {
            // scope to prevent RefCell borrow escaping when control is passed back to lua
            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let mut tables: Vec<rollback_mlua::Table> = Vec::with_capacity(entities.len() as usize);

            for (id, entity) in entities.query_mut::<hecs::With<&Entity, Q>>() {
                if entity.x == x && entity.y == y && !entity.deleted && entity.on_field {
                    tables.push(create_entity_table(lua, id.into())?);
                }
            }

            tables
        };

        let filtered_tables: Vec<rollback_mlua::Table> = tables
            .into_iter()
            .filter(|table| {
                callback.call::<_, bool>(table.clone()).unwrap_or_else(|e| {
                    log::error!("{e}");
                    false
                })
            })
            .collect();

        lua.pack_multi(filtered_tables)
    });
}

fn inject_tile_cache(lua_api: &mut BattleLuaApi) {
    lua_api.add_static_injector(|lua| {
        lua.set_named_registry_value(TILE_CACHE_REGISTRY_KEY, lua.create_table()?)?;
        Ok(())
    });
}

pub fn create_tile_table(
    lua: &rollback_mlua::Lua,
    (x, y): (i32, i32),
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let tile_cache: rollback_mlua::Table = lua.named_registry_value(TILE_CACHE_REGISTRY_KEY)?;

    let key = ((x as i64) << 4) | y as i64;

    let table = if let Ok(table) = tile_cache.get(key) {
        table
    } else {
        let table = lua.create_table()?;
        table.raw_set("#x", x)?;
        table.raw_set("#y", y)?;

        inherit_metatable(lua, TILE_TABLE, &table)?;

        tile_cache.raw_set(key, table.clone())?;

        table
    };

    Ok(table)
}

pub fn tile_mut_from_table<'a>(
    field: &'a mut Field,
    table: rollback_mlua::Table,
) -> rollback_mlua::Result<&'a mut Tile> {
    let tile_position = tile_position_from(table)?;

    field.tile_at_mut(tile_position).ok_or_else(invalid_tile)
}

fn tile_position_from(table: rollback_mlua::Table) -> rollback_mlua::Result<(i32, i32)> {
    Ok((table.raw_get("#x")?, table.raw_get("#y")?))
}
