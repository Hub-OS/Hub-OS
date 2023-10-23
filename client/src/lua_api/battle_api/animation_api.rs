use super::errors::{animator_not_found, sprite_not_found};
use super::{BattleLuaApi, ANIMATION_TABLE};
use crate::battle::{BattleAnimator, BattleCallback};
use crate::bindable::{GenerationalIndex, LuaVector};
use crate::lua_api::helpers::{absolute_path, inherit_metatable};
use crate::render::{DerivedFrame, FrameTime};
use crate::structures::TreeIndex;
use framework::prelude::GameIO;

pub fn inject_animation_api(lua_api: &mut BattleLuaApi) {
    lua_api.add_dynamic_function(ANIMATION_TABLE, "new", move |api_ctx, lua, params| {
        let path: Option<String> = lua.unpack_multi(params)?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let animators = &mut simulation.animators;

        let mut animator = BattleAnimator::new();

        if let Some(path) = path {
            let path = absolute_path(lua, path)?;
            let _ = animator.load(api_ctx.game_io, &path);
        }

        let index = animators.insert(animator);

        let table = lua.create_table()?;
        table.raw_set("#id", index)?;
        inherit_metatable(lua, ANIMATION_TABLE, &table)?;

        lua.pack_multi(table)
    });

    lua_api.add_dynamic_function(ANIMATION_TABLE, "copy_from", move |api_ctx, lua, params| {
        let (table, other_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;
        let other_id: GenerationalIndex = other_table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let animators = &mut simulation.animators;
        let [animator, other_animator] = animators
            .get_disjoint_mut([id, other_id])
            .ok_or_else(animator_not_found)?;

        let callbacks = animator.copy_from(other_animator);
        animator.find_and_apply_to_target(&mut simulation.sprite_trees);

        simulation.pending_callbacks.extend(callbacks);
        simulation.call_pending_callbacks(api_ctx.game_io, api_ctx.resources);

        lua.pack_multi(())
    });

    lua_api.add_dynamic_function(ANIMATION_TABLE, "apply", move |api_ctx, lua, params| {
        let (table, sprite_table): (rollback_mlua::Table, rollback_mlua::Table) =
            lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let animators = &mut simulation.animators;
        let animator = animators.get_mut(id).ok_or_else(animator_not_found)?;

        let slot_index: GenerationalIndex = sprite_table.raw_get("#tree")?;
        let sprite_index: TreeIndex = sprite_table.raw_get("#sprite")?;

        let sprite_tree = simulation
            .sprite_trees
            .get_mut(slot_index)
            .ok_or_else(sprite_not_found)?;

        let sprite_node = sprite_tree
            .get_mut(sprite_index)
            .ok_or_else(sprite_not_found)?;

        animator.apply(sprite_node);

        lua.pack_multi(())
    });

    updater(lua_api, "load", |animator, game_io, lua, path: String| {
        let path = absolute_path(lua, path)?;
        Ok(animator.load(game_io, &path))
    });

    updater(lua_api, "update", |animator, _, _, _: ()| {
        Ok(animator.update())
    });

    updater(lua_api, "sync_time", |animator, _, _, time: FrameTime| {
        Ok(animator.sync_time(time))
    });

    setter(lua_api, "pause", |animator, lua, _: ()| {
        animator.disable();
        lua.pack_multi(())
    });
    setter(lua_api, "resume", |animator, lua, _: ()| {
        animator.enable();
        lua.pack_multi(())
    });

    getter(lua_api, "completed", |animator, lua, _: ()| {
        lua.pack_multi(animator.is_complete())
    });

    getter(lua_api, "has_state", |animator, lua, state: String| {
        lua.pack_multi(animator.has_state(&state))
    });

    getter(lua_api, "state", |animator, lua, _: ()| {
        lua.pack_multi(animator.current_state())
    });

    updater(lua_api, "set_state", |animator, _, _, state: String| {
        Ok(animator.set_state(&state))
    });

    lua_api.add_dynamic_function(ANIMATION_TABLE, "derive_state", |api_ctx, lua, params| {
        let (table, state, frame_data): (rollback_mlua::Table, String, Vec<Vec<usize>>) =
            lua.unpack_multi(params)?;
        let derived_frames = frame_data
            .into_iter()
            .flat_map(|item| {
                let frame_index = *item.first()?;
                let duration = *item.get(1)? as FrameTime;

                Some(DerivedFrame::new(frame_index.max(1) - 1, duration))
            })
            .collect();

        let animator_index: GenerationalIndex = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let animators = &mut api_ctx.simulation.animators;

        if !animators.contains_key(animator_index) {
            return Err(animator_not_found());
        }

        let derived_state =
            BattleAnimator::derive_state(animators, &state, derived_frames, animator_index);

        lua.pack_multi(derived_state)
    });

    getter(lua_api, "has_point", |animator, lua, name: String| {
        lua.pack_multi(animator.point(&name).is_some())
    });

    getter(lua_api, "get_point", |animator, lua, name: String| {
        lua.pack_multi(LuaVector::from(animator.point(&name).unwrap_or_default()))
    });

    setter(lua_api, "set_playback", |animator, lua, mode| {
        animator.set_playback_mode(mode);
        lua.pack_multi(())
    });

    setter(lua_api, "on_complete", |animator, lua, callback| {
        animator.on_complete(callback);
        lua.pack_multi(())
    });

    setter(lua_api, "on_interrupt", |animator, lua, callback| {
        animator.on_interrupt(callback);
        lua.pack_multi(())
    });

    setter(
        lua_api,
        "on_frame",
        |animator, lua, (frame, callback, do_once): (usize, BattleCallback, Option<bool>)| {
            animator.on_frame(frame.max(1) - 1, callback, do_once.unwrap_or_default());
            lua.pack_multi(())
        },
    );
}

fn getter<F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(
            &BattleAnimator,
            &'lua rollback_mlua::Lua,
            P,
        ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>
        + 'static,
{
    lua_api.add_dynamic_function(ANIMATION_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = api_ctx.borrow();
        let animators = &api_ctx.simulation.animators;
        let animator = animators.get(id).ok_or_else(animator_not_found)?;

        lua.pack_multi(callback(animator, lua, param)?)
    });
}

fn setter<F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(
            &mut BattleAnimator,
            &'lua rollback_mlua::Lua,
            P,
        ) -> rollback_mlua::Result<rollback_mlua::MultiValue<'lua>>
        + 'static,
{
    lua_api.add_dynamic_function(ANIMATION_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let mut api_ctx = api_ctx.borrow_mut();
        let animators = &mut api_ctx.simulation.animators;
        let animator = animators.get_mut(id).ok_or_else(animator_not_found)?;

        lua.pack_multi(callback(animator, lua, param)?)
    });
}

fn updater<F, P>(lua_api: &mut BattleLuaApi, name: &str, callback: F)
where
    P: for<'lua> rollback_mlua::FromLuaMulti<'lua>,
    F: for<'lua> Fn(
            &mut BattleAnimator,
            &GameIO,
            &'lua rollback_mlua::Lua,
            P,
        ) -> rollback_mlua::Result<Vec<BattleCallback>>
        + 'static,
{
    lua_api.add_dynamic_function(ANIMATION_TABLE, name, move |api_ctx, lua, params| {
        let (table, param): (rollback_mlua::Table, P) = lua.unpack_multi(params)?;

        let id: GenerationalIndex = table.raw_get("#id")?;

        let api_ctx = &mut *api_ctx.borrow_mut();
        let simulation = &mut api_ctx.simulation;
        let game_io = api_ctx.game_io;

        let animators = &mut simulation.animators;
        let animator = animators.get_mut(id).ok_or_else(animator_not_found)?;

        let callbacks = callback(animator, game_io, lua, param)?;

        animator.find_and_apply_to_target(&mut simulation.sprite_trees);

        simulation.pending_callbacks.extend(callbacks);
        simulation.call_pending_callbacks(game_io, api_ctx.resources);

        lua.pack_multi(())
    });
}

pub fn create_animation_table(
    lua: &rollback_mlua::Lua,
    index: GenerationalIndex,
) -> rollback_mlua::Result<rollback_mlua::Table> {
    let table = lua.create_table()?;
    table.raw_set("#id", index)?;
    inherit_metatable(lua, ANIMATION_TABLE, &table)?;

    Ok(table)
}
