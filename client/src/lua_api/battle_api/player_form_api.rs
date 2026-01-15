use super::errors::{entity_not_found, form_not_found};
use super::{
    ACTIVATE_FN, BattleLuaApi, CARD_CHARGE_TIMING_FN, CHARGE_TIMING_FN, CHARGED_ATTACK_FN,
    CHARGED_CARD_FN, DEACTIVATE_FN, DESELECT_FN, MOVEMENT_FN, NORMAL_ATTACK_FN, PLAYER_FORM_TABLE,
    SELECT_FN, SPECIAL_ATTACK_FN, UPDATE_FN, create_card_select_button_and_table,
};
use crate::battle::{
    BattleCallback, CardSelectButton, CardSelectButtonPath, Player, PlayerForm,
    PlayerOverridableFlags,
};
use crate::bindable::EntityId;
use crate::lua_api::battle_api::MOVEMENT_INPUT_FN;
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::resources::{AssetManager, Globals};
use framework::common::GameIO;

pub fn inject_player_form_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(PLAYER_FORM_TABLE, "index", move |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;
        let index: usize = table.raw_get("#index")?;

        lua.pack_multi(index)
    });

    setter(
        lua_api,
        "set_mugshot_texture",
        |game_io, lua, form, path: String| {
            let path = absolute_path(lua, path)?;

            let assets = &Globals::from_resources(game_io).assets;
            form.mug_texture = Some(assets.texture(game_io, &path));

            Ok(())
        },
    );

    setter(
        lua_api,
        "set_description",
        |_, _, form, description: Option<String>| {
            form.description = description.map(|s| s.into());

            Ok(())
        },
    );

    setter(lua_api, "set_close_on_select", |_, _, form, value: bool| {
        form.close_on_select = value;
        Ok(())
    });

    setter(
        lua_api,
        "set_transition_on_select",
        |_, _, form, value: bool| {
            form.transition_on_select = value;
            Ok(())
        },
    );

    lua_api.add_dynamic_function(
        PLAYER_FORM_TABLE,
        "create_card_button",
        |api_ctx, lua, params| {
            let (table, slot_width, button_slot): (rollback_mlua::Table, usize, Option<usize>) =
                lua.unpack_multi(params)?;
            let entity_table: rollback_mlua::Table = table.raw_get("#entity")?;
            let entity_id: EntityId = entity_table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;

            let button_path = CardSelectButtonPath {
                entity_id,
                form_index: Some(table.raw_get("#index")?),
                augment_index: None,
                card_button_slot: button_slot.unwrap_or_default().max(1),
            };

            let table = create_card_select_button_and_table(
                game_io,
                simulation,
                lua,
                &entity_table,
                button_path,
                slot_width,
            )?;

            lua.pack_multi(table)
        },
    );

    lua_api.add_dynamic_function(
        PLAYER_FORM_TABLE,
        "create_special_button",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;
            let entity_table: rollback_mlua::Table = table.raw_get("#entity")?;
            let entity_id: EntityId = entity_table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;

            let button_path = CardSelectButtonPath {
                entity_id,
                form_index: Some(table.raw_get("#index")?),
                augment_index: None,
                card_button_slot: CardSelectButton::SPECIAL_SLOT,
            };

            let table = create_card_select_button_and_table(
                game_io,
                simulation,
                lua,
                &entity_table,
                button_path,
                1,
            )?;

            lua.pack_multi(table)
        },
    );

    flag_setter(lua_api, "set_charge_with_shoot", |flags, value| {
        flags.set_special_on_input(value);
    });

    flag_setter(lua_api, "set_special_on_input", |flags, value| {
        flags.set_special_on_input(value);
    });

    flag_setter(lua_api, "set_movement_on_input", |flags, value| {
        flags.set_movement_on_input(value);
    });

    lua_api.add_dynamic_function(
        PLAYER_FORM_TABLE,
        "deactivate",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let entity_id: EntityId = table.raw_get("#entity_id")?;
            let index: usize = table.raw_get("#index")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;
            let player = entities
                .query_one_mut::<&mut Player>(entity_id.into())
                .map_err(|_| entity_not_found())?;

            let _form = player.forms.get_mut(index).ok_or_else(form_not_found)?;

            PlayerForm::deactivate(api_ctx.simulation, entity_id, index);

            lua.pack_multi(())
        },
    );

    callback_setter(
        lua_api,
        ACTIVATE_FN,
        |form| &mut form.activate_callback,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        DEACTIVATE_FN,
        |form| &mut form.deactivate_callback,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        SELECT_FN,
        |form| &mut form.select_callback,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        DESELECT_FN,
        |form| &mut form.deselect_callback,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        CHARGE_TIMING_FN,
        |form| &mut form.overridables.calculate_charge_time,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        UPDATE_FN,
        |form| &mut form.update_callback,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        NORMAL_ATTACK_FN,
        |form| &mut form.overridables.normal_attack,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        CHARGED_ATTACK_FN,
        |form| &mut form.overridables.charged_attack,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        SPECIAL_ATTACK_FN,
        |form| &mut form.overridables.special_attack,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        CARD_CHARGE_TIMING_FN,
        |form| &mut form.overridables.calculate_card_charge_time,
        |lua, form_table, card_props| lua.pack_multi((form_table, card_props)),
    );

    callback_setter(
        lua_api,
        CHARGED_CARD_FN,
        |form| &mut form.overridables.charged_card,
        |lua, form_table, card_props| lua.pack_multi((form_table, card_props)),
    );

    callback_setter(
        lua_api,
        MOVEMENT_INPUT_FN,
        |form| &mut form.overridables.movement_input,
        |lua, form_table, _| lua.pack_multi(form_table),
    );

    callback_setter(
        lua_api,
        MOVEMENT_FN,
        |form| &mut form.overridables.movement,
        |lua, form_table, direction| lua.pack_multi((form_table, direction)),
    );
}

fn setter<P>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    setter: fn(&GameIO, &rollback_mlua::Lua, &mut PlayerForm, P) -> rollback_mlua::Result<()>,
) where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
{
    lua_api.add_dynamic_function(PLAYER_FORM_TABLE, name, move |api_ctx, lua, params| {
        let (table, setter_params): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let entity_id: EntityId = table.raw_get("#entity_id")?;
        let index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let player = entities
            .query_one_mut::<&mut Player>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let form = player.forms.get_mut(index).ok_or_else(form_not_found)?;

        let game_io = &api_ctx.game_io;
        setter(game_io, lua, form, setter_params)?;
        lua.pack_multi(())
    });
}

fn callback_setter<P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: fn(&mut PlayerForm) -> &mut Option<BattleCallback<P, R>>,
    param_transformer: for<'lua> fn(
        &'lua rollback_mlua::Lua,
        rollback_mlua::Table<'lua>,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + 'static,
{
    lua_api.add_dynamic_setter(PLAYER_FORM_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = table.raw_get("#entity_id")?;
        let index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let Ok(player) = entities.query_one_mut::<&mut Player>(entity_id.into()) else {
            return lua.pack_multi(());
        };

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

fn flag_setter(
    lua_api: &mut BattleLuaApi,
    name: &str,
    setter: fn(&mut PlayerOverridableFlags, Option<bool>),
) {
    lua_api.add_dynamic_function(PLAYER_FORM_TABLE, name, move |api_ctx, lua, params| {
        let (table, value): (rollback_mlua::Table, Option<bool>) = lua.unpack_multi(params)?;

        let entity_id: EntityId = table.raw_get("#entity_id")?;
        let index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let player = entities
            .query_one_mut::<&mut Player>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let form = player.forms.get_mut(index).ok_or_else(form_not_found)?;
        setter(&mut form.overridables.flags, value);

        lua.pack_multi(table)
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
