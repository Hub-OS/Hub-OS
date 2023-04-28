use super::animation_api::create_animation_table;
use super::errors::action_aready_used;
use super::errors::action_not_found;
use super::errors::entity_not_found;
use super::errors::invalid_sync_node;
use super::errors::mismatched_entity;
use super::errors::package_not_loaded;
use super::errors::too_many_forms;
use super::field_api::get_field_table;
use super::player_form_api::create_player_form_table;
use super::sprite_api::create_sprite_table;
use super::sync_node_api::create_sync_node_table;
use super::tile_api::create_tile_table;
use super::*;
use crate::battle::*;
use crate::bindable::*;
use crate::lua_api::errors::unexpected_nil;
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::packages::PackageId;
use crate::render::{FrameTime, SpriteNode};
use crate::resources::Globals;
use framework::prelude::Vec2;

pub fn inject_entity_api(lua_api: &mut BattleLuaApi) {
    inject_character_api(lua_api);
    inject_spell_api(lua_api);
    inject_living_api(lua_api);
    inject_player_api(lua_api);

    generate_constructor_fn(lua_api, ARTIFACT_TABLE, |api_ctx| {
        Ok(api_ctx.simulation.create_artifact(api_ctx.game_io))
    });
    generate_constructor_fn(lua_api, SPELL_TABLE, |api_ctx| {
        Ok(api_ctx.simulation.create_spell(api_ctx.game_io))
    });
    generate_constructor_fn(lua_api, OBSTACLE_TABLE, |api_ctx| {
        Ok(api_ctx.simulation.create_obstacle(api_ctx.game_io))
    });
    generate_constructor_fn(lua_api, EXPLOSION_TABLE, |api_ctx| {
        Ok(api_ctx.simulation.create_explosion(api_ctx.game_io))
    });
    generate_constructor_fn(lua_api, POOF_TABLE, |api_ctx| {
        Ok(api_ctx.simulation.create_poof(api_ctx.game_io))
    });
    generate_constructor_fn(lua_api, ALERT_TABLE, |api_ctx| {
        Ok(api_ctx.simulation.create_alert(api_ctx.game_io))
    });

    generate_cast_fn::<&Artifact>(lua_api, ARTIFACT_TABLE);
    generate_cast_fn::<hecs::Without<&Spell, &Obstacle>>(lua_api, SPELL_TABLE);
    generate_cast_fn::<&Obstacle>(lua_api, OBSTACLE_TABLE);
    generate_cast_fn::<&Player>(lua_api, PLAYER_TABLE);
    generate_cast_fn::<&Character>(lua_api, CHARACTER_TABLE);

    lua_api.add_dynamic_function(ENTITY_TABLE, "id", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        lua.pack_multi(id)
    });

    getter(lua_api, "name", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.name.clone())
    });
    setter(lua_api, "set_name", |entity: &mut Entity, _, name| {
        entity.name = name;
        Ok(())
    });

    getter(lua_api, "element", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.element)
    });
    setter(lua_api, "set_element", |entity: &mut Entity, _, element| {
        entity.element = element;
        Ok(())
    });

    getter(lua_api, "facing", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.facing)
    });
    getter(lua_api, "facing_away", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.facing.reversed())
    });
    setter(lua_api, "set_facing", |entity: &mut Entity, _, facing| {
        entity.facing = facing;
        Ok(())
    });

    getter(lua_api, "team", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.team)
    });
    setter(lua_api, "set_team", |entity: &mut Entity, _, team| {
        entity.team = team;
        Ok(())
    });
    getter(lua_api, "is_team", |entity: &Entity, lua, team| {
        lua.pack_multi(entity.team == team)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "get_tile", |api_ctx, lua, params| {
        let (table, direction, count): (rollback_mlua::Table, Option<Direction>, Option<i32>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let vec = direction.unwrap_or_default().i32_vector();
        let count = count.unwrap_or_default();

        let x = entity.x + vec.0 * count;
        let y = entity.y + vec.1 * count;

        let field = &api_ctx.simulation.field;

        if field.in_bounds((x, y)) {
            lua.pack_multi(create_tile_table(lua, (x, y))?)
        } else {
            lua.pack_multi(())
        }
    });

    getter(lua_api, "current_tile", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(create_tile_table(lua, (entity.x, entity.y))?)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "field", |_, lua, _| {
        let field_table = get_field_table(lua)?;

        lua.pack_multi(field_table)
    });

    getter(lua_api, "sharing_tile", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.share_tile)
    });
    setter(
        lua_api,
        "enable_sharing_tile",
        |entity: &mut Entity, _, share: Option<bool>| {
            entity.share_tile = share.unwrap_or(true);
            Ok(())
        },
    );

    getter(
        lua_api,
        "ignoring_negative_tile_effects",
        |entity: &Entity, lua, _: ()| lua.pack_multi(entity.ignore_negative_tile_effects),
    );

    setter(
        lua_api,
        "ignore_negative_tile_effects",
        |entity: &mut Entity, _, enabled: Option<bool>| {
            entity.ignore_negative_tile_effects = enabled.unwrap_or(true);
            Ok(())
        },
    );

    getter(
        lua_api,
        "ignoring_hole_tiles",
        |entity: &Entity, lua, _: ()| lua.pack_multi(entity.ignore_hole_tiles),
    );

    setter(
        lua_api,
        "ignore_hole_tiles",
        |entity: &mut Entity, _, enabled: Option<bool>| {
            entity.ignore_hole_tiles = enabled.unwrap_or(true);
            Ok(())
        },
    );

    getter(lua_api, "tile_offset", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(LuaVector::from(entity.tile_offset))
    });

    getter(lua_api, "offset", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(LuaVector::from(entity.offset))
    });

    setter(
        lua_api,
        "set_offset",
        |entity: &mut Entity, _, offset: (f32, f32)| {
            entity.offset = offset.into();
            Ok(())
        },
    );

    getter(lua_api, "elevation", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.elevation)
    });
    setter(
        lua_api,
        "set_elevation",
        |entity: &mut Entity, _, elevation| {
            entity.elevation = elevation;
            Ok(())
        },
    );

    getter(lua_api, "height", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.height)
    });
    setter(lua_api, "set_height", |entity: &mut Entity, _, height| {
        entity.height = height;
        Ok(())
    });

    setter(
        lua_api,
        "never_flip",
        |entity: &mut Entity, _, never_flip| {
            entity.sprite_tree.root_mut().set_never_flip(never_flip);
            Ok(())
        },
    );

    getter(lua_api, "sprite", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(create_sprite_table(
            lua,
            entity.id,
            entity.sprite_tree.root_index(),
            Some(entity.animator_index),
        )?)
    });

    getter(lua_api, "texture", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.sprite_tree.root().texture_path())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_texture", |api_ctx, lua, params| {
        let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
        let path = absolute_path(lua, path)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let game_io = &api_ctx.game_io;
        let sprite_node = entity.sprite_tree.root_mut();
        sprite_node.set_texture(game_io, path);
        simulation.animators[entity.animator_index].apply(sprite_node);

        lua.pack_multi(())
    });

    getter(lua_api, "palette", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.sprite_tree.root().palette_path())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_palette", |api_ctx, lua, params| {
        let (table, path): (rollback_mlua::Table, Option<String>) = lua.unpack_multi(params)?;
        let path = path.map(|path| absolute_path(lua, path)).transpose()?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let sprite_node = entity.sprite_tree.root_mut();
        sprite_node.set_palette(api_ctx.game_io, path);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "create_node", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let sprite_index = entity
            .sprite_tree
            .insert_root_child(SpriteNode::new(api_ctx.game_io, SpriteColorMode::Add));

        let sprite_table = create_sprite_table(lua, id, sprite_index, None);

        lua.pack_multi(sprite_table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "create_sync_node", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let sprite_index = entity
            .sprite_tree
            .insert_root_child(SpriteNode::new(api_ctx.game_io, SpriteColorMode::Add));

        // copy derived states
        let entity_animator = &simulation.animators[entity.animator_index];
        let derived_states = entity_animator.derived_states().to_vec();

        // create and setup the new animator
        let mut animator = BattleAnimator::new();
        animator.set_target(id, sprite_index);
        animator.copy_derive_states(derived_states);
        let animator_index = simulation.animators.insert(animator);

        // add the new animator to the sync list
        let entity_animator = &mut simulation.animators[entity.animator_index];
        entity_animator.add_synced_animator(animator_index);

        let sync_node_table = create_sync_node_table(lua, id, sprite_index, animator_index);

        lua.pack_multi(sync_node_table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "remove_sync_node", |api_ctx, lua, params| {
        let (table, sync_node_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let animator_index: GenerationalIndex = sync_node_table.raw_get("#anim")?;
        let animator_index = animator_index.into();

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        if let Some(animator) = simulation.animators.get(animator_index) {
            let Some(sprite_index) = animator.target_sprite_index() else {
                return Err(invalid_sync_node());
            };

            if animator.target_entity_id() != Some(id) {
                return Err(mismatched_entity());
            }

            if sprite_index == GenerationalIndex::tree_root() {
                // prevent scripters from targeting entity.animator_index
                // we use [] to access entity.animator_index, deletion without deleting the entity would cause a crash
                return Err(invalid_sync_node());
            }

            // remove sprite and animator from the simulation
            entity.sprite_tree.remove(sprite_index);
            simulation.animators.remove(animator_index);
        }

        // remove animator from the sync list
        let entity_animator = &mut simulation.animators[entity.animator_index];
        entity_animator.remove_synced_animator(animator_index);

        lua.pack_multi(())
    });

    setter(lua_api, "hide", |entity: &mut Entity, _, _: ()| {
        entity.sprite_tree.root_mut().set_visible(false);
        Ok(())
    });
    setter(lua_api, "reveal", |entity: &mut Entity, _, _: ()| {
        entity.sprite_tree.root_mut().set_visible(true);
        Ok(())
    });

    getter(lua_api, "color", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(LuaColor::from(entity.sprite_tree.root().color()))
    });
    setter(
        lua_api,
        "set_color",
        |entity: &mut Entity, _, color: LuaColor| {
            entity.sprite_tree.root_mut().set_color(color.into());
            Ok(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_shadow", |api_ctx, lua, params| {
        let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
        let path = absolute_path(lua, path)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        entity.set_shadow(api_ctx.game_io, path);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "show_shadow", |api_ctx, lua, params| {
        let (table, visible): (rollback_mlua::Table, Option<bool>) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let sprite_node = &mut entity.sprite_tree[entity.shadow_index];
        sprite_node.set_visible(visible.unwrap_or(true));

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "animation", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let animation_table = create_animation_table(lua, entity.animator_index)?;

        lua.pack_multi(animation_table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "load_animation", |api_ctx, lua, params| {
        let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
        let path = absolute_path(lua, path)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let animator = &mut simulation.animators[entity.animator_index];
        let callbacks = animator.load(api_ctx.game_io, &path);

        let root_node = entity.sprite_tree.root_mut();
        animator.apply(root_node);

        simulation.pending_callbacks.extend(callbacks);
        simulation.call_pending_callbacks(api_ctx.game_io, api_ctx.vms);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "create_component", |api_ctx, lua, params| {
        let (entity_table, lifetime): (rollback_mlua::Table, ComponentLifetime) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = entity_table.get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .map_err(|_| entity_not_found())?;

        let component = Component::new(entity_id, lifetime);
        let id = api_ctx.simulation.components.insert(component);

        if lifetime == ComponentLifetime::Local {
            entity.local_components.push(id);
        }

        let table = lua.create_table()?;
        table.raw_set("#id", GenerationalIndex::from(id))?;
        table.raw_set("#entity", entity_table)?;
        inherit_metatable(lua, COMPONENT_TABLE, &table)?;

        lua.pack_multi(table)
    });

    getter(lua_api, "context", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(&entity.hit_context)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "queue_action", |api_ctx, lua, params| {
        let (table, action_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let id: EntityId = table.raw_get("#id")?;
        let action_index: GenerationalIndex = action_table.raw_get("#id")?;

        let action = (simulation.actions)
            .get_mut(action_index.into())
            .ok_or_else(action_not_found)?;

        if action.used {
            return Err(action_aready_used());
        }

        let used = simulation.use_action(api_ctx.game_io, id, action_index.into());

        lua.pack_multi(used)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "can_move_to", move |api_ctx, lua, params| {
        let (table, tile_table): (rollback_mlua::Table, Option<rollback_mlua::Table>) =
            lua.unpack_multi(params)?;

        let tile_table = match tile_table {
            Some(tile_table) => tile_table,
            None => return lua.pack_multi(false),
        };

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let id: EntityId = table.raw_get("#id")?;
        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let dest = (tile_table.raw_get("#x")?, tile_table.raw_get("#y")?);

        if !simulation.field.in_bounds(dest) {
            return lua.pack_multi(false);
        }

        let can_move_to_callback = entity.current_can_move_to_callback(&simulation.actions);
        let can_move = can_move_to_callback.call(api_ctx.game_io, simulation, api_ctx.vms, dest);

        lua.pack_multi(can_move)
    });

    movement_function(lua_api, "teleport", |dest: (i32, i32), _: ()| {
        Movement::teleport(dest)
    });
    movement_function(lua_api, "slide", Movement::slide);
    movement_function(
        lua_api,
        "jump",
        |dest: (i32, i32), (height, frame_time): (f32, FrameTime)| {
            Movement::jump(dest, height, frame_time)
        },
    );
    lua_api.add_dynamic_function(ENTITY_TABLE, "queue_movement", |api_ctx, lua, params| {
        let (table, movement): (rollback_mlua::Table, Movement) = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        attempt_movement(api_ctx, lua, table, movement)
    });
    getter(lua_api, "is_sliding", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(
            entity
                .movement
                .as_ref()
                .map(|action| action.is_sliding())
                .unwrap_or_default(),
        )
    });
    getter(lua_api, "is_jumping", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(
            entity
                .movement
                .as_ref()
                .map(|action| action.is_jumping())
                .unwrap_or_default(),
        )
    });
    getter(lua_api, "is_teleporting", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(
            entity
                .movement
                .as_ref()
                .map(|action| action.is_teleporting())
                .unwrap_or_default(),
        )
    });
    getter(lua_api, "is_moving", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.movement.is_some())
    });

    delete_getter(lua_api, "deleted", |entity: &Entity| entity.deleted);
    delete_getter(lua_api, "will_erase_eof", |entity: &Entity| entity.erased);

    lua_api.add_dynamic_function(ENTITY_TABLE, "erase", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        simulation.mark_entity_for_erasure(api_ctx.game_io, api_ctx.vms, id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "delete", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        simulation.delete_entity(api_ctx.game_io, api_ctx.vms, id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "default_player_delete",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            simulation.delete_entity(api_ctx.game_io, api_ctx.vms, id);

            let is_living = simulation
                .entities
                .query_one_mut::<&Living>(id.into())
                .is_ok();

            if is_living {
                crate::battle::delete_player_animation(api_ctx.game_io, simulation, id);
            }

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "default_character_delete",
        |api_ctx, lua, params| {
            let (table, explosion_count): (rollback_mlua::Table, Option<usize>) =
                lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            simulation.delete_entity(api_ctx.game_io, api_ctx.vms, id);

            if simulation.entities.contains(id.into()) {
                crate::battle::delete_character_animation(simulation, id, explosion_count);
            }

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "on_delete", |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let vm_index = api_ctx.vm_index;

        let callback = BattleCallback::new_transformed_lua_callback(
            lua,
            vm_index,
            callback,
            move |_, lua, _| {
                let entity_table = create_entity_table(lua, id)?;
                lua.pack_multi(entity_table)
            },
        )?;

        entity.delete_callbacks.push(callback);

        lua.pack_multi(())
    });

    callback_setter(
        lua_api,
        SPAWN_FN,
        |entity: &mut Entity| &mut entity.spawn_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        UPDATE_FN,
        |entity: &mut Entity| &mut entity.update_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        COUNTER_FN,
        |entity: &mut Entity| &mut entity.counter_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        DELETE_FN,
        |entity: &mut Entity| &mut entity.delete_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        BATTLE_START_FN,
        |entity: &mut Entity| &mut entity.battle_start_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        BATTLE_END_FN,
        |entity: &mut Entity| &mut entity.battle_end_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        CAN_MOVE_TO_FN,
        |entity: &mut Entity| &mut entity.can_move_to_callback,
        |lua, _, dest| {
            let tile_table = create_tile_table(lua, dest);
            lua.pack_multi(tile_table)
        },
    );
}

fn inject_character_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(CHARACTER_TABLE, "from_package", |api_ctx, lua, params| {
        let (package_id, team, rank): (PackageId, Team, CharacterRank) =
            lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();

        let namespace = lua.named_registry_value(NAMESPACE_REGISTRY_KEY)?;

        let id = api_ctx.simulation.load_character(
            api_ctx.game_io,
            api_ctx.vms,
            &package_id,
            namespace,
            rank,
        )?;

        let entities = &mut api_ctx.simulation.entities;
        let entity = entities.query_one_mut::<&mut Entity>(id.into()).unwrap();
        entity.team = team;

        lua.pack_multi(create_entity_table(lua, id))
    });

    // todo: get_held_card_props

    // todo: can_attack, maybe better inverted and described as is_idle?
    // nothing is stoping a card action from being queued other than another card action
    // lua_api.add_dynamic_function(ENTITY_TABLE, "can_attack", |api_ctx, lua, params| {
    //     let entity_table: rollback_mlua::Table = lua.unpack_multi(params)?;

    //     let entity_id: EntityId = entity_table.get("#id")?;

    //     let api_ctx = &mut *api_ctx.borrow_mut();
    //     let entities = &mut api_ctx.simulation.entities;

    //     let entity = entities
    //         .query_one_mut::<&Entity>(entity_id.into())
    //         .map_err(|_| entity_not_found())?;

    //     if entity.action_index.is_some() {
    //         return lua.pack_multi(false);
    //     }

    //     if let Ok((entity, _)) = entities.query_one_mut::<(&Entity, &Player)>(entity_id.into()) {
    //         let animator = &api_ctx.simulation.animators[entity.animator_index];

    //         if animator.current_state() != Some(Player::IDLE_STATE) {
    //             return lua.pack_multi(false);
    //         }
    //     }

    //     lua.pack_multi(api_ctx.simulation.is_entity_actionable(entity_id))
    // });

    getter(lua_api, "rank", |character: &Character, lua, _: ()| {
        lua.pack_multi(character.rank)
    });
}

fn inject_spell_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(HIT_PROPS_TABLE, "new", |_, lua, params| {
        let (damage, flags, element, mut rest): (
            i32,
            HitFlags,
            Element,
            rollback_mlua::MultiValue,
        ) = lua.unpack_multi(params)?;

        let middle_param = rest.pop_front().ok_or_else(|| unexpected_nil("Element"))?;

        let secondary_element: Element;
        let context: Option<HitContext>;
        let drag: Drag;

        match lua.unpack(middle_param.clone()) {
            Ok(element) => {
                secondary_element = element;

                (context, drag) = lua.unpack_multi(rest)?;
            }
            Err(_) => {
                secondary_element = Element::None;
                context = lua.unpack(middle_param)?;
                drag = lua.unpack_multi(rest)?;
            }
        };

        lua.pack_multi(HitProperties {
            damage,
            flags,
            element,
            secondary_element,
            aggressor: EntityId::default(),
            drag,
            context: context.unwrap_or_default(),
        })
    });

    setter(
        lua_api,
        "set_tile_highlight",
        |spell: &mut Spell, _, mode: TileHighlight| {
            spell.requested_highlight = mode;
            Ok(())
        },
    );

    getter(lua_api, "copy_hit_props", |spell: &Spell, lua, _: ()| {
        lua.pack_multi(&spell.hit_props)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_hit_props", |api_ctx, lua, params| {
        let (table, props): (rollback_mlua::Table, HitProperties) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let (entity, spell) = entities
            .query_one_mut::<(&mut Entity, &mut Spell)>(id.into())
            .map_err(|_| entity_not_found())?;

        // copy context into entity for get_context
        entity.hit_context = props.context;
        spell.hit_props = props;

        lua.pack_multi(())
    });

    callback_setter(
        lua_api,
        ATTACK_FN,
        |spell: &mut Spell| &mut spell.attack_callback,
        |lua, table, id| {
            let other_table = create_entity_table(lua, id);
            lua.pack_multi((table, other_table))
        },
    );

    callback_setter(
        lua_api,
        COLLISION_FN,
        |spell: &mut Spell| &mut spell.collision_callback,
        |lua, table, id| {
            let other_table = create_entity_table(lua, id);
            lua.pack_multi((table, other_table))
        },
    );
}

fn inject_living_api(lua_api: &mut BattleLuaApi) {
    // todo: living.aggressor is never set
    // is this function necessary?
    // the documented usage was for preventing bubbles from the same starfish from hitting each other
    // but then why not just use the same team for bubbles as entities with the same team can't hit each other?
    // currently leaving as undocumented
    setter(
        lua_api,
        "ignore_common_aggressor",
        |living: &mut Living, _, enable: Option<bool>| {
            living.ignore_common_aggressor = enable.unwrap_or(true);
            Ok(())
        },
    );

    getter(lua_api, "max_health", |living: &Living, lua, _: ()| {
        lua.pack_multi(living.max_health)
    });
    getter(lua_api, "health", |living: &Living, lua, _: ()| {
        lua.pack_multi(living.health)
    });
    setter(lua_api, "set_health", |living: &mut Living, _, health| {
        living.set_health(health);
        Ok(())
    });

    getter(lua_api, "hitbox_enabled", |living: &Living, lua, _: ()| {
        lua.pack_multi(living.hitbox_enabled)
    });
    setter(
        lua_api,
        "enable_hitbox",
        |living: &mut Living, _, enabled: Option<bool>| {
            living.hitbox_enabled = enabled.unwrap_or(true);
            Ok(())
        },
    );

    getter(lua_api, "counterable", |living: &Living, lua, _: ()| {
        lua.pack_multi(living.counterable)
    });
    setter(
        lua_api,
        "set_counterable",
        |living: &mut Living, _, counterable| {
            living.counterable = counterable;
            Ok(())
        },
    );
    // todo: set_counter_frame_range

    getter(lua_api, "intangible", |living: &Living, lua, _: ()| {
        lua.pack_multi(living.intangibility.is_enabled())
    });

    setter(
        lua_api,
        "set_intangible",
        |living: &mut Living, _, (intangible, rule): (bool, Option<IntangibleRule>)| {
            if intangible {
                living.intangibility.enable(rule.unwrap_or_default());
            } else {
                living.intangibility.disable();
            }

            Ok(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "add_defense_rule", |api_ctx, lua, params| {
        let (table, defense_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        DefenseRule::add(api_ctx, lua, defense_table, id)?;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "remove_defense_rule",
        |api_ctx, lua, params| {
            let (table, defense_table): (rollback_mlua::Table, rollback_mlua::Table) =
                lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            DefenseRule::remove(api_ctx, lua, defense_table, id)?;

            lua.pack_multi(())
        },
    );

    // todo: has_status?

    setter(
        lua_api,
        "register_status_callback",
        |living: &mut Living, _, (hit_flag, callback): (HitFlags, BattleCallback)| {
            living.register_status_callback(hit_flag, callback);
            Ok(())
        },
    );

    callback_setter(
        lua_api,
        COUNTERED_FN,
        |living: &mut Living| &mut living.countered_callback,
        |lua, table, _| lua.pack_multi(table),
    );
}

fn inject_player_api(lua_api: &mut BattleLuaApi) {
    getter(lua_api, "emotion", |player: &Player, lua, ()| {
        lua.pack_multi(player.emotion_window.emotion())
    });
    setter(
        lua_api,
        "set_emotion",
        |player: &mut Player, _lua, emotion: Emotion| {
            player.emotion_window.set_emotion(emotion);
            Ok(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "input_has", |api_ctx, lua, params| {
        let (table, input_query): (rollback_mlua::Table, InputQuery) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        if let Ok(player) = entities.query_one_mut::<&mut Player>(id.into()) {
            let player_input = &simulation.inputs[player.index];

            let result = match input_query {
                InputQuery::JustPressed(input) => player_input.was_just_pressed(input),
                InputQuery::Held(input) => player_input.is_down(input),
            };

            lua.pack_multi(result)
        } else {
            lua.pack_multi(false)
        }
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "set_fully_charged_color",
        |api_ctx, lua, params| {
            let (table, color): (rollback_mlua::Table, LuaColor) = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let player = entities
                .query_one_mut::<&mut Player>(id.into())
                .map_err(|_| entity_not_found())?;

            player.charged_color = color.into();

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "set_charge_position",
        |api_ctx, lua, params| {
            let (table, x, y): (rollback_mlua::Table, f32, f32) = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let (entity, player) = entities
                .query_one_mut::<(&mut Entity, &Player)>(id.into())
                .map_err(|_| entity_not_found())?;

            entity.sprite_tree[player.charge_sprite_index].set_offset(Vec2::new(x, y));

            lua.pack_multi(())
        },
    );

    setter(
        lua_api,
        "slide_when_moving",
        |player: &mut Player, _, slide: Option<bool>| {
            player.slide_when_moving = slide.unwrap_or(true);
            Ok(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "create_form", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        if player.forms.len() >= 5 {
            return Err(too_many_forms());
        }

        let index = player.forms.len();

        player.forms.push(PlayerForm::default());

        let form_table = create_player_form_table(lua, table, index)?;

        lua.pack_multi(form_table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "get_augment", |api_ctx, lua, params| {
        let (table, augment_id): (rollback_mlua::Table, rollback_mlua::String) =
            lua.unpack_multi(params)?;

        let augment_id = augment_id.to_str()?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let mut augment_iter = player.augments.iter();
        let augment_table = augment_iter
            .find(|(_, augment)| augment.package_id.as_str() == augment_id)
            .map(|(index, _)| create_augment_table(lua, id, index))
            .transpose()?;

        lua.pack_multi(augment_table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "get_augments", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let augment_tables: rollback_mlua::Result<Vec<_>> = player
            .augments
            .iter()
            .map(|(index, _)| create_augment_table(lua, id, index))
            .collect();

        lua.pack_multi(augment_tables?)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "boost_augment", |api_ctx, lua, params| {
        let (table, augment_id, level_boost): (rollback_mlua::Table, rollback_mlua::String, i32) =
            lua.unpack_multi(params)?;

        let level_boost = level_boost.clamp(-100, 100);
        let augment_id = augment_id.to_str()?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let mut augment_iter = player.augments.iter_mut();
        let existing_augment =
            augment_iter.find(|(_, augment)| augment.package_id.as_str() == augment_id);

        if let Some((index, augment)) = existing_augment {
            let updated_level = (augment.level as i32 + level_boost).clamp(0, 100);
            augment.level = updated_level as u8;

            if augment.level == 0 {
                // delete
                let augment = player.augments.remove(index).unwrap();
                let callback = augment.delete_callback;

                callback.call(api_ctx.game_io, simulation, api_ctx.vms, ());
            }
        } else if level_boost > 0 {
            // create
            let globals = api_ctx.game_io.resource::<Globals>().unwrap();
            let package_id = PackageId::from(augment_id);
            let package = globals
                .augment_packages
                .package_or_fallback(player.namespace(), &package_id)
                .ok_or_else(|| package_not_loaded(&package_id))?;

            let package_info = &package.package_info;

            let vms = api_ctx.vms;
            let vm_index =
                BattleSimulation::find_vm(vms, &package_info.id, package_info.namespace)?;

            let index = player
                .augments
                .insert(Augment::from((package, level_boost as usize)));

            let lua = &vms[vm_index].lua;
            let has_init = lua
                .globals()
                .contains_key("augment_init")
                .unwrap_or_default();

            if has_init {
                let result = api_ctx.simulation.call_global(
                    api_ctx.game_io,
                    vms,
                    vm_index,
                    "augment_init",
                    move |lua| create_augment_table(lua, id, index),
                );

                if let Err(e) = result {
                    log::error!("{e}");
                }
            }
        }

        lua.pack_multi(())
    });

    setter(
        lua_api,
        "boost_max_health",
        |living: &mut Living, _, health: i32| {
            living.max_health += health;
            living.health = living.health.min(living.max_health);
            Ok(())
        },
    );

    getter(lua_api, "attack_level", |player: &Player, lua, _: ()| {
        lua.pack_multi(player.attack_level())
    });
    setter(
        lua_api,
        "boost_attack_level",
        |player: &mut Player, _, level: i8| {
            player.attack_boost = (player.attack_boost as i8 + level).clamp(0, 5) as u8;
            Ok(())
        },
    );

    getter(lua_api, "rapid_level", |player: &Player, lua, _: ()| {
        lua.pack_multi(player.rapid_level())
    });
    setter(
        lua_api,
        "boost_rapid_level",
        |player: &mut Player, _, level: i8| {
            player.rapid_boost = (player.rapid_boost as i8 + level).clamp(0, 5) as u8;
            Ok(())
        },
    );

    getter(lua_api, "charge_level", |player: &Player, lua, _: ()| {
        lua.pack_multi(player.charge_level())
    });
    setter(
        lua_api,
        "boost_charge_level",
        |player: &mut Player, _, level: i8| {
            player.charge_boost = (player.charge_boost as i8 + level).clamp(0, 5) as u8;
            Ok(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "calculate_default_charge_time",
        |api_ctx, lua, params| {
            let (table, level): (rollback_mlua::Table, Option<u8>) = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;
            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let player = entities
                .query_one_mut::<&mut Player>(id.into())
                .map_err(|_| entity_not_found())?;

            let level = level.unwrap_or_else(|| player.charge_level());

            let time = Player::calculate_default_charge_time(level);

            lua.pack_multi(time)
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "calculate_charge_time",
        |api_ctx, lua, params| {
            let (table, level): (rollback_mlua::Table, Option<u8>) = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;
            let vms = api_ctx.vms;

            let time = Player::calculate_charge_time(game_io, simulation, vms, id, level);

            lua.pack_multi(time)
        },
    );

    callback_setter(
        lua_api,
        CHARGE_TIMING_FN,
        |player: &mut Player| &mut player.calculate_charge_time_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        NORMAL_ATTACK_FN,
        |player: &mut Player| &mut player.normal_attack_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        CHARGED_ATTACK_FN,
        |player: &mut Player| &mut player.charged_attack_callback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        SPECIAL_ATTACK_FN,
        |player: &mut Player| &mut player.special_attack_callback,
        |lua, table, _| lua.pack_multi(table),
    );
}

fn delete_getter<F>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    F: Fn(&Entity) -> bool + 'static,
{
    lua_api.add_dynamic_function(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        match entities.query_one_mut::<&Entity>(id.into()) {
            Ok(entity) => lua.pack_multi(callback(entity)),
            Err(_) => lua.pack_multi(true),
        }
    });
}

fn getter<C, F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    C: hecs::Component,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(
            &C,
            &'lua rollback_mlua::Lua,
            P,
        ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>
        + 'static,
{
    lua_api.add_dynamic_function(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let component = entities
            .query_one_mut::<&C>(id.into())
            .map_err(|_| entity_not_found())?;

        lua.pack_multi(callback(component, lua, param)?)
    });
}

fn setter<C, F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    C: hecs::Component,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(&mut C, &'lua rollback_mlua::Lua, P) -> rollback_mlua::Result<()> + 'static,
{
    lua_api.add_dynamic_function(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let component = entities
            .query_one_mut::<&mut C>(id.into())
            .map_err(|_| entity_not_found())?;

        lua.pack_multi(callback(component, lua, param)?)
    });
}

fn callback_setter<C, G, P, F, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: G,
    param_transformer: F,
) where
    C: hecs::Component,
    P: for<'lua> rollback_mlua::ToLuaMulti<'lua>,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
    G: for<'lua> Fn(&mut C) -> &mut BattleCallback<P, R> + Send + Sync + 'static,
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
    lua_api.add_dynamic_setter(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let entity = entities
            .query_one_mut::<&mut C>(id.into())
            .map_err(|_| entity_not_found())?;

        let key = lua.create_registry_value(table)?;

        if let Some(callback) = callback {
            *callback_getter(entity) = BattleCallback::new_transformed_lua_callback(
                lua,
                api_ctx.vm_index,
                callback,
                move |_, lua, p| {
                    let table: rollback_mlua::Table = lua.registry_value(&key)?;
                    param_transformer(lua, table, p)
                },
            )?;
        } else {
            *callback_getter(entity) = BattleCallback::default();
        }

        lua.pack_multi(())
    });
}

fn movement_function<F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn((i32, i32), P) -> Movement + 'static,
{
    lua_api.add_dynamic_function(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, tile_table, rest): (
            rollback_mlua::Table,
            Option<rollback_mlua::Table>,
            rollback_mlua::MultiValue,
        ) = lua.unpack_multi(params)?;

        let Some(tile_table) = tile_table else {
            // todo: should we only accept Table instead of an option? (would throw errors on nil tiles)
            return lua.pack_multi(());
        };

        let mut rest = rest.into_vec();

        let mut on_begin = None;

        if rest.last().map(|v| v.type_name() == "function") == Some(true) {
            on_begin = lua.unpack(rest.pop().unwrap())?;
        }

        let params = lua.unpack_multi(rollback_mlua::MultiValue::from_vec(rest))?;

        let dest = (tile_table.raw_get("#x")?, tile_table.raw_get("#y")?);
        let mut movement = callback(dest, params);

        let api_ctx = &mut *api_ctx.borrow_mut();

        if let Some(on_begin) = on_begin {
            movement.on_begin = Some(BattleCallback::new_lua_callback(
                lua,
                api_ctx.vm_index,
                on_begin,
            )?);
        }

        attempt_movement(api_ctx, lua, table, movement)
    });
}

fn attempt_movement<'lua>(
    api_ctx: &mut BattleScriptContext,
    lua: &'lua rollback_mlua::Lua,
    table: rollback_mlua::Table,
    movement: Movement,
) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>> {
    let id: EntityId = table.raw_get("#id")?;

    let simulation = &mut api_ctx.simulation;

    if !simulation.field.in_bounds(movement.dest) {
        return lua.pack_multi(false);
    }

    let entities = &mut simulation.entities;

    let entity = entities
        .query_one_mut::<&mut Entity>(id.into())
        .map_err(|_| entity_not_found())?;

    if entity.movement.is_some() {
        return lua.pack_multi(false);
    }

    if !entity.can_move_to_callback.clone().call(
        api_ctx.game_io,
        simulation,
        api_ctx.vms,
        movement.dest,
    ) {
        return lua.pack_multi(false);
    }

    let entity = simulation
        .entities
        .query_one_mut::<&mut Entity>(id.into())
        .unwrap();

    entity.movement = Some(movement);

    lua.pack_multi(true)
}

fn generate_constructor_fn<F>(lua_api: &mut BattleLuaApi, table_name: &str, constructor: F)
where
    F: Fn(&mut BattleScriptContext) -> rollback_mlua::Result<EntityId> + 'static,
{
    lua_api.add_dynamic_function(table_name, "new", move |api_ctx, lua, params| {
        let team: Option<Team> = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let id = constructor(api_ctx)?;

        if let Some(team) = team {
            let entities = &mut api_ctx.simulation.entities;
            let entity = entities.query_one_mut::<&mut Entity>(id.into()).unwrap();

            entity.team = team;
        }

        lua.pack_multi(create_entity_table(lua, id)?)
    });
}

fn generate_cast_fn<Q: hecs::Query>(lua_api: &mut BattleLuaApi, table_name: &str) {
    lua_api.add_dynamic_function(table_name, "from", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        if entities.query_one_mut::<Q>(id.into()).is_ok() {
            lua.pack_multi(table)
        } else {
            lua.pack_multi(())
        }
    });
}

pub fn create_entity_table(
    lua: &rollback_mlua::Lua,
    id: EntityId,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    inherit_metatable(lua, ENTITY_TABLE, &table)?;

    table.raw_set("#id", id)?;

    Ok(table)
}
