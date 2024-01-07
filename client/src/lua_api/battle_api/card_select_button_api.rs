use super::animation_api::create_animation_table;
use super::errors::{
    animator_not_found, button_already_exists, button_not_found, sprite_not_found,
};
use super::sprite_api::create_sprite_table;
use super::{BattleLuaApi, CARD_SELECT_BUTTON_TABLE, SELECTION_CHANGE_FN, USE_FN};
use crate::battle::{BattleCallback, BattleSimulation, CardSelectButton, CardSelectButtonPath};
use crate::bindable::{CardProperties, EntityId};
use crate::lua_api::helpers::inherit_metatable;
use crate::render::ui::{FontStyle, TextStyle};
use crate::resources::{ResourcePaths, TEXT_DARK_SHADOW_COLOR};
use crate::structures::TreeIndex;
use framework::prelude::{GameIO, Vec2};

pub fn inject_card_select_button_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(CARD_SELECT_BUTTON_TABLE, "sprite", move |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;
        let sprite_table: rollback_mlua::Table = table.raw_get("#sprite")?;

        lua.pack_multi(sprite_table)
    });

    lua_api.add_convenience_method(CARD_SELECT_BUTTON_TABLE, "sprite", "texture", None);
    lua_api.add_convenience_method(CARD_SELECT_BUTTON_TABLE, "sprite", "set_texture", None);

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
        "preview_sprite",
        move |_, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;
            let sprite_table: rollback_mlua::Table = table.raw_get("#preview_sprite")?;

            lua.pack_multi(sprite_table)
        },
    );

    lua_api.add_convenience_method(
        CARD_SELECT_BUTTON_TABLE,
        "preview_sprite",
        "texture",
        Some("preview_texture"),
    );
    lua_api.add_convenience_method(
        CARD_SELECT_BUTTON_TABLE,
        "preview_sprite",
        "set_texture",
        Some("set_preview_texture"),
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

    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "use_card_preview",
        move |api_ctx, lua, params| {
            let (table, card_props): (rollback_mlua::Table, CardProperties) =
                lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;

            // get indices before mutating anything
            let sprite_tree_index = button.preview_sprite_tree_index;
            let animator_index = button.preview_animator_index;

            // update sprite tree
            let sprite_tree = simulation
                .sprite_trees
                .get_mut(sprite_tree_index)
                .ok_or_else(sprite_not_found)?;

            *sprite_tree = card_props.create_preview_tree(game_io);

            // fix placement
            let root_node = sprite_tree.root_mut();
            root_node.set_origin(Vec2::ZERO);

            let root_offset = Vec2::new(root_node.size().x * 0.5, 0.0);

            for index in sprite_tree.root_node().children().to_vec() {
                let node = &mut sprite_tree[index];
                node.set_offset(node.offset() + root_offset);
            }

            // display card name
            let mut name_style = TextStyle::new_monospace(game_io, FontStyle::Thick);
            name_style.letter_spacing = 2.0;
            name_style.shadow_color = TEXT_DARK_SHADOW_COLOR;

            let name_index =
                sprite_tree.insert_root_text_child(game_io, &name_style, &card_props.short_name);
            let name_node = sprite_tree.get_mut(name_index).unwrap();
            name_node.set_offset(Vec2::new(1.0, -name_style.line_height() - 1.0));

            // update animator
            let animator = simulation
                .animators
                .get_mut(animator_index)
                .ok_or_else(animator_not_found)?;

            let callbacks = animator.load(game_io, ResourcePaths::BLANK);
            simulation.pending_callbacks.extend(callbacks);
            simulation.call_pending_callbacks(game_io, api_ctx.resources);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "use_fixed_card_cursor",
        move |api_ctx, lua, params| {
            let (table, value): (rollback_mlua::Table, bool) = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;
            button.uses_fixed_card_cursor = value;

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "use_default_audio",
        move |api_ctx, lua, params| {
            let (table, value): (rollback_mlua::Table, bool) = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;
            button.uses_default_audio = value;

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        CARD_SELECT_BUTTON_TABLE,
        "set_description",
        |api_ctx, lua, params| {
            let (table, description): (rollback_mlua::Table, Option<String>) =
                lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let button = button_mut_from_table(simulation, &table).ok_or_else(button_not_found)?;
            button.description = description.map(|s| s.into());

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(CARD_SELECT_BUTTON_TABLE, "owner", move |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        lua.pack_multi(table.raw_get::<_, rollback_mlua::Table>("#entity")?)
    });

    callback_setter(
        lua_api,
        USE_FN,
        |button| &mut button.use_callback,
        |lua, button_table, _| {
            let player_table = button_table.get::<_, rollback_mlua::Table>("#entity")?;
            lua.pack_multi((button_table, player_table))
        },
    );

    callback_setter(
        lua_api,
        SELECTION_CHANGE_FN,
        |button| &mut button.selection_change_callback,
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

    let button = CardSelectButton::new(
        game_io,
        &mut simulation.sprite_trees,
        &mut simulation.animators,
        slot_width,
    );

    let sprite_table = create_sprite_table(
        lua,
        button.sprite_tree_index,
        TreeIndex::tree_root(),
        Some(button.animator_index),
    )?;

    let preview_sprite_table = create_sprite_table(
        lua,
        button.preview_sprite_tree_index,
        TreeIndex::tree_root(),
        Some(button.preview_animator_index),
    )?;

    *button_option = Some(Box::new(button));

    // create the table
    let table = lua.create_table()?;
    table.raw_set("#entity", entity_table.clone())?;
    table.raw_set("#entity_id", entity_id)?;
    table.raw_set("#sprite", sprite_table)?;
    table.raw_set("#preview_sprite", preview_sprite_table)?;

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
