use super::errors::{entity_not_found, invalid_tile};
use super::{create_entity_table, BattleLuaApi, TILE_TABLE};
use crate::battle::{
    AttackBox, BattleScriptContext, Character, Entity, Field, Living, Obstacle, Spell, Tile,
    TileState,
};
use crate::bindable::{Direction, EntityId, Team, TileHighlight};
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_tile_api(lua_api: &mut BattleLuaApi) {
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

    lua_api.add_dynamic_function(TILE_TABLE, "get_state", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.state_index())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "set_state", |api_ctx, lua, params| {
        let (table, state_index): (rollback_mlua::Table, usize) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let simulation = &mut api_ctx.simulation;
        let vms = api_ctx.vms;

        let Some(tile_state) = simulation.tile_states.get(state_index) else {
            return lua.pack_multi(());
        };

        let max_lifetime = tile_state.max_lifetime;

        let field = &mut simulation.field;
        let tile_position = tile_position_from(table)?;
        let tile = field.tile_at_mut(tile_position).ok_or_else(invalid_tile)?;

        if tile_state.is_hole && !tile.reservations().is_empty() {
            // tile must be walkable for entities that are on or are moving to this tile
            return lua.pack_multi(());
        }

        let change_request_callback = tile_state.change_request_callback.clone();
        let change_passed = change_request_callback.call(game_io, simulation, vms, state_index);

        if !change_passed {
            return lua.pack_multi(());
        }

        let field = &mut simulation.field;
        let tile = field.tile_at_mut(tile_position).ok_or_else(invalid_tile)?;

        tile.set_state_index(state_index, max_lifetime);

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

    lua_api.add_dynamic_function(TILE_TABLE, "is_cracked", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.state_index() == TileState::CRACKED)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "is_hole", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        let tile_state = &api_ctx.simulation.tile_states[tile.state_index()];
        lua.pack_multi(tile_state.is_hole)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "is_walkable", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        let tile_state = &api_ctx.simulation.tile_states[tile.state_index()];
        lua.pack_multi(!tile_state.is_hole)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "is_hidden", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.state_index() == TileState::HIDDEN)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "is_reserved", |api_ctx, lua, params| {
        let (table, exclude_list): (rollback_mlua::Table, Vec<EntityId>) =
            lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;

        let excluded_count = tile
            .reservations()
            .iter()
            .filter(|reservation| exclude_list.contains(reservation))
            .count();

        lua.pack_multi(tile.reservations().len() > excluded_count)
    });

    // todo: rename to reserve_by_entity_id?
    lua_api.add_dynamic_function(
        TILE_TABLE,
        "reserve_entity_by_id",
        |api_ctx, lua, params| {
            let (table, id): (rollback_mlua::Table, EntityId) = lua.unpack_multi(params)?;

            let mut api_ctx = api_ctx.borrow_mut();
            let tile = tile_from(&mut api_ctx.simulation.field, table)?;
            tile.reserve_for(id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(TILE_TABLE, "get_team", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.team())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "set_team", |api_ctx, lua, params| {
        let (table, team): (rollback_mlua::Table, Team) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;

        tile.set_team(team);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "get_facing", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        lua.pack_multi(tile.direction())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "set_facing", |api_ctx, lua, params| {
        let (table, direction): (rollback_mlua::Table, Direction) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
        tile.set_direction(direction);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(TILE_TABLE, "highlight", |api_ctx, lua, params| {
        let (table, highlight): (rollback_mlua::Table, TileHighlight) = lua.unpack_multi(params)?;

        let mut api_ctx = api_ctx.borrow_mut();
        let tile = tile_from(&mut api_ctx.simulation.field, table)?;
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
        let field = &mut api_ctx.simulation.field;
        let entities = &mut api_ctx.simulation.entities;

        let tile = field.tile_at_mut((x, y)).ok_or_else(invalid_tile)?;
        let entity_id: EntityId = entity_table.raw_get("#id")?;

        if tile.ignoring_attacker(entity_id) {
            return lua.pack_multi(());
        }

        let (entity, spell) = entities
            .query_one_mut::<(&Entity, &Spell)>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let attack_box = AttackBox::new_from((x, y), entity, spell);
        api_ctx.simulation.queued_attacks.push(attack_box);

        lua.pack_multi(())
    });

    generate_find_hittable_fn::<()>(lua_api, "find_entities");
    generate_find_hittable_fn::<&Character>(lua_api, "find_characters");
    generate_find_hittable_fn::<&Obstacle>(lua_api, "find_obstacles");
    generate_find_entity_fn::<hecs::Without<&Spell, &Obstacle>>(lua_api, "find_spells");

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

        let contains_entity = entity.spawned && entity.x == x && entity.y == y;

        lua.pack_multi(contains_entity)
    });

    lua_api.add_dynamic_function(TILE_TABLE, "add_entity", |api_ctx, lua, params| {
        let (table, entity_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let x: i32 = table.raw_get("#x")?;
        let y: i32 = table.raw_get("#y")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let entity = entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        if !entity.spawned {
            return lua.pack_multi(());
        }

        let card_actions = &api_ctx.simulation.card_actions;

        let field = &mut api_ctx.simulation.field;
        let current_tile = field.tile_at_mut((entity.x, entity.y)).unwrap();
        current_tile.unignore_attacker(entity.id);
        current_tile.handle_auto_reservation_removal(card_actions, entity);

        entity.on_field = true;
        entity.x = x;
        entity.y = y;

        let tile = field.tile_at_mut((x, y)).unwrap();
        tile.handle_auto_reservation_addition(card_actions, entity);

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

    if !entity.spawned {
        return Ok(());
    }

    let contains_entity = entity.spawned && entity.x == x && entity.y == y;

    if contains_entity {
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

fn generate_find_hittable_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, name: &str) {
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

            for (id, (entity, living)) in entities.query_mut::<hecs::With<(&Entity, &Living), Q>>()
            {
                if entity.x == x
                    && entity.y == y
                    && !entity.deleted
                    && entity.on_field
                    && living.hitbox_enabled
                {
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

pub fn create_tile_table(
    lua: &rollback_mlua::Lua,
    (x, y): (i32, i32),
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    table.raw_set("#x", x)?;
    table.raw_set("#y", y)?;

    inherit_metatable(lua, TILE_TABLE, &table)?;

    Ok(table)
}

fn tile_from<'a>(
    field: &'a mut Field,
    table: rollback_mlua::Table,
) -> rollback_mlua::Result<&'a mut Tile> {
    let tile_position = tile_position_from(table)?;

    field.tile_at_mut(tile_position).ok_or_else(invalid_tile)
}

fn tile_position_from(table: rollback_mlua::Table) -> rollback_mlua::Result<(i32, i32)> {
    Ok((table.raw_get("#x")?, table.raw_get("#y")?))
}
