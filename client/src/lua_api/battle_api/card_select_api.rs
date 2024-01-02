use super::errors::{entity_not_found, form_not_found};
use super::{BattleLuaApi, ENTITY_TABLE};
use crate::battle::{BattleCallback, CardSelectRestriction, Player, StagedItem, StagedItemData};
use crate::bindable::{CardProperties, EntityId};
use crate::lua_api::helpers::absolute_path;
use crate::packages::CardPackage;
use crate::resources::{AssetManager, Globals, ResourcePaths};
use framework::prelude::GameIO;

pub fn inject_card_select_api(lua_api: &mut BattleLuaApi) {
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

        let index: usize = form_table.raw_get("#index")?;
        let form_index = index.saturating_sub(1);

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

            let player = entities
                .query_one_mut::<&mut Player>(id.into())
                .map_err(|_| entity_not_found())?;

            if let Some(item) = player.staged_items.pop() {
                if let Some(callback) = item.undo_callback.clone() {
                    callback.call(game_io, resources, simulation, ());
                }
            }

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "staged_items", move |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;
        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let iter = player
            .staged_items
            .iter()
            .enumerate()
            .map(|(i, item)| (i + 1, item));
        let table = lua.create_table_from(iter)?;

        lua.pack_multi(table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "staged_item", move |api_ctx, lua, params| {
        let (table, index): (rollback_mlua::Table, usize) = lua.unpack_multi(params)?;
        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let index = index.saturating_sub(1);
        let item = player.staged_items.iter().nth(index);

        lua.pack_multi(item)
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "staged_item_texture",
        move |api_ctx, lua, params| {
            let (table, index): (rollback_mlua::Table, usize) = lua.unpack_multi(params)?;
            let id: EntityId = table.raw_get("#id")?;

            let mut api_ctx = api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let entities = &mut api_ctx.simulation.entities;

            let player = entities
                .query_one_mut::<&mut Player>(id.into())
                .map_err(|_| entity_not_found())?;

            let index = index.saturating_sub(1);
            let item = player.staged_items.iter().nth(index);

            let texture_path = item.map(|item| match &item.data {
                StagedItemData::Deck(i) => {
                    if let Some(card) = player.deck.get(*i) {
                        CardPackage::icon_texture(game_io, &card.package_id).1
                    } else {
                        ResourcePaths::CARD_ICON_MISSING
                    }
                }
                StagedItemData::Card(props) => {
                    CardPackage::icon_texture(game_io, &props.package_id).1
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

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "card_select_restriction",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;
            let id: EntityId = table.raw_get("#id")?;

            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let player = entities
                .query_one_mut::<&mut Player>(id.into())
                .map_err(|_| entity_not_found())?;

            let table = lua.create_table()?;

            match CardSelectRestriction::resolve(player) {
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

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "set_card_selection_blocked",
        move |api_ctx, lua, params| {
            let (table, blocked): (rollback_mlua::Table, bool) = lua.unpack_multi(params)?;
            let id: EntityId = table.raw_get("#id")?;

            let mut api_ctx = api_ctx.borrow_mut();
            let entities = &mut api_ctx.simulation.entities;

            let player = entities
                .query_one_mut::<&mut Player>(id.into())
                .map_err(|_| entity_not_found())?;

            player.card_select_blocked = blocked;

            lua.pack_multi(())
        },
    );
}

fn generate_stage_item_fn<F>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    F: for<'lua> Fn(
            &mut Player,
            &'lua rollback_mlua::Lua,
            &GameIO,
            rollback_mlua::MultiValue<'lua>,
        ) -> rollback_mlua::Result<StagedItemData>
        + 'static,
{
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
        let entities = &mut api_ctx.simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let data = callback(player, lua, game_io, params)?;
        let undo_callback = undo_callback
            .map(|function| BattleCallback::new_lua_callback(lua, vm_index, function))
            .transpose()?;

        let item = StagedItem {
            data,
            undo_callback,
        };

        player.staged_items.stage_item(item);

        lua.pack_multi(())
    });
}
