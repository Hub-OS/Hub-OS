use super::animation_api::create_animation_table;
use super::entity_api::create_entity_table;
use super::errors::{
    action_not_found, action_step_not_found, attachment_not_found, entity_not_found,
    sprite_not_found, unmarked_dependency, warn_deprecated,
};
use super::sprite_api::create_sprite_table;
use super::tile_api::create_tile_table;
use super::*;
use crate::battle::*;
use crate::bindable::{CardProperties, EntityId, GenerationalIndex, SpriteColorMode};
use crate::lua_api::helpers::inherit_metatable;
use crate::packages::CardPackage;
use crate::render::{FrameTime, SpriteNode};
use crate::resources::Globals;
use packets::structures::PackageId;
use rollback_mlua::LuaSerdeExt;

pub fn inject_action_api(lua_api: &mut BattleLuaApi) {
    inject_step_api(lua_api);
    inject_attachment_api(lua_api);

    lua_api.add_dynamic_function(CARD_PROPERTIES_TABLE, "new", move |api_ctx, lua, _| {
        let api_ctx = api_ctx.borrow();

        let vms = api_ctx.resources.vm_manager.vms();
        let namespace = vms[api_ctx.vm_index].preferred_namespace();

        lua.pack_multi(CardProperties {
            namespace: Some(namespace),
            ..Default::default()
        })
    });

    lua_api.add_dynamic_function(
        CARD_PROPERTIES_TABLE,
        "from_package",
        |api_ctx, lua, params| {
            let (package_id, code): (PackageId, Option<String>) = lua.unpack_multi(params)?;

            let api_ctx = api_ctx.borrow();

            let vms = api_ctx.resources.vm_manager.vms();
            let vm = &vms[api_ctx.vm_index];
            let namespace = vm.preferred_namespace();

            if !vm.permitted_dependencies.contains(&package_id) {
                return Err(unmarked_dependency());
            }

            let globals = Globals::from_resources(api_ctx.game_io);
            let card_packages = &globals.card_packages;

            let mut card_properties = if let Some(package) =
                card_packages.package_or_fallback(namespace, &package_id)
            {
                let status_registry = &api_ctx.resources.status_registry;
                package.card_properties.to_bindable(status_registry)
            } else {
                log::warn!("Failed to create card properties, missing {package_id:?} in namespace {namespace:?}");
                CardProperties::default()
            };

            card_properties.code = code.unwrap_or_default();
            card_properties.namespace = Some(namespace);

            lua.pack_multi(card_properties)
        },
    );

    lua_api.add_dynamic_function(
        CARD_PROPERTIES_TABLE,
        "resolve_damage",
        |api_ctx, lua, params| {
            let (card_properties, entity_table): (CardProperties, rollback_mlua::Table) =
                lua.unpack_multi(params)?;

            let entity_id: EntityId = entity_table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let vms = api_ctx.resources.vm_manager.vms();
            let preferred_namespace = vms[api_ctx.vm_index].preferred_namespace();
            let namespace = card_properties.namespace.unwrap_or(preferred_namespace);

            let damage = CardDamageResolver::new(
                api_ctx.game_io,
                api_ctx.resources,
                namespace,
                &card_properties.package_id,
            )
            .resolve(
                api_ctx.game_io,
                api_ctx.resources,
                api_ctx.simulation,
                entity_id,
            ) + card_properties.boosted_damage;

            lua.pack_multi(damage)
        },
    );

    lua_api.add_dynamic_function(
        CARD_PROPERTIES_TABLE,
        "icon_texture",
        |api_ctx, lua, params| {
            let card_props: CardProperties = lua.unpack_multi(params)?;

            let api_ctx = api_ctx.borrow();

            let namespace = card_props.namespace.unwrap_or_else(|| {
                let vms = api_ctx.resources.vm_manager.vms();
                vms[api_ctx.vm_index].preferred_namespace()
            });

            let (_, path) =
                CardPackage::icon_texture(api_ctx.game_io, namespace, &card_props.package_id);

            lua.pack_multi(path)
        },
    );

    lua_api.add_dynamic_function(ACTION_TABLE, "from_card", move |api_ctx, lua, params| {
        let (entity_table, card_props): (rollback_mlua::Table, CardProperties) =
            lua.unpack_multi(params)?;
        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let vms = api_ctx.resources.vm_manager.vms();
        let namespace = vms[api_ctx.vm_index].preferred_namespace();

        let action_index = Action::create_from_card_properties(
            api_ctx.game_io,
            api_ctx.resources,
            api_ctx.simulation,
            entity_id,
            namespace,
            &card_props,
        );

        let optional_table = action_index
            .map(|action_index| create_action_table(lua, action_index))
            .transpose()?;

        lua.pack_multi(optional_table)
    });

    lua_api.add_dynamic_function(ACTION_TABLE, "new", move |api_ctx, lua, params| {
        let (entity_table, state): (rollback_mlua::Table, Option<String>) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = entity_table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let simulation = &mut api_ctx.simulation;
        let action_index =
            Action::create(game_io, simulation, state.unwrap_or_default(), entity_id)
                .ok_or_else(entity_not_found)?;

        let table = create_action_table(lua, action_index)?;

        lua.pack_multi(table)
    });

    getter(lua_api, "owner", |action, lua, _: ()| {
        lua.pack_multi(create_entity_table(lua, action.entity)?)
    });

    lua_api.add_dynamic_function(ACTION_TABLE, "set_lockout", move |api_ctx, lua, params| {
        let (table, lockout): (rollback_mlua::Table, rollback_mlua::Value) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let actions = &mut api_ctx.simulation.actions;
        let action = actions.get_mut(id).ok_or_else(action_not_found)?;

        action.lockout_type = lua.from_value(lockout)?;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ACTION_TABLE,
        "allow_auto_tile_reservation",
        move |api_ctx, lua, params| {
            let (table, allow): (rollback_mlua::Table, Option<bool>) = lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            if let Some(action) = simulation.actions.get_mut(id) {
                action.set_auto_reservation_preference(
                    &mut simulation.entities,
                    &mut simulation.field,
                    allow.unwrap_or(true),
                );
            };

            lua.pack_multi(())
        },
    );

    setter(
        lua_api,
        "override_animation_frames",
        |action, _, frame_data: Vec<Vec<usize>>| {
            if action.executed {
                // animation already set, ignore to avoid deleting a valid state
                return Ok(());
            }

            let derived_frames = frame_data
                .into_iter()
                .flat_map(|item| {
                    let frame_index = *item.first()?;
                    let duration = *item.get(1)? as FrameTime;

                    Some(DerivedAnimationFrame::new(frame_index.max(1) - 1, duration))
                })
                .collect();

            action.derived_frames = Some(derived_frames);
            Ok(())
        },
    );

    lua_api.add_dynamic_function(
        ACTION_TABLE,
        "add_anim_action",
        move |api_ctx, lua, params| {
            warn_deprecated(lua, ":add_anim_action() (use :on_anim_frame() instead)");

            let (table, frame, callback): (rollback_mlua::Table, usize, rollback_mlua::Function) =
                lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let actions = &mut api_ctx.simulation.actions;
            let action = actions.get_mut(id).ok_or_else(action_not_found)?;

            let callback = BattleCallback::new_lua_callback(lua, api_ctx.vm_index, callback)?;

            action.frame_callbacks.push((frame.max(1) - 1, callback));

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ACTION_TABLE,
        "on_anim_frame",
        move |api_ctx, lua, params| {
            let (table, frame, callback): (rollback_mlua::Table, usize, rollback_mlua::Function) =
                lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let actions = &mut api_ctx.simulation.actions;
            let action = actions.get_mut(id).ok_or_else(action_not_found)?;

            let callback = BattleCallback::new_lua_callback(lua, api_ctx.vm_index, callback)?;

            action.frame_callbacks.push((frame.max(1) - 1, callback));

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ACTION_TABLE, "on_end", move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let actions = &mut api_ctx.simulation.actions;
        let action = actions.get_mut(id).ok_or_else(action_not_found)?;

        let callback = BattleCallback::new_lua_callback(lua, api_ctx.vm_index, callback)?;
        action.end_callbacks.push(callback);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ACTION_TABLE, "end_action", move |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        Action::delete_multi(api_ctx.game_io, api_ctx.resources, simulation, true, [id]);

        lua.pack_multi(())
    });

    getter(
        lua_api,
        "copy_card_properties",
        move |action, lua, _: ()| lua.pack_multi(&action.properties),
    );

    setter(
        lua_api,
        "set_card_properties",
        |action, _, properties: CardProperties| {
            action.properties = properties;
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

    // setter(
    //     lua_api,
    //     "time_freeze_blackout_tiles",
    //     |action, _, enable| {
    //         action.time_freeze_blackout_tiles = enable;
    //         Ok(())
    //     },
    // );

    // todo: property getters
    callback_setter(
        lua_api,
        UPDATE_FN,
        |action| &mut action.update_callback,
        |_, lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        ANIMATION_END_FN,
        |action| &mut action.animation_end_callback,
        |_, lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        ACTION_END_FN,
        |action| &mut action.end_callback,
        |_, lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        EXECUTE_FN,
        |action| &mut action.execute_callback,
        |action, lua, table, _| {
            let entity_table = create_entity_table(lua, action.entity)?;
            lua.pack_multi((table, entity_table))
        },
    );

    callback_setter(
        lua_api,
        CAN_MOVE_TO_FN,
        |action| &mut action.can_move_to_callback,
        |_, lua, _, dest| {
            let tile_table = create_tile_table(lua, dest)?;
            lua.pack_multi(tile_table)
        },
    );
}

fn inject_step_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(ACTION_TABLE, "create_step", move |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let actions = &mut api_ctx.simulation.actions;
        let action = actions.get_mut(id).ok_or_else(action_not_found)?;

        let step = ActionStep::default();
        let index = action.steps.len();
        action.steps.push(step);

        let step_table = lua.create_table()?;
        step_table.raw_set("#id", id)?;
        step_table.raw_set("#index", index)?;
        inherit_metatable(lua, ACTION_STEP_TABLE, &step_table)?;

        lua.pack_multi(step_table)
    });

    lua_api.add_dynamic_setter(ACTION_STEP_TABLE, UPDATE_FN, |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;
        let index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let actions = &mut api_ctx.simulation.actions;
        let action = actions.get_mut(id).ok_or_else(action_not_found)?;

        let step = (action.steps)
            .get_mut(index)
            .ok_or_else(action_step_not_found)?;

        step.callback = if let Some(callback) = callback {
            let key = lua.create_registry_value(table)?;

            BattleCallback::new_transformed_lua_callback(
                lua,
                api_ctx.vm_index,
                callback,
                move |_, lua, _| {
                    let table: rollback_mlua::Table = lua.registry_value(&key)?;
                    lua.pack_multi(table)
                },
            )?
        } else {
            BattleCallback::default()
        };

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ACTION_STEP_TABLE,
        "complete_step",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let id: GenerationalIndex = table.raw_get("#id")?;
            let index: usize = table.raw_get("#index")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let actions = &mut api_ctx.simulation.actions;
            let action = actions.get_mut(id).ok_or_else(action_not_found)?;

            if let Some(step) = action.steps.get_mut(index) {
                step.completed = true;
            }

            lua.pack_multi(())
        },
    );
}

pub fn inject_attachment_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(ACTION_TABLE, "create_attachment", |api_ctx, lua, params| {
        let (action_table, point_name): (rollback_mlua::Table, String) =
            lua.unpack_multi(params)?;

        let action_id: GenerationalIndex = action_table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let table = shared_attachment_constructor(api_ctx, lua, action_id, point_name, None)?;

        lua.pack_multi(table)
    });

    lua_api.add_dynamic_function(
        ATTACHMENT_TABLE,
        "create_attachment",
        |api_ctx, lua, params| {
            let (parent_table, point_name): (rollback_mlua::Table, String) =
                lua.unpack_multi(params)?;

            let action_id: GenerationalIndex = parent_table.raw_get("#id")?;
            let parent_index: usize = parent_table.raw_get("#index")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let table = shared_attachment_constructor(
                api_ctx,
                lua,
                action_id,
                point_name,
                Some(parent_index),
            )?;

            lua.pack_multi(table)
        },
    );

    lua_api.add_dynamic_function(ATTACHMENT_TABLE, "sprite", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;
        let sprite_table: rollback_mlua::Table = table.raw_get("#sprite")?;

        lua.pack_multi(sprite_table)
    });

    lua_api.add_dynamic_function(ATTACHMENT_TABLE, "animation", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let action_id: GenerationalIndex = table.raw_get("#id")?;
        let attachment_index: usize = table.raw_get("#index")?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let actions = &mut api_ctx.simulation.actions;
        let action = actions.get_mut(action_id).ok_or_else(action_not_found)?;

        let attachment = action
            .attachments
            .get_mut(attachment_index)
            .ok_or_else(attachment_not_found)?;

        lua.pack_multi(create_animation_table(lua, attachment.animator_index)?)
    });
}

fn shared_attachment_constructor<'lua>(
    api_ctx: &mut BattleScriptContext,
    lua: &'lua rollback_mlua::Lua,
    action_id: GenerationalIndex,
    point_name: String,
    parent_index: Option<usize>,
) -> rollback_mlua::Result<rollback_mlua::Table<'lua>> {
    let simulation = &mut api_ctx.simulation;

    let actions = &mut simulation.actions;
    let action = actions.get_mut(action_id).ok_or_else(action_not_found)?;

    // create sprite node
    let entities = &mut simulation.entities;
    let entity = entities
        .query_one_mut::<&mut Entity>(action.entity.into())
        .map_err(|_| entity_not_found())?;

    let (parent_sprite_index, parent_animator_index) = match parent_index {
        Some(parent_index) => {
            let attachment = action
                .attachments
                .get_mut(parent_index)
                .ok_or_else(attachment_not_found)?;

            (attachment.sprite_index, attachment.animator_index)
        }
        None => (action.sprite_index, entity.animator_index),
    };

    let sprite_tree = simulation
        .sprite_trees
        .get_mut(entity.sprite_tree_index)
        .ok_or_else(sprite_not_found)?;

    let sprite_index = sprite_tree
        .insert_child(
            parent_sprite_index,
            SpriteNode::new(api_ctx.game_io, SpriteColorMode::Add),
        )
        .unwrap();

    // create animator
    let mut animator = BattleAnimator::new();
    animator.set_target(entity.sprite_tree_index, sprite_index);

    if !action.executed {
        // disable to prevent updates during card action startup frames
        animator.disable();
    }

    let animator_index = simulation.animators.insert(animator);

    // create attachment
    let attachment = ActionAttachment::new(
        point_name,
        sprite_index,
        animator_index,
        parent_animator_index,
    );

    // update attachment's offset
    if action.executed {
        attachment.apply_animation(sprite_tree, &mut simulation.animators);
    }

    action.attachments.push(attachment);

    // create table
    let table = lua.create_table()?;
    table.raw_set("#id", action_id)?;
    table.raw_set("#index", action.attachments.len() - 1)?;
    table.raw_set(
        "#sprite",
        create_sprite_table(
            lua,
            entity.sprite_tree_index,
            sprite_index,
            Some(animator_index),
        )?,
    )?;
    inherit_metatable(lua, ATTACHMENT_TABLE, &table)?;

    Ok(table)
}

fn getter<P>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: for<'lua> fn(
        &Action,
        &'lua rollback_mlua::Lua,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
{
    lua_api.add_dynamic_function(ACTION_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = api_ctx.borrow();
        let actions = &api_ctx.simulation.actions;
        let action = actions.get(id).ok_or_else(action_not_found)?;

        lua.pack_multi(callback(action, lua, param)?)
    });
}

fn setter<P>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: for<'lua> fn(&mut Action, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<()>,
) where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
{
    lua_api.add_dynamic_function(ACTION_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let actions = &mut api_ctx.simulation.actions;
        let action = actions.get_mut(id).ok_or_else(action_not_found)?;

        lua.pack_multi(callback(action, lua, param)?)
    });
}

fn callback_setter<P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: for<'lua> fn(&mut Action) -> &mut Option<BattleCallback<P, R>>,
    param_transformer: for<'lua> fn(
        &mut Action,
        &'lua rollback_mlua::Lua,
        rollback_mlua::Table<'lua>,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + 'static,
{
    lua_api.add_dynamic_setter(ACTION_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let actions = &mut api_ctx.simulation.actions;
        let Some(action) = actions.get_mut(id) else {
            return lua.pack_multi(());
        };

        let key = lua.create_registry_value(table)?;

        *callback_getter(action) = callback
            .map(|callback| {
                BattleCallback::new_transformed_lua_callback(
                    lua,
                    api_ctx.vm_index,
                    callback,
                    move |api_ctx, lua, p| {
                        let api_ctx = &mut *api_ctx.borrow_mut();
                        let actions = &mut api_ctx.simulation.actions;
                        let action = actions.get_mut(id).ok_or_else(action_not_found)?;

                        let table: rollback_mlua::Table = lua.registry_value(&key)?;
                        param_transformer(action, lua, table, p)
                    },
                )
            })
            .transpose()?;

        lua.pack_multi(())
    });
}

pub fn create_action_table(
    lua: &rollback_mlua::Lua,
    index: GenerationalIndex,
) -> rollback_mlua::Result<rollback_mlua::Table<'_>> {
    let table = lua.create_table()?;
    table.raw_set("#id", index)?;
    inherit_metatable(lua, ACTION_TABLE, &table)?;

    Ok(table)
}
