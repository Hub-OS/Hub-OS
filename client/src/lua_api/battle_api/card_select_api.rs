use super::errors::{entity_not_found, form_not_found};
use super::{BattleLuaApi, ENTITY_TABLE};
use crate::battle::{
    BattleCallback, CardSelectRestriction, Player, PlayerHand, StagedItem, StagedItemData,
};
use crate::bindable::{CardProperties, EntityId};
use crate::lua_api::battle_api::entity_api::add_entity_query_fn;
use crate::lua_api::helpers::absolute_path;
use crate::packages::{CardPackage, PackageNamespace};
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::GameIO;

pub fn inject_card_select_api(lua_api: &mut BattleLuaApi) {
    add_entity_query_fn::<&mut PlayerHand>(lua_api, "staged_items_confirmed", |hand, lua, _, _| {
        lua.pack_multi(hand.staged_items.confirmed())
    });

    add_entity_query_fn::<&mut PlayerHand>(lua_api, "confirm_staged_items", |hand, lua, _, _| {
        hand.staged_items.set_confirmed(true);
        lua.pack_multi(())
    });

    generate_stage_item_fn(lua_api, "stage_card", |_, lua, _, params| {
        let properties: CardProperties = lua.unpack_multi(params)?;
        Ok(StagedItemData::Card(properties))
    });

    generate_stage_item_fn(lua_api, "stage_deck_card", |_, lua, _, params| {
        let index: usize = lua.unpack_multi(params)?;

        Ok(StagedItemData::Deck(index.saturating_sub(1)))
    });

    generate_stage_item_fn(lua_api, "stage_deck_discard", |_, lua, _, params| {
        let index: usize = lua.unpack_multi(params)?;

        Ok(StagedItemData::Discard(index.saturating_sub(1)))
    });

    generate_stage_item_fn(lua_api, "stage_form", |player, lua, game_io, params| {
        let (form_table, texture_path): (rollback_mlua::Table, Option<String>) =
            lua.unpack_multi(params)?;

        let form_index: usize = form_table.raw_get("#index")?;

        if form_index > player.forms.len() {
            return Err(form_not_found());
        }

        let texture_path = texture_path
            .map(|path| -> rollback_mlua::Result<Box<str>> { Ok(absolute_path(lua, path)?.into()) })
            .transpose()?;
        let texture = texture_path.as_ref().map(|path| {
            let globals = game_io.resource::<Globals>().unwrap();
            let assets = &globals.assets;
            assets.texture(game_io, path)
        });

        Ok(StagedItemData::Form((form_index, texture, texture_path)))
    });

    generate_stage_item_fn(lua_api, "stage_icon", |_, lua, game_io, params| {
        let texture_path: String = lua.unpack_multi(params)?;
        let texture_path: Box<str> = absolute_path(lua, texture_path)?.into();

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;
        let texture = assets.texture(game_io, &texture_path);

        Ok(StagedItemData::Icon((texture, texture_path)))
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "pop_staged_item",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;
            let id: EntityId = table.raw_get("#id")?;

            let mut api_ctx = api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let resources = api_ctx.resources;
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let (player, hand) = entities
                .query_one_mut::<(&mut Player, &mut PlayerHand)>(id.into())
                .map_err(|_| entity_not_found())?;

            if let Some(item) = hand.staged_items.pop() {
                if let StagedItemData::Form((index, ..)) = item.data
                    && let Some(callback) = &player.forms[index].deselect_callback
                {
                    callback.clone().call(game_io, resources, simulation, ());
                }

                if let Some(callback) = item.undo_callback.clone() {
                    callback.call(game_io, resources, simulation, ());
                }
            }

            lua.pack_multi(())
        },
    );

    add_entity_query_fn::<&mut PlayerHand>(lua_api, "staged_items", move |hand, lua, _, _| {
        let iter = hand
            .staged_items
            .iter()
            .enumerate()
            .map(|(i, item)| (i + 1, item));
        let table = lua.create_table_from(iter)?;

        lua.pack_multi(table)
    });

    add_entity_query_fn::<&mut PlayerHand>(lua_api, "staged_item", move |hand, lua, _, params| {
        let index: usize = lua.unpack_multi(params)?;

        let index = index.saturating_sub(1);
        let item = hand.staged_items.iter().nth(index);

        lua.pack_multi(item)
    });

    add_entity_query_fn::<&mut PlayerHand>(
        lua_api,
        "staged_item_texture",
        |hand, lua, game_io, params| {
            let index: usize = lua.unpack_multi(params)?;

            let index = index.saturating_sub(1);
            let item = hand.staged_items.iter().nth(index);

            let texture_path = item.map(|item| match &item.data {
                StagedItemData::Deck(i) => {
                    if let Some((card, namespace)) = hand.deck.get(*i) {
                        CardPackage::icon_texture(game_io, *namespace, &card.package_id).1
                    } else {
                        ResourcePaths::CARD_ICON_MISSING
                    }
                }
                StagedItemData::Card(props) => {
                    CardPackage::icon_texture(
                        game_io,
                        props.namespace.unwrap_or(PackageNamespace::Local),
                        &props.package_id,
                    )
                    .1
                }
                StagedItemData::Form((_, _, texture_path)) => {
                    texture_path.as_ref().map(|s| &**s).unwrap_or("")
                }
                StagedItemData::Icon((_, texture_path)) => texture_path,
                StagedItemData::Discard(_) => "",
            });

            lua.pack_multi(texture_path)
        },
    );

    add_entity_query_fn::<&mut PlayerHand>(
        lua_api,
        "card_select_restriction",
        move |hand, lua, _, _| {
            let table = lua.create_table()?;

            match CardSelectRestriction::resolve(hand) {
                CardSelectRestriction::Code(code) => {
                    table.set("code", code)?;
                }
                CardSelectRestriction::Package(package_id) => {
                    table.set("package_id", package_id.as_str())?;
                }
                CardSelectRestriction::Mixed { package_id, code } => {
                    table.set("package_id", package_id.as_str())?;
                    table.set("code", code)?;
                }
                CardSelectRestriction::Any => {}
            }

            lua.pack_multi(table)
        },
    );

    add_entity_query_fn::<&mut PlayerHand>(
        lua_api,
        "set_card_selection_blocked",
        move |hand, lua, _, params| {
            hand.card_select_blocked = lua.unpack_multi(params)?;

            lua.pack_multi(())
        },
    );
}

fn generate_stage_item_fn(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: for<'lua> fn(
        &mut Player,
        &'lua rollback_mlua::Lua,
        &GameIO,
        rollback_mlua::MultiValue<'lua>,
    ) -> rollback_mlua::Result<StagedItemData>,
) {
    lua_api.add_dynamic_function(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, rest): (rollback_mlua::Table, rollback_mlua::MultiValue) =
            lua.unpack_multi(params)?;

        let mut rest = rest.into_vec();
        let mut undo_callback = None;

        if rest.last().map(|v| v.type_name() == "function") == Some(true) {
            undo_callback = lua.unpack(rest.pop().unwrap())?;
        }

        let params = lua.unpack_multi(rollback_mlua::MultiValue::from_vec(rest))?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let vm_index = api_ctx.vm_index;
        let game_io = api_ctx.game_io;
        let resources = api_ctx.resources;
        let simulation = &mut *api_ctx.simulation;
        let entities = &mut simulation.entities;

        let (player, hand) = entities
            .query_one_mut::<(&mut Player, &mut PlayerHand)>(id.into())
            .map_err(|_| entity_not_found())?;

        let data = callback(player, lua, game_io, params)?;
        let undo_callback = undo_callback
            .map(|function| BattleCallback::new_lua_callback(lua, vm_index, function))
            .transpose()?;

        let item = StagedItem {
            data,
            undo_callback,
        };

        if let StagedItemData::Form((index, ..)) = item.data {
            if let Some((prev_index, undo_callback)) = hand.staged_items.drop_form_selection() {
                if let Some(callback) = &player.forms[prev_index].deselect_callback {
                    simulation.pending_callbacks.push(callback.clone());
                }

                if let Some(callback) = undo_callback {
                    simulation.pending_callbacks.push(callback);
                }
            }

            if let Some(callback) = &player.forms[index].select_callback {
                simulation.pending_callbacks.push(callback.clone());
            }
        }

        hand.staged_items.stage_item(item);
        simulation.call_pending_callbacks(game_io, resources);

        lua.pack_multi(())
    });
}
