use super::errors::{augment_not_found, entity_not_found};
use super::{
    AUGMENT_TABLE, BattleLuaApi, CARD_CHARGE_TIMING_FN, CHARGE_TIMING_FN, CHARGED_ATTACK_FN,
    CHARGED_CARD_FN, DELETE_FN, MOVEMENT_FN, NORMAL_ATTACK_FN, SPECIAL_ATTACK_FN,
    create_card_select_button_and_table, create_entity_table,
};
use crate::battle::{Augment, BattleCallback, CardSelectButton, CardSelectButtonPath, Player};
use crate::bindable::{EntityId, GenerationalIndex};
use crate::lua_api::battle_api::MOVEMENT_INPUT_FN;
use crate::lua_api::helpers::inherit_metatable;

pub fn inject_augment_api(lua_api: &mut BattleLuaApi) {
    getter(lua_api, "id", |augment, _, _: ()| {
        Ok(augment.package_id.clone())
    });

    getter(lua_api, "level", |augment, _, _: ()| Ok(augment.level));

    lua_api.add_dynamic_function(AUGMENT_TABLE, "owner", move |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        lua.pack_multi(create_entity_table(lua, id)?)
    });

    getter(lua_api, "has_tag", |augment, _, tag: String| {
        Ok(augment.tags.iter().any(|t| *t == tag))
    });

    lua_api.add_dynamic_function(AUGMENT_TABLE, "deleted", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;
        let entity_id: EntityId = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let Ok(player) = entities.query_one_mut::<&mut Player>(entity_id.into()) else {
            return lua.pack_multi(true);
        };

        let exists = player.augments.contains_key(index);

        lua.pack_multi(!exists)
    });

    lua_api.add_dynamic_function(
        AUGMENT_TABLE,
        "create_card_button",
        |api_ctx, lua, params| {
            let (table, slot_width, button_slot): (rollback_mlua::Table, usize, Option<usize>) =
                lua.unpack_multi(params)?;
            let entity_id: EntityId = table.raw_get("#id")?;
            let entity_table = create_entity_table(lua, entity_id)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;

            let button_path = CardSelectButtonPath {
                entity_id,
                form_index: None,
                augment_index: Some(table.raw_get("#index")?),
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
        AUGMENT_TABLE,
        "create_special_button",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;
            let entity_id: EntityId = table.raw_get("#id")?;
            let entity_table = create_entity_table(lua, entity_id)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;

            let button_path = CardSelectButtonPath {
                entity_id,
                form_index: None,
                augment_index: Some(table.raw_get("#index")?),
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

    setter(
        lua_api,
        "set_charge_with_shoot",
        |augment, _, charge: Option<bool>| {
            augment.overridables.flags.set_charges_with_shoot(charge);
            Ok(())
        },
    );

    setter(
        lua_api,
        "set_special_on_input",
        |augment, _, value: Option<bool>| {
            augment.overridables.flags.set_special_on_input(value);
            Ok(())
        },
    );

    setter(
        lua_api,
        "set_movement_on_input",
        |augment, _, value: Option<bool>| {
            augment.overridables.flags.set_movement_on_input(value);
            Ok(())
        },
    );

    callback_setter(
        lua_api,
        CHARGE_TIMING_FN,
        |augment| &mut augment.overridables.calculate_charge_time,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        NORMAL_ATTACK_FN,
        |augment: &mut Augment| &mut augment.overridables.normal_attack,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        CHARGED_ATTACK_FN,
        |augment: &mut Augment| &mut augment.overridables.charged_attack,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        SPECIAL_ATTACK_FN,
        |augment: &mut Augment| &mut augment.overridables.special_attack,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        CARD_CHARGE_TIMING_FN,
        |augment: &mut Augment| &mut augment.overridables.calculate_card_charge_time,
        |lua, table, card_props| lua.pack_multi((table, card_props)),
    );

    callback_setter(
        lua_api,
        CHARGED_CARD_FN,
        |augment: &mut Augment| &mut augment.overridables.charged_card,
        |lua, table, card_props| lua.pack_multi((table, card_props)),
    );

    callback_setter(
        lua_api,
        MOVEMENT_INPUT_FN,
        |augment: &mut Augment| &mut augment.overridables.movement_input,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        MOVEMENT_FN,
        |augment: &mut Augment| &mut augment.overridables.movement,
        |lua, table, direction| lua.pack_multi((table, direction)),
    );

    callback_setter(
        lua_api,
        DELETE_FN,
        |augment: &mut Augment| &mut augment.delete_callback,
        |lua, table, _| lua.pack_multi(table),
    );
}

pub fn create_augment_table(
    lua: &rollback_mlua::Lua,
    entity_id: EntityId,
    index: GenerationalIndex,
) -> rollback_mlua::Result<rollback_mlua::Table<'_>> {
    let table = lua.create_table()?;
    table.raw_set("#id", entity_id)?;
    table.raw_set("#index", index)?;

    inherit_metatable(lua, AUGMENT_TABLE, &table)?;

    Ok(table)
}

fn getter<P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: for<'lua> fn(&Augment, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<R>,
) where
    R: for<'lua> rollback_mlua::IntoLua<'lua> + 'static,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
{
    lua_api.add_dynamic_function(AUGMENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let augment = player.augments.get(index).ok_or_else(augment_not_found)?;

        lua.pack_multi(callback(augment, lua, param)?)
    });
}

fn setter<P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: for<'lua> fn(&mut Augment, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<R>,
) where
    R: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
{
    lua_api.add_dynamic_function(AUGMENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let augment = player
            .augments
            .get_mut(index)
            .ok_or_else(augment_not_found)?;

        lua.pack_multi(callback(augment, lua, param)?)
    });
}

fn callback_setter<P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: for<'lua> fn(&mut Augment) -> &mut Option<BattleCallback<P, R>>,
    param_transformer: for<'lua> fn(
        &'lua rollback_mlua::Lua,
        rollback_mlua::Table<'lua>,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    lua_api.add_dynamic_setter(AUGMENT_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let index: GenerationalIndex = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let Ok(player) = entities.query_one_mut::<&mut Player>(id.into()) else {
            return lua.pack_multi(());
        };

        let augment = player
            .augments
            .get_mut(index)
            .ok_or_else(augment_not_found)?;

        let key = lua.create_registry_value(table)?;

        *callback_getter(augment) = callback
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
