use super::animation_api::create_animation_table;
use super::errors::{
    action_aready_processed, action_entity_mismatch, action_not_found, aux_prop_already_bound,
    entity_not_found, invalid_memory_value, invalid_sync_node, mismatched_entity,
    package_not_loaded, sprite_not_found, too_many_forms, unmarked_dependency,
};
use super::field_api::get_field_compat_table;
use super::player_form_api::create_player_form_table;
use super::sprite_api::create_sprite_table;
use super::sync_node_api::create_sync_node_table;
use super::tile_api::{create_tile_table, tile_mut_from_table};
use super::*;
use crate::battle::*;
use crate::bindable::*;
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::packages::{PackageId, PackageNamespace};
use crate::render::{FrameTime, SpriteNode};
use crate::resources::Globals;
use crate::saves::Card;
use crate::structures::TreeIndex;
use framework::common::GameIO;
use framework::prelude::Vec2;
use packets::structures::MemoryCell;

pub fn inject_entity_api(lua_api: &mut BattleLuaApi) {
    inject_character_api(lua_api);
    inject_spell_api(lua_api);
    inject_living_api(lua_api);
    inject_player_api(lua_api);

    generate_constructor_fn(lua_api, ARTIFACT_TABLE, |api_ctx| {
        Ok(Artifact::create(api_ctx.game_io, api_ctx.simulation))
    });
    generate_constructor_fn(lua_api, SPELL_TABLE, |api_ctx| {
        Ok(Spell::create(api_ctx.game_io, api_ctx.simulation))
    });
    generate_constructor_fn(lua_api, OBSTACLE_TABLE, |api_ctx| {
        Ok(Obstacle::create(api_ctx.game_io, api_ctx.simulation))
    });
    generate_constructor_fn(lua_api, EXPLOSION_TABLE, |api_ctx| {
        Ok(Artifact::create_explosion(
            api_ctx.game_io,
            api_ctx.simulation,
        ))
    });
    generate_constructor_fn(lua_api, POOF_TABLE, |api_ctx| {
        Ok(Artifact::create_poof(api_ctx.game_io, api_ctx.simulation))
    });
    generate_constructor_fn(lua_api, ALERT_TABLE, |api_ctx| {
        Ok(Artifact::create_alert(api_ctx.game_io, api_ctx.simulation))
    });
    generate_constructor_fn(lua_api, TRAP_ALERT_TABLE, |api_ctx| {
        Ok(Artifact::create_trap_alert(
            api_ctx.game_io,
            api_ctx.simulation,
        ))
    });

    generate_cast_fn::<&Artifact>(lua_api, ARTIFACT_TABLE);
    generate_cast_fn::<hecs::Without<&Spell, &Obstacle>>(lua_api, SPELL_TABLE);
    generate_cast_fn::<&Obstacle>(lua_api, OBSTACLE_TABLE);
    generate_cast_fn::<&Living>(lua_api, LIVING_TABLE);
    generate_cast_fn::<&Player>(lua_api, PLAYER_TABLE);
    generate_cast_fn::<&Character>(lua_api, CHARACTER_TABLE);

    lua_api.add_dynamic_function(ENTITY_TABLE, "id", |_, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        lua.pack_multi(id)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "name", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let name = simulation
            .entities
            .query_one_mut::<&EntityName>(id.into())
            .map(|name| &*name.0)
            .unwrap_or("");

        lua.pack_multi(name)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_name", |api_ctx, lua, params| {
        let (table, name): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        if name.is_empty() {
            let _ = simulation.entities.remove_one::<EntityName>(id.into());
        } else {
            let name = name.into();
            let _ = simulation.entities.insert_one(id.into(), EntityName(name));
        }

        lua.pack_multi(())
    });

    getter::<&Entity, _>(lua_api, "element", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.element)
    });
    setter(lua_api, "set_element", |entity: &mut Entity, _, element| {
        entity.element = element;
        Ok(())
    });

    getter::<&Entity, _>(lua_api, "facing", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.facing)
    });
    getter::<&Entity, _>(lua_api, "facing_away", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.facing.reversed())
    });
    setter(lua_api, "set_facing", |entity: &mut Entity, _, facing| {
        entity.facing = facing;
        Ok(())
    });

    getter::<&Entity, _>(lua_api, "team", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.team)
    });
    setter(lua_api, "set_team", |entity: &mut Entity, _, team| {
        entity.team = team;
        Ok(())
    });
    getter::<&Entity, _>(lua_api, "is_team", |entity: &Entity, lua, team| {
        lua.pack_multi(entity.team.is_allied(team))
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_owner", |api_ctx, lua, params| {
        let (table, owner_value): (rollback_mlua::Table, Option<rollback_mlua::Value>) =
            lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let owner = owner_value
            .map(|value| {
                if let Ok(team) = lua.unpack(value.clone()) {
                    Ok(EntityOwner::Team(team))
                } else if let rollback_mlua::Value::Table(table) = value {
                    let entity_id: EntityId = table.raw_get("#id")?;

                    let entities = &mut simulation.entities;
                    let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
                        return Err(entity_not_found());
                    };

                    Ok(EntityOwner::Entity(entity.team, entity_id))
                } else {
                    Err(rollback_mlua::Error::FromLuaConversionError {
                        from: value.type_name(),
                        to: "Entity or Team",
                        message: None,
                    })
                }
            })
            .transpose()?;

        let id: EntityId = table.raw_get("#id")?;

        let Some(owner) = owner else {
            simulation.ownership_tracking.untrack(id);
            return lua.pack_multi(());
        };

        let pending_delete = simulation.ownership_tracking.track(owner, id);

        if let Some(id) = pending_delete {
            Entity::delete(api_ctx.game_io, api_ctx.resources, simulation, id);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "owner", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let owner = simulation.ownership_tracking.resolve_owner(id);

        lua.pack_multi(owner)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "get_tile", |api_ctx, lua, params| {
        let (table, direction, distance): (rollback_mlua::Table, Option<Direction>, Option<i32>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let vec = direction.unwrap_or_default().i32_vector();
        let distance = distance.unwrap_or_default();

        let x = entity.x + vec.0 * distance;
        let y = entity.y + vec.1 * distance;

        let field = &api_ctx.simulation.field;

        if field.in_bounds((x, y)) {
            lua.pack_multi(create_tile_table(lua, (x, y))?)
        } else {
            lua.pack_multi(())
        }
    });

    getter::<&Entity, _>(lua_api, "current_tile", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(create_tile_table(lua, (entity.x, entity.y))?)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "field", |_, lua, _| {
        let field_table = get_field_compat_table(lua)?;

        lua.pack_multi(field_table)
    });

    getter::<&Entity, _>(lua_api, "spawned", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.spawned)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "hittable", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let living_hittable = entities
            .query_one_mut::<&mut Living>(id.into())
            .ok()
            .map(|living| living.hitbox_enabled || living.health > 0)
            .unwrap_or_default();

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        lua.pack_multi(!entity.deleted && entity.on_field && living_hittable)
    });

    getter::<&Entity, _>(lua_api, "sharing_tile", |entity: &Entity, lua, _: ()| {
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

    getter::<&Entity, _>(lua_api, "auto_reserves", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.auto_reserves_tiles)
    });
    setter(
        lua_api,
        "set_auto_reserve",
        |entity: &mut Entity, _, share: Option<bool>| {
            entity.auto_reserves_tiles = share.unwrap_or(true);
            Ok(())
        },
    );

    getter::<&Entity, _>(
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

    getter::<&Entity, _>(
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

    getter::<&Entity, _>(lua_api, "movement_offset", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(LuaVector::from(entity.movement_offset))
    });

    setter(
        lua_api,
        "set_movement_offset",
        |entity: &mut Entity, _, offset: (f32, f32)| {
            entity.movement_offset = offset.into();
            Ok(())
        },
    );

    getter::<&Entity, _>(lua_api, "offset", |entity: &Entity, lua, _: ()| {
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

    getter::<&Entity, _>(lua_api, "elevation", |entity: &Entity, lua, _: ()| {
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

    getter::<&Entity, _>(lua_api, "height", |entity: &Entity, lua, _: ()| {
        lua.pack_multi(entity.height)
    });
    setter(lua_api, "set_height", |entity: &mut Entity, _, height| {
        entity.height = height;
        Ok(())
    });

    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "never_flip", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "set_never_flip", None);

    lua_api.add_dynamic_function(ENTITY_TABLE, "sprite", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        // get sprite table from cache
        if let Ok(sprite_table) = table.raw_get::<_, rollback_mlua::Table>("#sprite") {
            return lua.pack_multi(sprite_table);
        }

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let sprite_table = create_sprite_table(
            lua,
            entity.sprite_tree_index,
            TreeIndex::tree_root(),
            Some(entity.animator_index),
        )?;

        // cache sprite table
        table.raw_set("#sprite", sprite_table.clone())?;

        lua.pack_multi(sprite_table)
    });

    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "texture", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "set_texture", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "palette", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "set_palette", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "create_node", None);

    lua_api.add_dynamic_function(ENTITY_TABLE, "create_sync_node", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(id.into())
            .map_err(|_| entity_not_found())?;

        let sprite_tree = simulation
            .sprite_trees
            .get_mut(entity.sprite_tree_index)
            .ok_or_else(sprite_not_found)?;

        let sprite_index =
            sprite_tree.insert_root_child(SpriteNode::new(api_ctx.game_io, SpriteColorMode::Add));

        // copy derived states
        let entity_animator = &simulation.animators[entity.animator_index];
        let derived_states = entity_animator.derived_states().to_vec();

        // create and setup the new animator
        let mut animator = BattleAnimator::new();
        animator.set_target(entity.sprite_tree_index, sprite_index);
        animator.copy_derive_states(derived_states);
        let animator_index = simulation.animators.insert(animator);

        // add the new animator to the sync list
        let entity_animator = &mut simulation.animators[entity.animator_index];
        entity_animator.add_synced_animator(animator_index);

        let sync_node_table =
            create_sync_node_table(lua, entity.sprite_tree_index, sprite_index, animator_index);

        lua.pack_multi(sync_node_table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "remove_sync_node", |api_ctx, lua, params| {
        let (table, sync_node_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;
        let animator_index: GenerationalIndex = sync_node_table.raw_get("#anim")?;

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

            if animator.target_tree_index() != Some(entity.sprite_tree_index) {
                return Err(mismatched_entity());
            }

            if sprite_index == GenerationalIndex::tree_root() {
                // prevent scripters from targeting entity.animator_index
                // we use [] to access entity.animator_index, deletion without deleting the entity would cause a crash
                return Err(invalid_sync_node());
            }

            // remove sprite
            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
                sprite_tree.remove(sprite_index);
            }

            // remove animator
            simulation.animators.remove(animator_index);
        }

        // remove animator from the sync list
        let entity_animator = &mut simulation.animators[entity.animator_index];
        entity_animator.remove_synced_animator(animator_index);

        lua.pack_multi(())
    });

    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "remove_node", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "hide", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "reveal", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "color", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "set_color", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "layer", None);
    lua_api.add_convenience_method(ENTITY_TABLE, "sprite", "set_layer", None);

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_shadow", |api_ctx, lua, params| {
        let (table, texture_path, animation_path): (rollback_mlua::Table, String, Option<String>) =
            lua.unpack_multi(params)?;

        let texture_path = absolute_path(lua, texture_path)?;
        let animation_path = animation_path
            .map(|path| absolute_path(lua, path))
            .transpose()?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        EntityShadow::set(
            api_ctx.game_io,
            simulation,
            id,
            texture_path,
            animation_path,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "show_shadow", |api_ctx, lua, params| {
        let (table, visible): (rollback_mlua::Table, Option<bool>) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        EntityShadow::set_visible(simulation, id, visible.unwrap_or(true));

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

        if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
            let root_node = sprite_tree.root_mut();
            animator.apply(root_node);
        }

        simulation.pending_callbacks.extend(callbacks);
        simulation.call_pending_callbacks(api_ctx.game_io, api_ctx.resources);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "create_component", |api_ctx, lua, params| {
        let (entity_table, lifetime): (rollback_mlua::Table, ComponentLifetime) =
            lua.unpack_multi(params)?;

        let entity_id: EntityId = entity_table.get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        if !entities.contains(entity_id.into()) {
            return Err(entity_not_found());
        };

        let component = Component::new(entity_id, lifetime);
        let index = api_ctx.simulation.components.insert(component);

        if lifetime == ComponentLifetime::Local {
            if let Ok(components) = entities.query_one_mut::<&mut LocalComponents>(entity_id.into())
            {
                components.0.push(index);
            } else {
                let components = LocalComponents(vec![index]);
                let _ = entities.insert_one(entity_id.into(), components);
            }
        }

        let table = lua.create_table()?;
        table.raw_set("#id", index)?;
        table.raw_set("#entity", entity_table)?;
        inherit_metatable(lua, COMPONENT_TABLE, &table)?;

        lua.pack_multi(table)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "context", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        if let Ok(attack_context) = entities.query_one_mut::<&AttackContext>(id.into()) {
            lua.pack_multi(attack_context)
        } else {
            lua.pack_multi(&AttackContext::new(id))
        }
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "start_context", |api_ctx, lua, params| {
        let (table, action_type): (rollback_mlua::Table, ActionTypes) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        Living::update_action_context(
            api_ctx.game_io,
            api_ctx.resources,
            simulation,
            action_type,
            id,
        );

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "has_actions", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let id: EntityId = table.raw_get("#id")?;

        // check for async or active actions
        let has_processed_action = simulation
            .actions
            .iter()
            .any(|(_, action)| action.processed && action.entity == id);

        if has_processed_action {
            return lua.pack_multi(true);
        }

        // check for pending actions
        let entities = &mut simulation.entities;
        let Ok(action_queue) = entities.query_one_mut::<&ActionQueue>(id.into()) else {
            return lua.pack_multi(false);
        };

        lua.pack_multi(!action_queue.pending.is_empty())
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "can_time_freeze_counter",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let id: EntityId = table.raw_get("#id")?;

            if !simulation.time_freeze_tracker.can_counter() {
                return lua.pack_multi(false);
            }

            let Some(polled_id) = simulation.time_freeze_tracker.polled_entity() else {
                // no one to counter?
                return lua.pack_multi(false);
            };

            if polled_id == id {
                // can't tfc self
                return lua.pack_multi(false);
            }

            let entities = &mut simulation.entities;
            let Ok(polled_entity) = entities.query_one_mut::<&Entity>(polled_id.into()) else {
                return lua.pack_multi(false);
            };

            let time_freeze_team = polled_entity.team;

            let Ok(entity) = entities.query_one_mut::<&Entity>(id.into()) else {
                return lua.pack_multi(false);
            };

            lua.pack_multi(entity.team != time_freeze_team || entity.team == Team::Other)
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "queue_action", |api_ctx, lua, params| {
        let (table, action_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let game_io = api_ctx.game_io;
        let resources = api_ctx.resources;

        let id: EntityId = table.raw_get("#id")?;
        let action_index: GenerationalIndex = action_table.raw_get("#id")?;

        let action = (simulation.actions)
            .get_mut(action_index)
            .ok_or_else(action_not_found)?;

        if action.processed {
            return Err(action_aready_processed());
        }

        if action.entity != id {
            return Err(action_entity_mismatch());
        }

        Action::queue_action(game_io, resources, simulation, id, action_index);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "cancel_actions", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let resources = api_ctx.resources;
        let simulation = &mut api_ctx.simulation;

        let id: EntityId = table.raw_get("#id")?;
        Action::cancel_all(game_io, resources, simulation, id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "cancel_movement",
        move |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut *api_ctx.simulation;
            let entities = &mut simulation.entities;

            let (_, living) = entities
                .query_one_mut::<(&Entity, Option<&Living>)>(id.into())
                .map_err(|_| entity_not_found())?;

            let dragged = living
                .map(|living| living.status_director.is_dragged())
                .unwrap_or_default();

            if dragged {
                return lua.pack_multi(());
            }

            Movement::cancel(simulation, id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "can_move_to", move |api_ctx, lua, params| {
        let (table, tile_table): (rollback_mlua::Table, Option<rollback_mlua::Table>) =
            lua.unpack_multi(params)?;

        let Some(tile_table) = tile_table else {
            return lua.pack_multi(false);
        };

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;

        let id: EntityId = table.raw_get("#id")?;
        let dest = (tile_table.raw_get("#x")?, tile_table.raw_get("#y")?);

        if !simulation.field.in_bounds(dest) {
            return lua.pack_multi(false);
        }

        let can_move =
            Entity::can_move_to(api_ctx.game_io, api_ctx.resources, simulation, id, dest);

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
    getter::<Option<&Movement>, _>(
        lua_api,
        "is_sliding",
        |movement: Option<&Movement>, lua, _: ()| {
            lua.pack_multi(
                movement
                    .map(|action| action.is_sliding())
                    .unwrap_or_default(),
            )
        },
    );
    getter::<Option<&Movement>, _>(
        lua_api,
        "is_jumping",
        |movement: Option<&Movement>, lua, _: ()| {
            lua.pack_multi(
                movement
                    .map(|action| action.is_jumping())
                    .unwrap_or_default(),
            )
        },
    );
    getter::<Option<&Movement>, _>(
        lua_api,
        "is_teleporting",
        |movement: Option<&Movement>, lua, _: ()| {
            lua.pack_multi(
                movement
                    .map(|action| action.is_teleporting())
                    .unwrap_or_default(),
            )
        },
    );
    getter::<Option<&Movement>, _>(
        lua_api,
        "is_moving",
        |movement: Option<&Movement>, lua, _: ()| lua.pack_multi(movement.is_some()),
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "is_dragged", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let Ok(living) = entities.query_one_mut::<&Living>(id.into()) else {
            return lua.pack_multi(false);
        };

        lua.pack_multi(living.status_director.is_dragged())
    });

    delete_getter(lua_api, "deleted", |entity: &Entity| entity.deleted);
    delete_getter(lua_api, "will_erase_eof", |entity: &Entity| entity.erased);

    lua_api.add_dynamic_function(ENTITY_TABLE, "erase", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        Entity::mark_erased(api_ctx.game_io, api_ctx.resources, simulation, id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "delete", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        Entity::delete(api_ctx.game_io, api_ctx.resources, simulation, id);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "default_player_delete",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();

            let scripts = &api_ctx.resources.vm_manager.scripts;
            scripts.default_player_delete.call(
                api_ctx.game_io,
                api_ctx.resources,
                api_ctx.simulation,
                id,
            );

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

            let scripts = &api_ctx.resources.vm_manager.scripts;
            scripts.default_character_delete.call(
                api_ctx.game_io,
                api_ctx.resources,
                api_ctx.simulation,
                (id, explosion_count),
            );

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "on_delete", |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, rollback_mlua::Function) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        if !entities.contains(id.into()) {
            return Err(entity_not_found());
        };

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

        if let Ok(callbacks) = entities.query_one_mut::<&mut DeleteCallbacks>(id.into()) {
            callbacks.0.push(callback);
        } else {
            let _ = entities.insert_one(id.into(), DeleteCallbacks(vec![callback]));
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_idle", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let resources = api_ctx.resources;
        let simulation = &mut api_ctx.simulation;

        let entities = &mut simulation.entities;
        if let Ok(callback) = entities.query_one_mut::<&mut IdleCallback>(id.into()) {
            let callback = callback.0.clone();
            callback.call(game_io, resources, simulation, ());
        }

        lua.pack_multi(())
    });

    callback_setter(lua_api, SPAWN_FN, SpawnCallback, |lua, table, _| {
        lua.pack_multi(table)
    });

    callback_setter(lua_api, UPDATE_FN, UpdateCallback, |lua, table, _| {
        lua.pack_multi(table)
    });

    callback_setter(lua_api, IDLE_FN, IdleCallback, |lua, table, _| {
        lua.pack_multi(table)
    });

    callback_setter(lua_api, COUNTER_FN, CounterCallback, |lua, table, _| {
        lua.pack_multi(table)
    });

    callback_setter(lua_api, DELETE_FN, DeleteCallback, |lua, table, _| {
        lua.pack_multi(table)
    });

    callback_setter(
        lua_api,
        BATTLE_START_FN,
        BattleStartCallback,
        |lua, table, _| lua.pack_multi(table),
    );

    callback_setter(
        lua_api,
        BATTLE_END_FN,
        BattleEndCallback,
        |lua, table, won| lua.pack_multi((table, won)),
    );

    callback_setter(
        lua_api,
        CAN_MOVE_TO_FN,
        CanMoveToCallback,
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

        let vms = api_ctx.resources.vm_manager.vms();
        let vm = &vms[api_ctx.vm_index];

        if !vm.permitted_dependencies.contains(&package_id) {
            return Err(unmarked_dependency());
        }

        let namespace = vm.preferred_namespace();
        let id = Character::load(
            api_ctx.game_io,
            api_ctx.resources,
            api_ctx.simulation,
            &package_id,
            namespace,
            rank,
        )?;

        let entities = &mut api_ctx.simulation.entities;
        let entity = entities.query_one_mut::<&mut Entity>(id.into()).unwrap();
        entity.team = team;

        lua.pack_multi(create_entity_table(lua, id))
    });

    getter::<&Character, _>(
        lua_api,
        "field_cards",
        |character: &Character, lua, _: ()| {
            let card_iter = character.cards.iter().rev();
            let indexed_iter = card_iter.enumerate().map(|(i, v)| (i + 1, v));
            let table = lua.create_table_from(indexed_iter);

            lua.pack_multi(table)
        },
    );

    getter::<&Character, _>(
        lua_api,
        "field_card",
        |character: &Character, lua, index: isize| {
            // accepting index as isize to prevent type errors when we can just return nil
            let index = character.invert_card_index((index - 1) as usize);
            let card = character.cards.get(index);

            lua.pack_multi(card)
        },
    );

    setter(
        lua_api,
        "set_field_card",
        |character: &mut Character, _, (index, card): (isize, CardProperties)| {
            // accepting index as isize to prevent type errors when we can just return nil
            let index = character.invert_card_index((index - 1) as usize);

            let Some(card_ref) = character.cards.get_mut(index) else {
                return Ok(());
            };

            let should_restart_mutations = card_ref.package_id != card.package_id;

            *card_ref = card;

            if should_restart_mutations {
                character.next_card_mutation = Some(0);
            }

            Ok(())
        },
    );

    setter(
        lua_api,
        "remove_field_card",
        |character: &mut Character, _, reversed_index: isize| {
            // accepting index as isize to prevent type errors when we can just return nil
            let reversed_index = (reversed_index - 1) as usize;

            let usable_index = character.invert_card_index(reversed_index);

            // drop the card
            if character.cards.get(usable_index).is_some() {
                character.cards.remove(usable_index);
            }

            // cancel card use if we dropped the card
            if reversed_index == 0 {
                character.card_use_requested = false;
            }

            // fix next_card_mutation as indices were shifted
            if let Some(next_index) = &mut character.next_card_mutation
                && *next_index == reversed_index + 1
            {
                *next_index = reversed_index;
            }

            Ok(())
        },
    );

    setter(
        lua_api,
        "insert_field_card",
        |character: &mut Character, _, (index, card): (isize, CardProperties)| {
            // accepting index as isize to prevent type errors when we can just return nil
            let index = if (index as usize) < character.cards.len() {
                character.invert_card_index((index - 1) as usize)
            } else {
                0
            };

            if index > character.cards.len() {
                character.cards.push(card);
            } else {
                character.cards.insert(index, card);
            }

            character.next_card_mutation = Some(0);

            Ok(())
        },
    );

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

    //     lua.pack_multi(api_ctx.simulation.is_entity_actionable(entity_id))
    // });

    getter::<&Character, _>(lua_api, "rank", |character: &Character, lua, _: ()| {
        lua.pack_multi(character.rank)
    });

    callback_setter(lua_api, INTRO_FN, IntroCallback, |lua, table, _| {
        lua.pack_multi(table)
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

        let middle_param = rest.pop_front().unwrap_or(rollback_mlua::Value::Nil);

        let secondary_element: Element;
        let context: Option<AttackContext>;

        match lua.unpack(middle_param.clone()) {
            Ok(element) => {
                secondary_element = element;
                context = if let Some(context_value) = rest.pop_front() {
                    lua.unpack::<Option<AttackContext>>(context_value)?
                } else {
                    None
                }
            }
            Err(_) => {
                secondary_element = Element::None;
                context = lua.unpack(middle_param)?;
            }
        };

        let drag = lua.unpack_multi::<Option<Drag>>(rest)?.unwrap_or_default();

        lua.pack_multi(HitProperties {
            damage,
            flags,
            durations: Default::default(),
            element,
            secondary_element,
            drag,
            context: context.unwrap_or_default(),
        })
    });

    lua_api.add_dynamic_function(HIT_PROPS_TABLE, "from_card", |_, lua, params| {
        let (card_properties, context, drag): (
            CardProperties,
            Option<AttackContext>,
            Option<Drag>,
        ) = lua.unpack_multi(params)?;

        lua.pack_multi(HitProperties {
            damage: card_properties.damage,
            flags: card_properties.hit_flags,
            durations: card_properties.status_durations,
            element: card_properties.element,
            secondary_element: card_properties.secondary_element,
            drag: drag.unwrap_or_default(),
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

    getter::<&Spell, _>(lua_api, "copy_hit_props", |spell: &Spell, lua, _: ()| {
        lua.pack_multi(&spell.hit_props)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_hit_props", |api_ctx, lua, params| {
        let (table, props): (rollback_mlua::Table, HitProperties) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let spell = entities
            .query_one_mut::<&mut Spell>(id.into())
            .map_err(|_| entity_not_found())?;

        // copy context into entity for entity:context()
        let attack_context = props.context.clone();

        spell.hit_props = props;

        let _ = entities.insert_one(id.into(), attack_context);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "attack_tile", |api_ctx, lua, params| {
        let (table, tile_table): (rollback_mlua::Table, Option<rollback_mlua::Table>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut *api_ctx.simulation;

        // resolving tile, and query for Spell to generate errors
        let entities = &mut simulation.entities;
        let (entity, _) = entities
            .query_one_mut::<(&mut Entity, &mut Spell)>(id.into())
            .map_err(|_| entity_not_found())?;

        let (x, y): (i32, i32) = match tile_table {
            Some(table) => (table.raw_get("#x")?, table.raw_get("#y")?),
            None => (entity.x, entity.y),
        };

        Spell::attack_tile(simulation, id, x, y);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "attack_tiles", |api_ctx, lua, params| {
        let (table, tiles): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut *api_ctx.simulation;

        // query the entity for the error
        let entities = &mut simulation.entities;
        entities
            .query_one_mut::<(&mut Entity, &mut Spell)>(id.into())
            .map_err(|_| entity_not_found())?;

        for tile_table in tiles.sequence_values::<rollback_mlua::Table>() {
            let tile_table = tile_table?;
            let x: i32 = tile_table.raw_get("#x")?;
            let y: i32 = tile_table.raw_get("#y")?;

            Spell::attack_tile(simulation, id, x, y);
        }

        lua.pack_multi(())
    });

    callback_setter(lua_api, ATTACK_FN, AttackCallback, |lua, table, id| {
        let other_table = create_entity_table(lua, id);
        lua.pack_multi((table, other_table))
    });

    callback_setter(
        lua_api,
        COLLISION_FN,
        CollisionCallback,
        |lua, table, id| {
            let other_table = create_entity_table(lua, id);
            lua.pack_multi((table, other_table))
        },
    );
}

fn inject_living_api(lua_api: &mut BattleLuaApi) {
    getter::<&Living, _>(lua_api, "max_health", |living: &Living, lua, _: ()| {
        lua.pack_multi(living.max_health)
    });
    getter::<&Living, _>(lua_api, "health", |living: &Living, lua, _: ()| {
        lua.pack_multi(living.health)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "set_health", |api_ctx, lua, params| {
        let (table, health): (rollback_mlua::Table, i32) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut *api_ctx.simulation;

        // query the entity for the error
        let entities = &mut simulation.entities;
        let living = entities
            .query_one_mut::<&mut Living>(id.into())
            .map_err(|_| entity_not_found())?;

        let initializing_health = living.max_health == 0;
        let prev_health = living.health;

        living.set_health(health);

        // track changes
        if initializing_health {
            return lua.pack_multi(());
        }

        let hp_diff = living.health - prev_health;
        HpChanges::track_direct_change(simulation, id, hp_diff);

        lua.pack_multi(())
    });

    setter(
        lua_api,
        "hit",
        |living: &mut Living, _, hit_props: HitProperties| {
            living.queue_hit(None, hit_props);
            Ok(())
        },
    );

    getter::<&Living, _>(lua_api, "hitbox_enabled", |living: &Living, lua, _: ()| {
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

    getter::<&Living, _>(lua_api, "counterable", |living: &Living, lua, _: ()| {
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

    getter::<&Living, _>(lua_api, "intangible", |living: &Living, lua, _: ()| {
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

    lua_api.add_dynamic_function(ENTITY_TABLE, "has_defense", |api_ctx, lua, params| {
        let (table, priority): (rollback_mlua::Table, DefensePriority) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let has_priority = DefenseRule::has_priority(api_ctx.simulation, id, priority);

        lua.pack_multi(has_priority)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "remove_defense", |api_ctx, lua, params| {
        let (table, priority): (rollback_mlua::Table, DefensePriority) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        DefenseRule::remove_priority(
            api_ctx.game_io,
            api_ctx.resources,
            api_ctx.simulation,
            id,
            priority,
        );

        lua.pack_multi(())
    });

    getter::<&Living, _>(
        lua_api,
        "remaining_status_time",
        |living: &Living, lua, hit_flag: HitFlags| {
            lua.pack_multi(living.status_director.remaining_status_time(hit_flag))
        },
    );

    setter(
        lua_api,
        "set_remaining_status_time",
        |living: &mut Living, _, (hit_flag, duration): (HitFlags, FrameTime)| {
            living
                .status_director
                .set_remaining_status_time(hit_flag, duration);
            Ok(())
        },
    );

    setter(
        lua_api,
        "apply_status",
        |living: &mut Living, _, (hit_flag, duration): (HitFlags, FrameTime)| {
            living.status_director.apply_status(hit_flag, duration);
            Ok(())
        },
    );

    setter(
        lua_api,
        "remove_status",
        |living: &mut Living, _, hit_flag: HitFlags| {
            living.status_director.remove_statuses(hit_flag);
            Ok(())
        },
    );

    getter::<&Living, _>(
        lua_api,
        "status_immunities",
        |living: &Living, lua, _: ()| lua.pack_multi(living.status_director.immunities()),
    );

    setter(
        lua_api,
        "register_status_callback",
        |living: &mut Living, _, (hit_flag, callback): (HitFlags, BattleCallback)| {
            living.register_status_callback(hit_flag, callback);
            Ok(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "is_inactionable", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let status_registry = &api_ctx.resources.status_registry;
        let entities = &mut api_ctx.simulation.entities;

        let living = entities
            .query_one_mut::<&mut Living>(id.into())
            .map_err(|_| entity_not_found())?;

        lua.pack_multi(living.status_director.is_inactionable(status_registry))
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "is_immobile", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let status_registry = &api_ctx.resources.status_registry;
        let entities = &mut api_ctx.simulation.entities;

        let living = entities
            .query_one_mut::<&mut Living>(id.into())
            .map_err(|_| entity_not_found())?;

        lua.pack_multi(living.status_director.is_immobile(status_registry))
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "add_aux_prop", |api_ctx, lua, params| {
        let (table, value): (rollback_mlua::Table, rollback_mlua::Value) =
            lua.unpack_multi(params)?;

        // find entity
        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let resources = api_ctx.resources;

        let living = entities
            .query_one_mut::<&mut Living>(id.into())
            .map_err(|_| entity_not_found())?;

        // add aux prop
        let aux_prop_table: rollback_mlua::Table = lua.unpack(value.clone())?;

        if aux_prop_table.contains_key("#id")? {
            return Err(aux_prop_already_bound());
        }

        let aux_prop = AuxProp::from_lua(resources, lua, lua.unpack(value)?)?;
        let id = living.add_aux_prop(aux_prop);
        aux_prop_table.raw_set("#id", id)?;

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "remove_aux_prop", |api_ctx, lua, params| {
        let (table, value): (rollback_mlua::Table, rollback_mlua::Value) =
            lua.unpack_multi(params)?;

        // find entity
        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let living = entities
            .query_one_mut::<&mut Living>(id.into())
            .map_err(|_| entity_not_found())?;

        // remove aux prop
        let aux_prop_table: rollback_mlua::Table = lua.unpack(value)?;
        let id: GenerationalIndex = aux_prop_table.raw_get("#id")?;
        living.aux_props.remove(id);
        aux_prop_table.raw_remove("#id")?;

        lua.pack_multi(())
    });

    callback_setter(lua_api, COUNTERED_FN, CounteredCallback, |lua, table, _| {
        lua.pack_multi(table)
    });
}

fn inject_player_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(ENTITY_TABLE, "is_local", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        lua.pack_multi(player.index == simulation.local_player_index)
    });

    getter::<&Player, _>(lua_api, "player_index", |player: &Player, lua, ()| {
        lua.pack_multi(player.index)
    });

    getter::<&EmotionWindow, _>(
        lua_api,
        "emotions",
        |emotion_window: &EmotionWindow, lua, ()| {
            let emotions = emotion_window.emotions();
            let table = lua.create_table_from(emotions.enumerate().map(|(i, v)| (i + 1, v)))?;

            lua.pack_multi(table)
        },
    );
    getter::<&EmotionWindow, _>(
        lua_api,
        "emotion",
        |emotion_window: &EmotionWindow, lua, ()| lua.pack_multi(emotion_window.emotion()),
    );
    setter(
        lua_api,
        "set_emotion",
        |emotion_window: &mut EmotionWindow, _lua, emotion: Emotion| {
            emotion_window.set_emotion(emotion);
            Ok(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "emotions_texture", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let emotion_window = entities
            .query_one_mut::<&EmotionWindow>(id.into())
            .map_err(|_| entity_not_found())?;

        let tree_index = emotion_window.sprite_tree_index();

        if let Some(tree) = simulation.sprite_trees.get(tree_index) {
            lua.pack_multi(tree.root().texture_path())
        } else {
            lua.pack_multi("")
        }
    });

    getter::<&EmotionWindow, _>(
        lua_api,
        "emotions_animation_path",
        |emotion_window: &EmotionWindow, lua, _: ()| {
            lua.pack_multi(emotion_window.animation_path())
        },
    );

    getter::<&EmotionWindow, _>(
        lua_api,
        "emotion_node",
        |emotion_window: &EmotionWindow, lua, _: ()| {
            let table = create_sprite_table(
                lua,
                emotion_window.sprite_tree_index(),
                GenerationalIndex::tree_root(),
                None,
            )?;

            lua.pack_multi(table)
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "set_emotions_texture",
        |api_ctx, lua, params| {
            let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
            let path = absolute_path(lua, path)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let emotion_window = entities
                .query_one_mut::<&EmotionWindow>(id.into())
                .map_err(|_| entity_not_found())?;

            let tree_index = emotion_window.sprite_tree_index();

            if let Some(tree) = simulation.sprite_trees.get_mut(tree_index) {
                let sprite = tree.root_mut();
                sprite.set_texture(api_ctx.game_io, path);
                sprite.apply_animation(emotion_window.animator());
            }

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "load_emotions_animation",
        |api_ctx, lua, params| {
            let (table, path): (rollback_mlua::Table, String) = lua.unpack_multi(params)?;
            let path = absolute_path(lua, path)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let emotion_window = entities
                .query_one_mut::<&mut EmotionWindow>(id.into())
                .map_err(|_| entity_not_found())?;

            emotion_window.load_animation(api_ctx.game_io, path);

            let tree_index = emotion_window.sprite_tree_index();

            if let Some(tree) = simulation.sprite_trees.get_mut(tree_index) {
                tree.root_mut().apply_animation(emotion_window.animator());
            }

            lua.pack_multi(())
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
                InputQuery::Pulsed(input) => player_input.pulsed(input),
                InputQuery::Held(input) => player_input.is_down(input),
            };

            lua.pack_multi(result)
        } else {
            lua.pack_multi(false)
        }
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "input_delay", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let delay = entities
            .query_one_mut::<&mut Player>(id.into())
            .map(|player| {
                let player_input = &simulation.inputs[player.index];
                player_input.input_delay()
            })
            .unwrap_or_default();

        lua.pack_multi(delay)
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "remember", |api_ctx, lua, params| {
        let (table, key, value): (rollback_mlua::Table, String, rollback_mlua::Value) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let v = match value {
            rollback_mlua::Value::Integer(v) => v.into(),
            rollback_mlua::Value::Number(v) => v.into(),
            rollback_mlua::Value::String(v) => v.to_str()?.to_string().into(),
            _ => return Err(invalid_memory_value()),
        };

        if let Some(memories) = simulation.memories.get_mut(&player.index) {
            memories.update(|memories| {
                memories.insert(key, v);
            });
        } else {
            let memories = EntityMemories::new([(key, v)].into());
            simulation.memories.insert(player.index, memories);
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "forget", |api_ctx, lua, params| {
        let (table, key): (rollback_mlua::Table, rollback_mlua::String) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        if let Some(memories) = simulation.memories.get_mut(&player.index) {
            let key = key.to_str()?;
            memories.update(|memories| {
                memories.remove(key);
            });
        }

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ENTITY_TABLE, "recall", |api_ctx, lua, params| {
        let (table, key): (rollback_mlua::Table, rollback_mlua::String) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let player = entities
            .query_one_mut::<&mut Player>(id.into())
            .map_err(|_| entity_not_found())?;

        let Some(memories) = simulation.memories.get_mut(&player.index) else {
            return lua.pack_multi(());
        };

        memories.read(|memories| {
            let Some(value) = memories.get(key.to_str()?) else {
                return lua.pack_multi(());
            };

            match value {
                MemoryCell::Integer(v) => lua.pack_multi(*v),
                MemoryCell::Float(v) => lua.pack_multi(*v),
                MemoryCell::String(v) => lua.pack_multi(lua.create_string(v)?),
            }
        })
    });

    // player:play_audio() is defined in resource_api.rs

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

            player.attack_charge.set_color(color.into());

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(ENTITY_TABLE, "charge_position", |api_ctx, lua, params| {
        let table: rollback_mlua::Table = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        let (entity, player) = entities
            .query_one_mut::<(&mut Entity, &Player)>(id.into())
            .map_err(|_| entity_not_found())?;

        let offset = simulation
            .sprite_trees
            .get(entity.sprite_tree_index)
            .map(|sprite_tree| player.card_charge.sprite_offset(sprite_tree))
            .unwrap_or_default();

        lua.pack_multi(LuaVector::from(offset))
    });

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

            if let Some(sprite_tree) = simulation.sprite_trees.get_mut(entity.sprite_tree_index) {
                let offset = Vec2::new(x, y);

                player.card_charge.update_sprite_offset(sprite_tree, offset);
                (player.attack_charge).update_sprite_offset(sprite_tree, offset);
            }

            lua.pack_multi(())
        },
    );

    getter::<&Player, _>(
        lua_api,
        "slide_when_moving",
        |player: &Player, lua, _: ()| lua.pack_multi(player.slide_when_moving),
    );

    setter(
        lua_api,
        "set_slide_when_moving",
        |player: &mut Player, _, slide: Option<bool>| {
            player.slide_when_moving = slide.unwrap_or(true);
            Ok(())
        },
    );

    getter::<&Player, _>(
        lua_api,
        "movement_on_input",
        |player: &Player, lua, _: ()| lua.pack_multi(player.movement_on_input()),
    );

    setter(
        lua_api,
        "set_movement_on_input",
        |player: &mut Player, _, value: Option<bool>| {
            player.overridables.flags.set_movement_on_input(value);
            Ok(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "queue_default_player_movement",
        |api_ctx, lua, params| {
            let (table, tile_table): (rollback_mlua::Table, Option<rollback_mlua::Table>) =
                lua.unpack_multi(params)?;

            let Some(tile_table) = tile_table else {
                return lua.pack_multi(());
            };

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;

            let Ok(tile) = tile_mut_from_table(&mut simulation.field, tile_table) else {
                return lua.pack_multi(());
            };

            let tile_position = tile.position();

            let scripts = &api_ctx.resources.vm_manager.scripts;
            scripts.queue_default_player_movement.call(
                api_ctx.game_io,
                api_ctx.resources,
                api_ctx.simulation,
                (id, tile_position),
            );

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "queue_normal_attack",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let id: EntityId = table.raw_get("#id")?;

            Player::use_normal_attack(api_ctx.game_io, api_ctx.resources, simulation, id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "queue_charged_attack",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let id: EntityId = table.raw_get("#id")?;

            Player::use_charged_attack(api_ctx.game_io, api_ctx.resources, simulation, id);

            lua.pack_multi(())
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "queue_special_attack",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let id: EntityId = table.raw_get("#id")?;

            Player::use_special_attack(api_ctx.game_io, api_ctx.resources, simulation, id);

            lua.pack_multi(())
        },
    );

    setter(
        lua_api,
        "set_special_on_input",
        |player: &mut Player, _, value: Option<bool>| {
            player.overridables.flags.set_special_on_input(value);
            Ok(())
        },
    );

    getter::<&Player, _>(
        lua_api,
        "special_on_input",
        |player: &Player, lua, _: ()| lua.pack_multi(player.special_on_input()),
    );

    getter::<&PlayerHand, _>(
        lua_api,
        "has_regular_card",
        |hand: &PlayerHand, lua, _: ()| lua.pack_multi(hand.has_regular_card),
    );

    getter::<&PlayerHand, _>(lua_api, "deck_cards", |hand: &PlayerHand, lua, _: ()| {
        let table = lua.create_table_with_capacity(hand.deck.len(), 0)?;

        for tuple in &hand.deck {
            table.raw_push(create_deck_card_table(lua, tuple)?)?;
        }

        lua.pack_multi(table)
    });

    getter::<&PlayerHand, _>(
        lua_api,
        "deck_card",
        |hand: &PlayerHand, lua, index: usize| {
            let index = index.saturating_sub(1);
            let card_table = hand
                .deck
                .get(index)
                .map(|tuple| create_deck_card_table(lua, tuple))
                .transpose()?;

            lua.pack_multi(card_table)
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "deck_card_properties",
        |api_ctx, lua, params| {
            let (table, index): (rollback_mlua::Table, usize) = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let hand = entities
                .query_one_mut::<&mut PlayerHand>(id.into())
                .map_err(|_| entity_not_found())?;

            let index = index.saturating_sub(1);
            let Some((card, namespace)) = hand.deck.get(index) else {
                return lua.pack_multi(());
            };
            let ns = *namespace;

            let globals = Globals::from_resources(api_ctx.game_io);
            let card_packages = &globals.card_packages;

            let mut card_properties =
                if let Some(package) = card_packages.package_or_fallback(ns, &card.package_id) {
                    let status_registry = &api_ctx.resources.status_registry;
                    package.card_properties.to_bindable(status_registry)
                } else {
                    CardProperties::default()
                };

            card_properties.code = card.code.clone();
            card_properties.namespace = Some(ns);

            lua.pack_multi(card_properties)
        },
    );

    add_entity_query_fn::<(&mut PlayerHand, &PackageNamespace)>(
        lua_api,
        "set_deck_card",
        |(hand, default_ns), lua, _, params| {
            let (index, card_value): (usize, _) = lua.unpack_multi(params)?;
            let (card, namespace) = read_deck_card_table(card_value)?;
            let index = index.saturating_sub(1);

            if let Some(card_ref) = hand.deck.get_mut(index) {
                *card_ref = (card, namespace.unwrap_or(*default_ns));
            }

            lua.pack_multi(())
        },
    );

    setter(
        lua_api,
        "remove_deck_card",
        |hand: &mut PlayerHand, _, index: usize| {
            let index = index.saturating_sub(1);

            if hand.deck.get(index).is_none() {
                return Ok(());
            }

            hand.deck.remove(index);
            hand.staged_items.handle_deck_index_removed(index);

            if index == 0 {
                hand.has_regular_card = false;
            }

            Ok(())
        },
    );

    add_entity_query_fn::<(&mut PlayerHand, &PackageNamespace)>(
        lua_api,
        "insert_deck_card",
        |(hand, default_ns), lua, _, params| {
            let (index, card_value): (usize, _) = lua.unpack_multi(params)?;
            let (card, namespace) = read_deck_card_table(card_value)?;
            let card_tuple = (card, namespace.unwrap_or(*default_ns));

            let index = index.saturating_sub(1);

            if index > hand.deck.len() {
                hand.deck.push(card_tuple);
            } else {
                if index == 0 {
                    hand.has_regular_card = false;
                }

                hand.deck.insert(index, card_tuple);
                hand.staged_items.handle_deck_index_inserted(index);
            }

            lua.pack_multi(())
        },
    );

    // card_select_api.rs

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "create_card_button",
        |api_ctx, lua, params| {
            let (table, slot_width, button_slot): (rollback_mlua::Table, usize, Option<usize>) =
                lua.unpack_multi(params)?;
            let entity_id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;

            let button_path = CardSelectButtonPath {
                entity_id,
                form_index: None,
                augment_index: None,
                card_button_slot: button_slot.unwrap_or_default().max(1),
            };

            let table = create_card_select_button_and_table(
                game_io,
                simulation,
                lua,
                &table,
                button_path,
                slot_width,
            )?;

            lua.pack_multi(table)
        },
    );

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "create_special_button",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;
            let entity_id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let game_io = api_ctx.game_io;
            let simulation = &mut api_ctx.simulation;

            let button_path = CardSelectButtonPath {
                entity_id,
                form_index: None,
                augment_index: None,
                card_button_slot: CardSelectButton::SPECIAL_SLOT,
            };

            let table = create_card_select_button_and_table(
                game_io,
                simulation,
                lua,
                &table,
                button_path,
                1,
            )?;

            lua.pack_multi(table)
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

        if player.available_forms().count() >= 5 {
            return Err(too_many_forms());
        }

        let index = player.forms.len();

        player.forms.push(PlayerForm::default());

        let form_table = create_player_form_table(lua, table, index)?;

        lua.pack_multi(form_table)
    });

    lua_api.add_dynamic_function(
        ENTITY_TABLE,
        "create_hidden_form",
        |api_ctx, lua, params| {
            let table: rollback_mlua::Table = lua.unpack_multi(params)?;

            let id: EntityId = table.raw_get("#id")?;

            let api_ctx = &mut *api_ctx.borrow_mut();
            let simulation = &mut api_ctx.simulation;
            let entities = &mut simulation.entities;

            let player = entities
                .query_one_mut::<&mut Player>(id.into())
                .map_err(|_| entity_not_found())?;

            let index = player.forms.len();

            let form = PlayerForm {
                hidden: true,
                ..Default::default()
            };

            player.forms.push(form);

            let form_table = create_player_form_table(lua, table, index)?;

            lua.pack_multi(form_table)
        },
    );

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

    lua_api.add_dynamic_function(ENTITY_TABLE, "augments", |api_ctx, lua, params| {
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
        let game_io = api_ctx.game_io;
        let resources = api_ctx.resources;
        let simulation = &mut api_ctx.simulation;
        let entities = &mut simulation.entities;

        // ensure player
        if entities.satisfies::<&Player>(id.into()) != Ok(true) {
            return Err(entity_not_found());
        }

        let globals = Globals::from_resources(game_io);
        let package_id = PackageId::from(augment_id);
        let vms = api_ctx.resources.vm_manager.vms();
        let namespace = vms[api_ctx.vm_index].preferred_namespace();
        let package = globals
            .augment_packages
            .package_or_fallback(namespace, &package_id)
            .ok_or_else(|| package_not_loaded(&package_id))?;

        Augment::boost(
            game_io,
            resources,
            simulation,
            id,
            package,
            namespace,
            level_boost,
        );

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

    getter::<&Player, _>(lua_api, "hand_size", |player: &Player, lua, _: ()| {
        lua.pack_multi(player.hand_size())
    });
    setter(
        lua_api,
        "boost_hand_size",
        |player: &mut Player, _, level: i8| {
            player.hand_size_boost = player.hand_size_boost.saturating_add(level);
            Ok(())
        },
    );

    getter::<&Player, _>(lua_api, "attack_level", |player: &Player, lua, _: ()| {
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

    getter::<&Player, _>(lua_api, "rapid_level", |player: &Player, lua, _: ()| {
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

    getter::<&Player, _>(lua_api, "charge_level", |player: &Player, lua, _: ()| {
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

    setter(
        lua_api,
        "set_charge_with_shoot",
        |player: &mut Player, _, charge: Option<bool>| {
            player.overridables.flags.set_charges_with_shoot(charge);
            Ok(())
        },
    );

    getter::<&Player, _>(
        lua_api,
        "charges_with_shoot",
        |player: &Player, lua, _: ()| lua.pack_multi(player.charges_with_shoot()),
    );

    setter(
        lua_api,
        "mark_charging",
        |player: &mut Player, _, charge: Option<bool>| {
            player.marked_charging = charge.unwrap_or(true);
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

            let time = PlayerOverridables::default_calculate_charge_time(level);

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
            let resources = &mut api_ctx.resources;

            let time = Player::calculate_charge_time(game_io, resources, simulation, id, level);

            lua.pack_multi(time)
        },
    );

    optional_callback_setter(
        lua_api,
        CHARGE_TIMING_FN,
        |player: &mut Player| &mut player.overridables.calculate_charge_time,
        |lua, table, _| lua.pack_multi(table),
    );

    optional_callback_setter(
        lua_api,
        NORMAL_ATTACK_FN,
        |player: &mut Player| &mut player.overridables.normal_attack,
        |lua, table, _| lua.pack_multi(table),
    );

    optional_callback_setter(
        lua_api,
        CHARGED_ATTACK_FN,
        |player: &mut Player| &mut player.overridables.charged_attack,
        |lua, table, _| lua.pack_multi(table),
    );

    optional_callback_setter(
        lua_api,
        SPECIAL_ATTACK_FN,
        |player: &mut Player| &mut player.overridables.special_attack,
        |lua, table, _| lua.pack_multi(table),
    );

    optional_callback_setter(
        lua_api,
        CARD_CHARGE_TIMING_FN,
        |player: &mut Player| &mut player.overridables.calculate_card_charge_time,
        |lua, table, card_props| lua.pack_multi((table, card_props)),
    );

    optional_callback_setter(
        lua_api,
        CHARGED_CARD_FN,
        |player: &mut Player| &mut player.overridables.charged_card,
        |lua, table, card_props| lua.pack_multi((table, card_props)),
    );

    optional_callback_setter(
        lua_api,
        MOVEMENT_INPUT_FN,
        |player: &mut Player| &mut player.overridables.movement_input,
        |lua, table, _| lua.pack_multi(table),
    );

    optional_callback_setter(
        lua_api,
        MOVEMENT_FN,
        |player: &mut Player| &mut player.overridables.movement,
        |lua, table, direction| lua.pack_multi((table, direction)),
    );
}

fn delete_getter(lua_api: &mut BattleLuaApi, name: &str, callback: fn(&Entity) -> bool) {
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

fn getter<C, P>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: for<'world, 'lua> fn(
        C::Item<'world>,
        &'lua rollback_mlua::Lua,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    C: hecs::Query + 'static,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
{
    lua_api.add_dynamic_function(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        let component = entities
            .query_one_mut::<C>(id.into())
            .map_err(|_| entity_not_found())?;

        lua.pack_multi(callback(component, lua, param)?)
    });
}

fn setter<C, P>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: fn(&mut C, &rollback_mlua::Lua, P) -> rollback_mlua::Result<()>,
) where
    C: hecs::Component,
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
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

pub fn add_entity_query_fn<Q: hecs::Query + 'static>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: for<'q, 'lua> fn(
        Q::Item<'q>,
        &'lua rollback_mlua::Lua,
        &GameIO,
        rollback_mlua::MultiValue<'lua>,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) {
    lua_api.add_dynamic_function(ENTITY_TABLE, name, move |api_ctx, lua, mut params| {
        // todo: could we produce a better error for Nil entity tables?
        let player_value = params.pop_front();
        let player_table = player_value
            .as_ref()
            .and_then(|value| value.as_table())
            .ok_or_else(entity_not_found)?;

        let id: EntityId = player_table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let game_io = api_ctx.game_io;
        let entities = &mut api_ctx.simulation.entities;

        let components = entities
            .query_one_mut::<Q>(id.into())
            .map_err(|_| entity_not_found())?;

        callback(components, lua, game_io, params)
    });
}

fn callback_setter<C, P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_constructor: fn(BattleCallback<P, R>) -> C,
    param_transformer: for<'lua> fn(
        &'lua rollback_mlua::Lua,
        rollback_mlua::Table<'lua>,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    C: hecs::Component,
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    lua_api.add_dynamic_setter(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;

        if let Some(callback) = callback {
            let key = lua.create_registry_value(table)?;

            let callback = BattleCallback::new_transformed_lua_callback(
                lua,
                api_ctx.vm_index,
                callback,
                move |_, lua, p| {
                    let table: rollback_mlua::Table = lua.registry_value(&key)?;
                    param_transformer(lua, table, p)
                },
            )?;
            let _ = entities.insert_one(id.into(), callback_constructor(callback));
        } else {
            let _ = entities.remove_one::<C>(id.into());
        };

        lua.pack_multi(())
    });
}

fn optional_callback_setter<C, P, R>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback_getter: fn(&mut C) -> &mut Option<BattleCallback<P, R>>,
    param_transformer: for<'lua> fn(
        &'lua rollback_mlua::Lua,
        rollback_mlua::Table<'lua>,
        P,
    ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>,
) where
    C: hecs::Component,
    P: for<'lua> rollback_mlua::IntoLuaMulti<'lua> + 'static,
    R: for<'lua> rollback_mlua::FromLuaMulti<'lua> + Default + Send + Sync + Clone + 'static,
{
    lua_api.add_dynamic_setter(ENTITY_TABLE, name, move |api_ctx, lua, params| {
        let (table, callback): (rollback_mlua::Table, Option<rollback_mlua::Function>) =
            lua.unpack_multi(params)?;

        let id: EntityId = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let entities = &mut api_ctx.simulation.entities;
        let Ok(entity) = entities.query_one_mut::<&mut C>(id.into()) else {
            return lua.pack_multi(());
        };

        let key = lua.create_registry_value(table)?;

        *callback_getter(entity) = callback
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

fn movement_function<P>(
    lua_api: &mut BattleLuaApi,
    name: &str,
    callback: fn((i32, i32), P) -> Movement,
) where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua> + 'static,
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
            movement.begin_callback = Some(BattleCallback::new_lua_callback(
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

    let (_, existing_movement) = entities
        .query_one_mut::<(&Entity, Option<&Movement>)>(id.into())
        .map_err(|_| entity_not_found())?;

    if existing_movement.is_some() {
        return lua.pack_multi(false);
    }

    if !Entity::can_move_to(
        api_ctx.game_io,
        api_ctx.resources,
        simulation,
        id,
        movement.dest,
    ) {
        return lua.pack_multi(false);
    }

    simulation.entities.insert_one(id.into(), movement).unwrap();

    lua.pack_multi(true)
}

fn generate_constructor_fn(
    lua_api: &mut BattleLuaApi,
    table_name: &str,
    constructor: fn(&mut BattleScriptContext) -> rollback_mlua::Result<EntityId>,
) {
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

        if entities.satisfies::<Q>(id.into()).unwrap_or_default() {
            lua.pack_multi(table)
        } else {
            lua.pack_multi(())
        }
    });
}

pub fn create_entity_table(
    lua: &rollback_mlua::Lua,
    id: EntityId,
) -> rollback_mlua::Result<rollback_mlua::Table<'_>> {
    let table = lua.create_table()?;
    inherit_metatable(lua, ENTITY_TABLE, &table)?;

    table.raw_set("#id", id)?;

    Ok(table)
}

fn create_deck_card_table<'lua>(
    lua: &'lua rollback_mlua::Lua,
    (card, namespace): &(Card, PackageNamespace),
) -> rollback_mlua::Result<rollback_mlua::Table<'lua>> {
    let table = lua.create_table()?;
    table.set("package_id", card.package_id.as_str())?;
    table.set("code", card.code.as_str())?;
    table.set("namespace", *namespace)?;

    Ok(table)
}

fn read_deck_card_table(
    lua_value: rollback_mlua::Value,
) -> rollback_mlua::Result<(Card, Option<PackageNamespace>)> {
    let table = match lua_value {
        rollback_mlua::Value::Table(table) => table,
        _ => {
            return Err(rollback_mlua::Error::FromLuaConversionError {
                from: lua_value.type_name(),
                to: "DeckCard",
                message: None,
            });
        }
    };

    Ok((
        Card {
            package_id: table.get("package_id").unwrap_or_default(),
            code: table.get("code").unwrap_or_default(),
        },
        table.get("namespace").unwrap_or_default(),
    ))
}
