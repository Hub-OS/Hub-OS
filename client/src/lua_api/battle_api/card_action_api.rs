use super::animation_api::create_animation_table;
use super::entity_api::create_entity_table;
use super::errors::{
    attachment_not_found, card_action_not_found, card_action_step_not_found, entity_not_found,
};
use super::sprite_api::create_sprite_table;
use super::tile_api::create_tile_table;
use super::*;
use crate::battle::*;
use crate::bindable::{CardProperties, EntityId, GenerationalIndex, SpriteColorMode};
use crate::lua_api::helpers::inherit_metatable;
use crate::render::{DerivedFrame, FrameTime, SpriteNode};
use rollback_mlua::LuaSerdeExt;

pub fn inject_card_action_api(lua_api: &mut BattleLuaApi) {
    inject_step_api(lua_api);
    inject_attachment_api(lua_api);

    lua_api.add_dynamic_function(CARD_PROPERTIES_TABLE, "new", move |_, lua, _| {
        lua.pack_multi(&CardProperties::default())
    });

    lua_api.add_dynamic_function(CARD_ACTION_TABLE, "new", move |api_ctx, lua, params| {
        let (entity_table, state): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let entity = entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let mut sprite_node = SpriteNode::new(api_ctx.game_io, SpriteColorMode::Add);
        sprite_node.set_visible(false);

        let sprite_index = entity.sprite_tree.insert_root_child(sprite_node);

        let card_action = CardAction::new(entity_id, state, sprite_index);
        let card_action_index = api_ctx.simulation.card_actions.insert(card_action);

        let table = create_card_action_table(lua, card_action_index)?;

        lua.pack_multi(table)
    });

    lua_api.add_dynamic_function(
        CARD_ACTION_TABLE,
        "set_lockout",
        move |api_ctx, lua, params| {
            let (table, lockout): (rollback_mlua::Table, rollback_mlua::Value) =
                lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;

            let mut api_ctx = api_ctx.borrow_mut();
            let card_actions = &mut api_ctx.simulation.card_actions;
            let card_action = card_actions
                .get_mut(id.into())
                .ok_or_else(card_action_not_found)?;

            card_action.lockout_type = lua.from_value(lockout)?;

            lua.pack_multi(())
        },
    );

    setter(
        lua_api,
        "override_animation_frames",
        |card_action, _, frame_data: Vec<Vec<usize>>| {
            let derived_frames = frame_data
                .into_iter()
                .flat_map(|item| {
                    let frame_index = *item.first()?;
                    let duration = *item.get(1)? as FrameTime;

                    Some(DerivedFrame::new(frame_index.max(1) - 1, duration))
                })
                .collect();

            card_action.derived_frames = Some(derived_frames);
            Ok(())
        },
    );

    lua_api.add_dynamic_function(
        CARD_ACTION_TABLE,
        "add_anim_action",
        move |api_ctx, lua, params| {
            let (table, frame, callback): (rollback_mlua::Table, usize, rollback_mlua::Function) =
                lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let card_actions = &mut api_ctx.simulation.card_actions;
            let card_action = card_actions
                .get_mut(id.into())
                .ok_or_else(card_action_not_found)?;

            let callback = BattleCallback::new_lua_callback(lua, api_ctx.vm_index, callback)?;

            card_action
                .frame_callbacks
                .push((frame.max(1) - 1, callback));

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        CARD_ACTION_TABLE,
        "end_action",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let card_actions = &mut simulation.card_actions;

            if card_actions.get_mut(id.into()).is_none() {
                return Err(card_action_not_found());
            }

            simulation.delete_card_actions(api_ctx.game_io, api_ctx.vms, &[id.into()]);

            lua.pack_multi(())
        },
    );

    getter(lua_api, "get_actor", |card_action, lua, _: ()| {
        lua.pack_multi(create_entity_table(lua, card_action.entity)?)
    });

    getter(lua_api, "copy_metadata", move |card_action, lua, _: ()| {
        lua.pack_multi(&card_action.properties)
    });

    setter(
        lua_api,
        "set_metadata",
        |card_action, _, properties: CardProperties| {
            card_action.properties = properties;
            Ok(())
        },
    );

    //   "set_background", sol::overload(
    //     [](WeakWrapper<ScriptedCardAction>& cardAction, const std::string& bgTexturePath, const std::string& animPath, float velx, float vely) {
    //       cardAction.Unwrap()->SetBackgroundData(bgTexturePath, animPath, velx, vely);
    //     },
    //     [](WeakWrapper<ScriptedCardAction>& cardAction, const sf::Color& color) {
    //       cardAction.Unwrap()->SetBackgroundColor(color);
    //     }
    //   );

    setter(
        lua_api,
        "time_freeze_blackout_tiles",
        |card_action, _, enable| {
            card_action.time_freeze_blackout_tiles = enable;
            Ok(())
        },
    );

    // todo: property getters
    callback_setter(
        lua_api,
        UPDATE_FN,
        |card_action| &mut card_action.update_callback,
        |_, lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        ANIMATION_END_FN,
        |card_action| &mut card_action.animation_end_callback,
        |_, lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        ACTION_END_FN,
        |card_action| &mut card_action.end_callback,
        |_, lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        EXECUTE_FN,
        |card_action| &mut card_action.execute_callback,
        |card_action, lua, table, _| {
            let entity_table = create_entity_table(lua, card_action.entity)?;
            lua.pack_multi((table, entity_table))
        },
    );

    callback_setter(
        lua_api,
        CAN_MOVE_TO_FN,
        |card_action| &mut card_action.can_move_to_callback,
        |_, lua, table, dest| {
            let tile_table = create_tile_table(lua, dest)?;
            lua.pack_multi((table, tile_table))
        },
    );
}

fn inject_step_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(
        CARD_ACTION_TABLE,
        "create_step",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;

            let mut api_ctx = api_ctx.borrow_mut();
            let card_actions = &mut api_ctx.simulation.card_actions;
            let card_action = card_actions
                .get_mut(id.into())
                .ok_or_else(card_action_not_found)?;

            let step = CardActionStep::default();
            let index = card_action.steps.len();
            card_action.steps.push(step);

            let step_table = lua.create_table()?;
            step_table.raw_set("#id", id)?;
            step_table.raw_set("#index", index)?;
            inherit_metatable(lua, STEP_TABLE, &step_table)?;

            lua.pack_multi(step_table)
        },
    );

    lua_api.add_dynamic_setter(STEP_TABLE, UPDATE_FN, |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;
        let index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let card_actions = &mut api_ctx.simulation.card_actions;
        let card_action = card_actions
            .get_mut(id.into())
            .ok_or_else(card_action_not_found)?;

        let step = (card_action.steps)
            .get_mut(index)
            .ok_or_else(card_action_step_not_found)?;

        let key = lua.create_registry_value(table)?;
        step.callback = BattleCallback::new_transformed_lua_callback(
            lua,
            api_ctx.vm_index,
            callback,
            move |_, lua, _| {
                let table: rollback_mlua::Table = lua.registry_value(&key)?;
                lua.pack_multi(table)
            },
        )?;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(STEP_TABLE, "complete_step", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;
        let index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let card_actions = &mut api_ctx.simulation.card_actions;
        let card_action = card_actions
            .get_mut(id.into())
            .ok_or_else(card_action_not_found)?;

        let step = (card_action.steps)
            .get_mut(index)
            .ok_or_else(card_action_step_not_found)?;

        step.completed = true;

        lua.pack_multi(())
    });
}

pub fn inject_attachment_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(
        CARD_ACTION_TABLE,
        "add_attachment",
        |api_ctx, lua, params| {
            let (card_action_table, point_name): (rollback_mlua::Table, String) =
                lua.unpack_multi(params)?;

            let card_action_id: GenerationalIndex = card_action_table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let table =
                shared_attachment_constructor(api_ctx, lua, card_action_id, point_name, None)?;

            lua.pack_multi(table)
        },
    );

    lua_api.add_dynamic_function(
        ATTACHMENT_TABLE,
        "add_attachment",
        |api_ctx, lua, params| {
            let (parent_table, point_name): (rollback_mlua::Table, String) =
                lua.unpack_multi(params)?;

            let card_action_id: GenerationalIndex = parent_table.raw_get("#id")?;
            let parent_index: usize = parent_table.raw_get("#index")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let table = shared_attachment_constructor(
                api_ctx,
                lua,
                card_action_id,
                point_name,
                Some(parent_index),
            )?;

            lua.pack_multi(table)
        },
    );

    lua_api.add_dynamic_function(ATTACHMENT_TABLE, "sprite", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let card_action_id: GenerationalIndex = table.raw_get("#id")?;
        let attachment_index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let card_actions = &mut api_ctx.simulation.card_actions;
        let card_action = card_actions
            .get_mut(card_action_id.into())
            .ok_or_else(card_action_not_found)?;

        let attachment = card_action
            .attachments
            .get_mut(attachment_index)
            .ok_or_else(attachment_not_found)?;

        let sprite_index = attachment.sprite_index;

        lua.pack_multi(create_sprite_table(
            lua,
            card_action.entity,
            sprite_index,
            Some(attachment.animator_index),
        )?)
    });

    lua_api.add_dynamic_function(ATTACHMENT_TABLE, "get_animation", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let card_action_id: GenerationalIndex = table.raw_get("#id")?;
        let attachment_index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let card_actions = &mut api_ctx.simulation.card_actions;
        let card_action = card_actions
            .get_mut(card_action_id.into())
            .ok_or_else(card_action_not_found)?;

        let attachment = card_action
            .attachments
            .get_mut(attachment_index)
            .ok_or_else(attachment_not_found)?;

        lua.pack_multi(create_animation_table(lua, attachment.animator_index)?)
    });
}

fn shared_attachment_constructor<'lua>(
    api_ctx: &mut BattleScriptContext,
    lua: &'lua rollback_mlua::Lua,
    card_action_id: GenerationalIndex,
    point_name: String,
    parent_index: Option<usize>,
) -> rollback_mlua::Result<rollback_mlua::Table<'lua>> {
    let simulation = &mut api_ctx.simulation;

    let card_actions = &mut simulation.card_actions;
    let card_action = card_actions
        .get_mut(card_action_id.into())
        .ok_or_else(card_action_not_found)?;

    // create sprite node
    let entities = &mut simulation.entities;
    let entity = entities
        .query_one_mut::<&mut Entity>(card_action.entity.into())
        .map_err(|_| entity_not_found())?;

    let (parent_sprite_index, parent_animator_index) = match parent_index {
        Some(parent_index) => {
            let attachment = card_action
                .attachments
                .get_mut(parent_index)
                .ok_or_else(attachment_not_found)?;

            (attachment.sprite_index, attachment.animator_index)
        }
        None => (card_action.sprite_index, entity.animator_index),
    };

    let sprite_index = entity
        .sprite_tree
        .insert_child(
            parent_sprite_index,
            SpriteNode::new(api_ctx.game_io, SpriteColorMode::Add),
        )
        .unwrap();

    // create animator
    let mut animator = BattleAnimator::new();
    animator.set_target(card_action.entity, sprite_index);

    if !card_action.executed {
        // disable to prevent updates during card action startup frames
        animator.disable();
    }

    let animator_index = simulation.animators.insert(animator);

    // create attachment
    let attachment = CardActionAttachment::new(
        point_name,
        sprite_index,
        animator_index,
        parent_animator_index,
    );

    // update attachment's offset
    if card_action.executed {
        attachment.apply_animation(&mut entity.sprite_tree, &mut simulation.animators);
    }

    card_action.attachments.push(attachment);

    // create table
    let table = lua.create_table()?;
    table.raw_set("#id", card_action_id)?;
    table.raw_set("#index", card_action.attachments.len() - 1)?;
    inherit_metatable(lua, ATTACHMENT_TABLE, &table)?;

    Ok(table)
}

fn getter<F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(
            &CardAction,
            &'lua rollback_mlua::Lua,
            P,
        ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>
        + 'static,
{
    lua_api.add_dynamic_function(CARD_ACTION_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = api_ctx.borrow();
        let card_actions = &api_ctx.simulation.card_actions;
        let card_action = card_actions
            .get(id.into())
            .ok_or_else(card_action_not_found)?;

        lua.pack_multi(callback(card_action, lua, param)?)
    });
}

fn setter<F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(&mut CardAction, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<()>
        + 'static,
{
    lua_api.add_dynamic_function(CARD_ACTION_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let card_actions = &mut api_ctx.simulation.card_actions;
        let card_action = card_actions
            .get_mut(id.into())
            .ok_or_else(card_action_not_found)?;

        lua.pack_multi(callback(card_action, lua, param)?)
    });
}

fn callback_setter<G, P, F, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: G,
    param_transformer: F,
) where
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default,
    G: for<'lua> Fn(&mut CardAction) -> &mut Option<BattleCallback<P, R>> + Send + Sync + 'static,
    F: for<'lua> Fn(
            &mut CardAction,
            &'lua rollback_mlua::Lua,
            rollback_mlua::Table<'lua>,
            P,
        ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>
        + Send
        + Sync
        + Copy
        + 'static,
{
    lua_api.add_dynamic_setter(CARD_ACTION_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let card_actions = &mut api_ctx.simulation.card_actions;
        let card_action = card_actions
            .get_mut(id.into())
            .ok_or_else(card_action_not_found)?;

        let key = lua.create_registry_value(table)?;

        *callback_getter(card_action) = callback
            .map(|callback| {
                BattleCallback::new_transformed_lua_callback(
                    lua,
                    api_ctx.vm_index,
                    callback,
                    move |api_ctx, lua, p| {
                        let api_ctx = &mut *api_ctx.borrow_mut();
                        let card_actions = &mut api_ctx.simulation.card_actions;
                        let card_action = card_actions
                            .get_mut(id.into())
                            .ok_or_else(card_action_not_found)?;

                        let table: rollback_mlua::Table = lua.registry_value(&key)?;
                        param_transformer(card_action, lua, table, p)
                    },
                )
            })
            .transpose()?;

        lua.pack_multi(())
    });
}

fn create_card_action_table(
    lua: &rollback_mlua::Lua,
    index: generational_arena::Index,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    table.raw_set("#id", GenerationalIndex::from(index))?;
    inherit_metatable(lua, CARD_ACTION_TABLE, &table)?;

    Ok(table)
}
