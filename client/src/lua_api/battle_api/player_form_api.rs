use super::errors::{entity_not_found, form_not_found};
use super::{
    BattleLuaApi, CHARGED_ATTACK_FN, CHARGE_TIMING_FN, PLAYER_FORM_TABLE, SPECIAL_ATTACK_FN,
    UPDATE_FN,
};
use crate::battle::{BattleCallback, Player, PlayerForm};
use crate::bindable::EntityId;
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::resources::{AssetManager, Globals};

pub fn inject_player_form_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(
        PLAYER_FORM_TABLE,
        "set_mugshot_texture_path",
        move |api_ctx, lua, params| {
            let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
            let path = absolute_path(lua, path)?;

            let entity_id: EntityId = table.raw_get("#entity_id")?;
            let index: usize = table.raw_get("#index")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;
            let player = entities
                .query_one_mut::<&mut Player>(entity_id.into())
                .map_err(|_| entity_not_found())?;

            let form = player.forms.get_mut(index).ok_or_else(form_not_found)?;

            let game_io = &api_ctx.game_io;
            let assets = &game_io.resource::<Globals>().unwrap().assets;
            form.mug_texture = Some(assets.texture(game_io, &path));

            lua.pack_multi(())
        },
    );

    callback_setter(
        lua_api,
        "on_activate_func",
        |form| &mut form.activate_callback,
        |lua, form_table, _| {
            let player_table = form_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((form_table, player_table))
        },
    );

    callback_setter(
        lua_api,
        "on_deactivate_func",
        |form| &mut form.deactivate_callback,
        |lua, form_table, _| {
            let player_table = form_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((form_table, player_table))
        },
    );

    callback_setter(
        lua_api,
        CHARGE_TIMING_FN,
        |form| &mut form.calculate_charge_time_callback,
        |lua, form_table, _| {
            let player_table = form_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((form_table, player_table))
        },
    );

    callback_setter(
        lua_api,
        UPDATE_FN,
        |form| &mut form.update_callback,
        |lua, form_table, _| {
            let player_table = form_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((form_table, player_table))
        },
    );

    callback_setter(
        lua_api,
        CHARGED_ATTACK_FN,
        |form| &mut form.charged_attack_callback,
        |lua, form_table, _| {
            let player_table = form_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((form_table, player_table))
        },
    );

    callback_setter(
        lua_api,
        SPECIAL_ATTACK_FN,
        |form| &mut form.special_attack_callback,
        |lua, form_table, _| {
            let player_table = form_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((form_table, player_table))
        },
    );
}

fn callback_setter<G, P, F, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: G,
    param_transformer: F,
) where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default,
    G: for<'lua> Fn(&mut PlayerForm) -> &mut Option<BattleCallback<P, R>> + Send + Sync + 'static,
    F: for<'lua> Fn(
            &'lua rollback_mlua::Lua,
            rollback_mlua::Table<'lua>,
            P,
        ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>
        + Send
        + Sync
        + Copy
        + 'static,
{
    lua_api.add_dynamic_setter(PLAYER_FORM_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = table.raw_get("#entity_id")?;
        let index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let player = entities
            .query_one_mut::<&mut Player>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let form = player.forms.get_mut(index).ok_or_else(form_not_found)?;

        let key = lua.create_registry_value(table)?;

        *callback_getter(form) = callback
            .map(|callback| {
                BattleCallback::new_transformed_lua_callback(
                    lua,
                    api_ctx.vm_index,
                    callback,
                    move |_, lua, p| {
                        let table: rollback_mlua::Table = lua.registry_value(&key)?;
                        param_transformer(lua, table, p)
                    },
                )
            })
            .transpose()?;

        lua.pack_multi(())
    });
}

pub fn create_player_form_table<'lua>(
    lua: &'lua rollback_mlua::Lua,
    entity_table: rollback_mlua::Table,
    index: usize,
) -> rollback_mlua::Result<rollback_mlua::Table<'lua>> {
    let entity_id: EntityId = entity_table.get("#id")?;

    let table = lua.create_table()?;
    table.raw_set("#entity", entity_table)?;
    table.raw_set("#entity_id", entity_id)?;
    table.raw_set("#index", index)?;
    inherit_metatable(lua, PLAYER_FORM_TABLE, &table)?;

    Ok(table)
}
