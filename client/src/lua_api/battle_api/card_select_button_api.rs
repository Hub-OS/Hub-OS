use super::animation_api::create_animation_table;
use super::errors::{button_already_exists, button_not_found};
use super::{BattleLuaApi, ACTIVATE_FN, CARD_SELECT_BUTTON_TABLE, UNDO_FN};
use crate::battle::{BattleCallback, BattleSimulation, CardSelectButton, CardSelectButtonPath};
use crate::bindable::EntityId;
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::resources::{AssetManager, Globals};
use framework::prelude::GameIO;

pub fn inject_card_select_button_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "set_texture",
        move |api_ctx, lua, params| {
            let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
            let path = absolute_path(lua, path)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let globals = game_io.resource::<Globals>().unwrap();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;
            let texture = globals.assets.texture(game_io, &path);
            button.sprite.set_texture(texture);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "animation",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;
            let animation_table = create_animation_table(lua, button.animator_index)?;

            lua.pack_multi(animation_table)
        },
    );

    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "set_preview_texture",
        move |api_ctx, lua, params| {
            let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
            let path = absolute_path(lua, path)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let globals = game_io.resource::<Globals>().unwrap();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;
            let texture = globals.assets.texture(game_io, &path);
            button.preview_sprite.set_texture(texture);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "preview_animation",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;
            let animation_table = create_animation_table(lua, button.preview_animator_index)?;

            lua.pack_multi(animation_table)
        },
    );

    lua_api.add_dynamic_function(CARD_SELECT_BUTTON_TABLE, "owner", move |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        lua.pack_multi(table.raw_get::<_, rollback_mlua::Table>("#entity")?)
    });

    callback_setter(
        lua_api,
        ACTIVATE_FN,
        |button| &mut button.activate_callback,
        |lua, button_table, _| {
            let player_table = button_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((button_table, player_table))
        },
    );

    callback_setter(
        lua_api,
        UNDO_FN,
        |button| &mut button.undo_callback,
        |lua, button_table, _| {
            let player_table = button_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((button_table, player_table))
        },
    );
}

fn callback_setter<G, P, F, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: G,
    param_transformer: F,
) where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default,
    G: for<'lua> Fn(&mut CardSelectButton) -> &mut Option<BattleCallback<P, R>>
        + Send
        + Sync
        + 'static,
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
    lua_api.add_dynamic_setter(
        CARD_SELECT_BUTTON_TABLE,
        name,
        move |api_ctx, lua, params| {
            let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
                lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;

            let key = lua.create_registry_value(table)?;

            *callback_getter(button) = callback
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
        },
    );
}

fn button_mut_from_table<'a>(
    simulation: &'a mut BattleSimulation,
    table: &rollback_mlua::Table,
) -> Option<&'a mut CardSelectButton> {
    let entity_id = table.raw_get("#entity_id").ok()?;
    let form_index = table.raw_get("#form_index").ok()?;
    let augment_index = table.raw_get("#aug_index").ok()?;
    let uses_card_slots = table.raw_get("#card_slots").ok()?;

    let button_path = CardSelectButtonPath {
        entity_id,
        form_index,
        augment_index,
        uses_card_slots,
    };

    let entities = &mut simulation.entities;
    CardSelectButton::resolve_button_option_mut(entities, button_path)?
        .as_mut()
        .map(|button| &mut **button)
}

pub fn create_card_select_button_and_table<'lua>(
    game_io: &GameIO,
    simulation: &mut BattleSimulation,
    lua: &'lua rollback_mlua::Lua,
    entity_table: &rollback_mlua::Table<'lua>,
    button_path: CardSelectButtonPath,
    slot_width: usize,
) -> rollback_mlua::Result<rollback_mlua::Table<'lua>> {
    let entity_id: EntityId = entity_table.get("#id")?;

    // create the button
    let entities = &mut simulation.entities;
    let button_option = CardSelectButton::resolve_button_option_mut(entities, button_path)
        .ok_or_else(button_not_found)?;

    if button_option.is_some() {
        // delete the animators on failure
        return Err(button_already_exists());
    }

    let button = CardSelectButton::new(game_io, &mut simulation.animators, slot_width);
    *button_option = Some(Box::new(button));

    // create the table
    let table = lua.create_table()?;
    table.raw_set("#entity", entity_table.clone())?;
    table.raw_set("#entity_id", entity_id)?;

    if button_path.uses_card_slots {
        table.raw_set("#card_slots", true)?;
    }

    if let Some(index) = button_path.form_index {
        table.raw_set("#form_index", index)?;
    }

    if let Some(index) = button_path.augment_index {
        table.raw_set("#aug_index", index)?;
    }

    inherit_metatable(lua, CARD_SELECT_BUTTON_TABLE, &table)?;

    Ok(table)
}
