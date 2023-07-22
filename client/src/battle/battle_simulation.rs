use super::rollback_vm::RollbackVM;
use super::*;
use crate::bindable::*;
use crate::packages::{PackageId, PackageNamespace};
use crate::render::ui::{FontStyle, PlayerHealthUi, Text};
use crate::render::*;
use crate::resources::*;
use crate::saves::Card;
use crate::scenes::BattleEvent;
use framework::prelude::*;
use generational_arena::Arena;
use packets::structures::{BattleStatistics, BattleSurvivor};
use packets::NetplaySignal;
use rand::SeedableRng;
use rand_xoshiro::Xoshiro256PlusPlus;
use std::cell::RefCell;

pub struct BattleSimulation {
    pub config: BattleConfig,
    pub statistics: BattleStatistics,
    pub rng: Xoshiro256PlusPlus,
    pub inputs: Vec<PlayerInput>,
    pub time: FrameTime,
    pub battle_time: FrameTime,
    pub camera: Camera,
    pub background: Background,
    pub fade_sprite: Sprite,
    pub turn_gauge: TurnGauge,
    pub field: Field,
    pub tile_states: Vec<TileState>,
    pub entities: hecs::World,
    pub generation_tracking: Vec<hecs::Entity>,
    pub queued_attacks: Vec<AttackBox>,
    pub defense_judge: DefenseJudge,
    pub animators: Arena<BattleAnimator>,
    pub actions: Arena<Action>,
    pub time_freeze_tracker: TimeFreezeTracker,
    pub components: Arena<Component>,
    pub pending_callbacks: Vec<BattleCallback>,
    pub local_player_id: EntityId,
    pub local_health_ui: PlayerHealthUi,
    pub local_team: Team,
    pub music_stack_depth: usize,
    pub battle_started: bool,
    pub intro_complete: bool,
    pub is_resimulation: bool,
    pub exit: bool,
}

impl BattleSimulation {
    pub fn new(game_io: &GameIO, background: Background, player_count: usize) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(Vec2::new(0.0, 10.0));

        let game_run_duration = game_io.frame_start_instant() - game_io.game_start_instant();
        let default_seed = game_run_duration.as_secs();

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        Self {
            config: BattleConfig::new(globals, player_count),
            statistics: BattleStatistics::new(),
            rng: Xoshiro256PlusPlus::seed_from_u64(default_seed),
            time: 0,
            battle_time: 0,
            inputs: vec![PlayerInput::new(); player_count],
            camera,
            background,
            fade_sprite: assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL),
            turn_gauge: TurnGauge::new(game_io),
            field: Field::new(game_io, 8, 5),
            tile_states: TileState::create_registry(),
            entities: hecs::World::new(),
            generation_tracking: Vec::new(),
            queued_attacks: Vec::new(),
            defense_judge: DefenseJudge::new(),
            animators: Arena::new(),
            actions: Arena::new(),
            time_freeze_tracker: TimeFreezeTracker::new(),
            components: Arena::new(),
            pending_callbacks: Vec::new(),
            local_player_id: EntityId::DANGLING,
            local_health_ui: PlayerHealthUi::new(game_io),
            local_team: Team::Unset,
            music_stack_depth: globals.audio.music_stack_len() + 1,
            battle_started: false,
            intro_complete: false,
            is_resimulation: false,
            exit: false,
        }
    }

    pub fn seed_random(&mut self, seed: u64) {
        self.rng = Xoshiro256PlusPlus::seed_from_u64(seed);
    }

    pub fn clone(&mut self, game_io: &GameIO) -> Self {
        let mut entities = hecs::World::new();

        // spawn + remove blank entities to restore generations on dead entities
        // otherwise if there's no living entity holding an id
        // a new entity can spawn reusing an id a script may be tracking
        // the most obvious sign of this is seeing the camera flip after a player dies and enemy spawns
        // (enemy reuses the player id, making the engine think the player changed teams)
        for id in self.generation_tracking.iter().cloned() {
            entities.spawn_at(id, ());
            let _ = entities.despawn(id);
        }

        // starting with Entity as every entity will have Entity
        for (id, entity) in self.entities.query_mut::<&Entity>() {
            entities.spawn_at(id, (entity.clone(),));
        }

        // cloning every component
        for (id, component) in self.entities.query_mut::<&Artifact>() {
            let _ = entities.insert_one(id, component.clone());
        }

        for (id, component) in self.entities.query_mut::<&Character>() {
            let _ = entities.insert_one(id, component.clone());
        }

        for (id, component) in self.entities.query_mut::<&Living>() {
            let _ = entities.insert_one(id, component.clone());
        }

        for (id, component) in self.entities.query_mut::<&Obstacle>() {
            let _ = entities.insert_one(id, component.clone());
        }

        for (id, component) in self.entities.query_mut::<&Player>() {
            let _ = entities.insert_one(id, component.clone());
        }

        for (id, component) in self.entities.query_mut::<&Spell>() {
            let _ = entities.insert_one(id, component.clone());
        }

        Self {
            config: self.config.clone(),
            statistics: self.statistics.clone(),
            inputs: self.inputs.clone(),
            rng: self.rng.clone(),
            time: self.time,
            battle_time: self.battle_time,
            camera: self.camera.clone(game_io),
            background: self.background.clone(),
            fade_sprite: self.fade_sprite.clone(),
            turn_gauge: self.turn_gauge.clone(),
            field: self.field.clone(),
            tile_states: self.tile_states.clone(),
            entities,
            generation_tracking: self.generation_tracking.clone(),
            queued_attacks: self.queued_attacks.clone(),
            defense_judge: self.defense_judge,
            animators: self.animators.clone(),
            actions: self.actions.clone(),
            time_freeze_tracker: self.time_freeze_tracker.clone(),
            components: self.components.clone(),
            pending_callbacks: self.pending_callbacks.clone(),
            local_player_id: self.local_player_id,
            local_health_ui: self.local_health_ui.clone(),
            local_team: self.local_team,
            music_stack_depth: self.music_stack_depth,
            battle_started: self.battle_started,
            intro_complete: self.intro_complete,
            is_resimulation: self.is_resimulation,
            exit: self.exit,
        }
    }

    pub fn initialize_uninitialized(&mut self) {
        self.field.initialize_uninitialized();

        Player::initialize_uninitialized(self);
    }

    pub fn play_sound(&self, game_io: &GameIO, sound_buffer: &SoundBuffer) {
        if !self.is_resimulation {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(sound_buffer);
        }
    }

    pub fn play_music(
        &self,
        game_io: &GameIO,
        sound_buffer: &SoundBuffer,
        loops: bool,
        // todo:
        _start_ms: Option<u64>,
        _end_ms: Option<u64>,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();

        if globals.audio.music_stack_len() != self.music_stack_depth {
            return;
        }

        globals.audio.play_music(sound_buffer, loops);
    }

    pub fn wrap_up_statistics(&mut self) {
        let entities = &mut self.entities;

        if let Ok((entity, living)) =
            entities.query_one_mut::<(&Entity, &Living)>(self.local_player_id.into())
        {
            self.local_team = entity.team;
            self.statistics.health = living.health;
        }

        for (_, (entity, living)) in entities.query_mut::<(&Entity, &Living)>() {
            if entity.team == self.local_team && entity.id != self.local_player_id {
                self.statistics.ally_survivors.push(BattleSurvivor {
                    name: entity.name.clone(),
                    health: living.health,
                });
            }

            if entity.team != self.local_team && entity.team != Team::Other {
                self.statistics.enemy_survivors.push(BattleSurvivor {
                    name: entity.name.clone(),
                    health: living.health,
                });
            }

            if entity.team != self.local_team && entity.team == Team::Other {
                self.statistics.neutral_survivors.push(BattleSurvivor {
                    name: entity.name.clone(),
                    health: living.health,
                });
            }
        }

        self.statistics.calculate_score();
    }

    pub fn handle_local_signals(
        &mut self,
        local_index: usize,
        shared_assets: &mut SharedBattleAssets,
    ) {
        let input = &self.inputs[local_index];

        // handle flee
        if !self.is_resimulation && input.has_signal(NetplaySignal::AttemptingFlee) {
            // todo: check for success using some method in battle_init
            shared_assets
                .event_sender
                .send(BattleEvent::FleeResult(true))
                .unwrap();
        }
    }

    pub fn pre_update(&mut self, game_io: &GameIO, vms: &[RollbackVM], state: &dyn State) {
        #[cfg(debug_assertions)]
        self.assertions();

        // update bg
        self.background.update();

        // reset fade
        self.fade_sprite.set_color(Color::TRANSPARENT);

        if state.allows_animation_updates() {
            // update animations
            self.update_animations();
        }

        // process disconnects
        self.process_disconnects();

        // spawn pending entities
        self.spawn_pending();

        // apply animations after spawning to display frame 0
        self.apply_animations();

        // animation + spawn callbacks
        self.call_pending_callbacks(game_io, vms);
    }

    pub fn post_update(&mut self, game_io: &GameIO, vms: &[RollbackVM]) {
        // update scene components
        for (_, component) in &self.components {
            if component.lifetime == ComponentLifetime::Scene {
                self.pending_callbacks
                    .push(component.update_callback.clone());
            }
        }

        self.update_sync_nodes();

        // should this be called every time we invoke lua?
        self.call_pending_callbacks(game_io, vms);

        // remove dead entities
        self.cleanup_erased_entities();

        self.update_ui();

        self.field.update_animations();

        self.time += 1;
    }

    pub fn update_animations(&mut self) {
        let time_is_frozen = self.time_freeze_tracker.time_is_frozen();

        for (id, entity) in self.entities.query::<&Entity>().into_iter() {
            if entity.time_frozen_count > 0 {
                continue;
            }

            if let Some(living) = self.entities.query_one::<&Living>(id).unwrap().get() {
                if living.status_director.is_inactionable() {
                    continue;
                }
            }

            let animator = &mut self.animators[entity.animator_index];
            self.pending_callbacks.extend(animator.update());
        }

        for (_, action) in &mut self.actions {
            if !action.executed {
                continue;
            }

            let entities = &mut self.entities;

            let Ok(entity) = entities.query_one_mut::<&mut Entity>(action.entity.into()) else {
                continue;
            };

            if entity.time_frozen_count > 0 || (time_is_frozen && !action.properties.time_freeze) {
                continue;
            }

            if let Ok(living) = self.entities.query_one_mut::<&Living>(action.entity.into()) {
                if living.status_director.is_inactionable() {
                    continue;
                }
            }

            for attachment in &mut action.attachments {
                let animator = &mut self.animators[attachment.animator_index];
                self.pending_callbacks.extend(animator.update());
            }
        }
    }

    fn process_disconnects(&mut self) {
        if self.exit {
            // if we're exiting we'll just ignore disconnects
            // prevents player deletion from disconnect after a victory
            return;
        }

        let mut pending_deletion = Vec::new();

        let entities = &mut self.entities;
        for (id, (entity, player)) in entities.query_mut::<(&mut Entity, &Player)>() {
            let input = &self.inputs[player.index];

            if entity.deleted || !input.has_signal(NetplaySignal::Disconnect) {
                continue;
            }

            pending_deletion.push(id);
        }

        for id in pending_deletion {
            Component::new_delayed_deleter(self, id.into(), ComponentLifetime::BattleStep, 0);
        }
    }

    fn spawn_pending(&mut self) {
        for (_, entity) in self.entities.query_mut::<&mut Entity>() {
            if !entity.pending_spawn {
                continue;
            }

            entity.pending_spawn = false;
            entity.spawned = true;
            entity.on_field = true;

            let tile = self.field.tile_at_mut((entity.x, entity.y)).unwrap();
            tile.handle_auto_reservation_addition(&self.actions, entity);

            if entity.team == Team::Unset {
                entity.team = tile.team();
            }

            if entity.facing == Direction::None {
                entity.facing = tile.direction();
            }

            self.animators[entity.animator_index].enable();
            self.pending_callbacks.push(entity.spawn_callback.clone());

            for (_, component) in &self.components {
                if component.entity == entity.id {
                    self.pending_callbacks.push(component.init_callback.clone());
                }
            }

            if self.battle_started {
                self.pending_callbacks
                    .push(entity.battle_start_callback.clone())
            }
        }
    }

    fn apply_animations(&mut self) {
        for (_, entity) in self.entities.query_mut::<&mut Entity>() {
            let sprite_node = entity.sprite_tree.root_mut();

            // update root sprite
            self.animators[entity.animator_index].apply(sprite_node);
        }

        // update attachment sprites
        // separate loop from entities to account for async actions
        for (_, action) in &mut self.actions {
            if !action.executed {
                continue;
            }

            let entities = &mut self.entities;
            let Ok(entity) = entities.query_one_mut::<&mut Entity>(action.entity.into()) else {
                continue;
            };

            for attachment in &mut action.attachments {
                attachment.apply_animation(&mut entity.sprite_tree, &mut self.animators);
            }
        }
    }

    fn update_sync_nodes(&mut self) {
        let syncers: Vec<_> = (self.animators)
            .iter()
            .filter(|(_, animator)| animator.has_synced_animators())
            .map(|(index, _)| index)
            .collect();

        for animator_index in syncers {
            BattleAnimator::sync_animators(
                &mut self.animators,
                &mut self.entities,
                &mut self.pending_callbacks,
                animator_index,
            );
        }
    }

    pub fn call_pending_callbacks(&mut self, game_io: &GameIO, vms: &[RollbackVM]) {
        let callbacks = std::mem::take(&mut self.pending_callbacks);

        for callback in callbacks {
            callback.call(game_io, self, vms, ());
        }
    }

    fn cleanup_erased_entities(&mut self) {
        let mut pending_removal = Vec::new();
        let mut components_pending_removal = Vec::new();
        let mut actions_pending_removal = Vec::new();

        for (id, entity) in self.entities.query_mut::<&Entity>() {
            if !entity.erased {
                continue;
            }

            if entity.spawned {
                self.field.drop_entity(entity.id);
            }

            for (index, component) in &mut self.components {
                if component.entity == entity.id {
                    components_pending_removal.push(index);
                }
            }

            for (index, action) in &mut self.actions {
                if action.entity == entity.id {
                    actions_pending_removal.push(index);
                }
            }

            self.animators.remove(entity.animator_index);

            pending_removal.push(id);
        }

        for id in pending_removal {
            self.entities.despawn(id).unwrap();
        }

        for index in components_pending_removal {
            self.components.remove(index);
        }

        for index in actions_pending_removal {
            // action_end callbacks would already be handled by delete listeners
            self.actions.remove(index);
        }
    }

    fn update_ui(&mut self) {
        let entities = &mut self.entities;

        if let Ok((player, living)) =
            entities.query_one_mut::<(&mut Player, &Living)>(self.local_player_id.into())
        {
            self.local_health_ui.set_health(living.health);
            player.emotion_window.update();
        } else {
            self.local_health_ui.set_health(0);
        }

        self.local_health_ui.update();
    }

    pub fn is_entity_actionable(&mut self, entity_id: EntityId) -> bool {
        let entities = &mut self.entities;

        let time_is_frozen = self.time_freeze_tracker.time_is_frozen();

        if time_is_frozen {
            let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                return false;
            };

            if entity.time_frozen_count > 0 {
                return false;
            }
        }

        if let Ok(living) = entities.query_one_mut::<&Living>(entity_id.into()) {
            return !living.status_director.is_inactionable();
        };

        true
    }

    pub fn use_action(
        &mut self,
        game_io: &GameIO,
        entity_id: EntityId,
        index: generational_arena::Index,
    ) -> bool {
        let Ok(entity) = self.entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
            return false;
        };

        let time_is_frozen = self.time_freeze_tracker.time_is_frozen();

        if !time_is_frozen && entity.action_index.is_some() {
            // already set
            return false;
        }

        // validate index as it may be coming from lua
        let Some(action) = self.actions.get_mut(index) else {
            log::error!("Received invalid Action index {index:?}");
            return false;
        };

        if action.used {
            log::error!("Action already used, ignoring");
            return false;
        }

        action.entity = entity_id;

        if time_is_frozen && !action.properties.time_freeze {
            return false;
        }

        action.used = true;

        if action.properties.time_freeze {
            let time_freeze_tracker = &mut self.time_freeze_tracker;

            if time_is_frozen && !self.is_resimulation {
                // must be countering, play sfx
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.sfx.trap);
            }

            time_freeze_tracker.set_team_action(entity.team, index);
        } else {
            entity.action_index = Some(index);
        }

        true
    }

    pub fn delete_actions(
        &mut self,
        game_io: &GameIO,
        vms: &[RollbackVM],
        delete_indices: &[generational_arena::Index],
    ) {
        for index in delete_indices {
            let Some(action) = self.actions.get_mut(*index) else {
                continue;
            };

            if action.deleted {
                // avoid callbacks calling delete_actions on this card action
                continue;
            }

            action.deleted = true;

            // remove the index from the entity
            let entity = self
                .entities
                .query_one_mut::<&mut Entity>(action.entity.into())
                .unwrap();

            if entity.action_index == Some(*index) {
                action.complete_sync(
                    &mut self.entities,
                    &mut self.animators,
                    &mut self.pending_callbacks,
                    &mut self.field,
                );
            }

            // end callback
            if let Some(callback) = action.end_callback.clone() {
                callback.call(game_io, self, vms, ());
            }

            let action = self.actions.get(*index).unwrap();

            // remove attachments from the entity
            let entity = self
                .entities
                .query_one_mut::<&mut Entity>(action.entity.into())
                .unwrap();

            entity.sprite_tree.remove(action.sprite_index);

            for attachment in &action.attachments {
                self.animators.remove(attachment.animator_index);
            }

            // finally remove the card action
            self.actions.remove(*index);
        }

        self.call_pending_callbacks(game_io, vms);
    }

    pub fn mark_entity_for_erasure(&mut self, game_io: &GameIO, vms: &[RollbackVM], id: EntityId) {
        let Ok(entity) = self.entities.query_one_mut::<&mut Entity>(id.into()) else {
            return;
        };

        if entity.erased {
            return;
        }

        // clear the delete callback
        entity.delete_callback = BattleCallback::default();

        // mark as erased
        entity.erased = true;

        // delete
        self.delete_entity(game_io, vms, id);
    }

    pub fn delete_entity(&mut self, game_io: &GameIO, vms: &[RollbackVM], id: EntityId) {
        let entity = match self.entities.query_one_mut::<&mut Entity>(id.into()) {
            Ok(entity) => entity,
            _ => return,
        };

        if entity.deleted {
            return;
        }

        let card_indices: Vec<_> = (self.actions)
            .iter()
            .filter(|(_, action)| action.entity == id && action.used)
            .map(|(index, _)| index)
            .collect();

        entity.deleted = true;

        let listener_callbacks = std::mem::take(&mut entity.delete_callbacks);
        let delete_callback = entity.delete_callback.clone();

        // delete player augments
        if let Ok(player) = self.entities.query_one_mut::<&mut Player>(id.into()) {
            let augment_iter = player.augments.iter();
            let augment_callbacks =
                augment_iter.map(|(_, augment)| augment.delete_callback.clone());

            self.pending_callbacks.extend(augment_callbacks);
            self.call_pending_callbacks(game_io, vms);
        }

        // delete card actions
        self.delete_actions(game_io, vms, &card_indices);

        // call delete callbacks after
        self.pending_callbacks.extend(listener_callbacks);
        self.pending_callbacks.push(delete_callback);

        self.call_pending_callbacks(game_io, vms);
    }

    pub fn request_entity_spawn(&mut self, id: EntityId, (x, y): (i32, i32)) {
        let entity = self
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.x = x;
        entity.y = y;
        entity.pending_spawn = true;
    }

    pub fn find_vm(
        vms: &[RollbackVM],
        package_id: &PackageId,
        namespace: PackageNamespace,
    ) -> rollback_mlua::Result<usize> {
        let vm_index = namespace
            .find_with_overrides(|namespace| {
                vms.iter().position(|vm| {
                    vm.package_id == *package_id && vm.namespaces.contains(&namespace)
                })
            })
            .ok_or_else(|| {
                rollback_mlua::Error::RuntimeError(format!(
                    "no package with id {:?} found",
                    package_id
                ))
            })?;

        Ok(vm_index)
    }

    pub fn call_global<'lua, F, M>(
        &mut self,
        game_io: &GameIO,
        vms: &'lua [RollbackVM],
        vm_index: usize,
        fn_name: &str,
        param_generator: F,
    ) -> rollback_mlua::Result<()>
    where
        F: FnOnce(&'lua rollback_mlua::Lua) -> rollback_mlua::Result<M>,
        M: rollback_mlua::ToLuaMulti<'lua>,
    {
        let lua = &vms[vm_index].lua;

        let global_fn: rollback_mlua::Function = lua
            .globals()
            .get(fn_name)
            .map_err(|e| rollback_mlua::Error::RuntimeError(format!("{e}: {fn_name}")))?;

        let api_ctx = RefCell::new(BattleScriptContext {
            vm_index,
            vms,
            game_io,
            simulation: self,
        });

        let lua_api = &game_io.resource::<Globals>().unwrap().battle_api;

        lua_api.inject_dynamic(lua, &api_ctx, move |lua| {
            let params = param_generator(lua)?;
            global_fn.call(params)
        });

        Ok(())
    }

    pub fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let mut blind_filter = None;

        // resolve perspective
        if let Ok((entity, living)) =
            (self.entities).query_one_mut::<(&Entity, &Living)>(self.local_player_id.into())
        {
            self.local_team = entity.team;

            if living.status_director.remaining_status_time(HitFlag::BLIND) > 0 {
                blind_filter = Some(entity.team);
            }
        }

        // Bool assignment. If entity is team blue set the fact that they're perspective flipped to true. -Dawn
        let perspective_flipped = self.local_team == Team::Blue;

        let assets = &game_io.resource::<Globals>().unwrap().assets;

        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::default());

        // draw field
        self.field.draw(
            game_io,
            &mut sprite_queue,
            &self.tile_states,
            perspective_flipped,
        );

        // draw dramatic fade
        if self.fade_sprite.color().a > 0.0 {
            self.fade_sprite.set_bounds(self.camera.bounds());

            sprite_queue.draw_sprite(&self.fade_sprite);
        }

        // draw entities, sorting by position
        let mut sorted_entities = Vec::with_capacity(self.entities.len() as usize);

        // filter characters + obstacles for blindness
        for (id, entity) in self.entities.query_mut::<hecs::With<&Entity, &Living>>() {
            if blind_filter.is_some() && blind_filter != Some(entity.team) {
                continue;
            }

            if entity.on_field && entity.sprite_tree.root().visible() {
                sorted_entities.push((id, entity.sort_key()));
            }
        }

        // add everything else
        for (id, entity) in self.entities.query_mut::<hecs::Without<&Entity, &Living>>() {
            if entity.on_field && entity.sprite_tree.root().visible() {
                sorted_entities.push((id, entity.sort_key()));
            }
        }

        sorted_entities.sort_by_key(|(_, key)| *key);

        for (id, _) in sorted_entities {
            let entity = self.entities.query_one_mut::<&mut Entity>(id).unwrap();

            // offset for calculating initial placement position
            let mut offset: Vec2 = entity.corrected_offset(perspective_flipped);

            // elevation
            offset.y -= entity.elevation;
            let shadow_node = &mut entity.sprite_tree[entity.shadow_index];
            shadow_node.set_offset(Vec2::new(shadow_node.offset().x, entity.elevation));

            let tile_center =
                (self.field).calc_tile_center((entity.x, entity.y), perspective_flipped);
            let initial_position = tile_center + offset;

            // true if only one is true, since flipping twice causes us to no longer be flipped
            let flipped = perspective_flipped ^ entity.flipped();

            // offset each child by parent node accounting for perspective, and draw
            let sprite_tree = &mut entity.sprite_tree;
            sprite_tree.draw_with_offset(&mut sprite_queue, initial_position, flipped);
        }

        sprite_queue.set_shader_effect(SpriteShaderEffect::Default);

        // draw hp on living entities
        if self.intro_complete {
            let mut hp_text = Text::new(game_io, FontStyle::EntityHP);
            hp_text.style.letter_spacing = 0.0;
            let tile_size = self.field.tile_size();

            type Query<'a> = hecs::Without<(&'a Entity, &'a Living, &'a Character), &'a Obstacle>;

            for (_, (entity, living, ..)) in self.entities.query_mut::<Query>() {
                if entity.deleted
                    || !entity.on_field
                    || living.health <= 0
                    || !entity.sprite_tree.root().visible()
                    || entity.id == self.local_player_id
                {
                    continue;
                }

                if blind_filter.is_some() && blind_filter != Some(entity.team) {
                    // blindness filter
                    continue;
                }

                let entity_screen_position =
                    entity.screen_position(&self.field, perspective_flipped);

                hp_text.text = living.health.to_string();
                let text_size = hp_text.measure().size;

                (hp_text.style.bounds).set_position(entity_screen_position);
                hp_text.style.bounds.x -= text_size.x * 0.5;
                hp_text.style.bounds.y += tile_size.y * 0.5 - text_size.y;
                hp_text.draw(game_io, &mut sprite_queue);
            }
        }

        // draw card icons for the local player
        if let Ok((entity, character)) =
            (self.entities).query_one_mut::<(&Entity, &Character)>(self.local_player_id.into())
        {
            sprite_queue.set_color_mode(SpriteColorMode::Multiply);

            let mut base_position = entity.screen_position(&self.field, perspective_flipped);
            base_position.y -= entity.height + 16.0;

            let mut border_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
            border_sprite.set_color(Color::BLACK);
            border_sprite.set_size(Vec2::new(16.0, 16.0));

            for i in 0..character.cards.len() {
                let card = &character.cards[i];
                let blank_card = Card {
                    package_id: card.package_id.clone(),
                    code: String::new(),
                };

                let cards_before = character.cards.len() - i;
                let card_offset = 2.0 * cards_before as f32;
                let position = base_position - card_offset;

                border_sprite.set_position(position - 1.0);
                sprite_queue.draw_sprite(&border_sprite);

                blank_card.draw_icon(game_io, &mut sprite_queue, position);
            }
        }

        render_pass.consume_queue(sprite_queue);
    }

    pub fn draw_ui(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        self.local_health_ui.draw(game_io, sprite_queue);

        let entities = &mut self.entities;

        if let Ok(player) = entities.query_one_mut::<&mut Player>(self.local_player_id.into()) {
            let local_health_bounds = self.local_health_ui.bounds();
            let mut emotion_window_position = local_health_bounds.position();
            emotion_window_position.y += local_health_bounds.height + 1.0;

            player.emotion_window.set_position(emotion_window_position);
            player.emotion_window.draw(sprite_queue);
        }
    }

    #[cfg(debug_assertions)]
    fn assertions(&mut self) {
        // make sure card actions are being deleted
        let held_action_count = self
            .entities
            .query_mut::<&Entity>()
            .into_iter()
            .filter(|(_, entity)| entity.action_index.is_some())
            .count();

        let executed_action_count = self
            .actions
            .iter()
            .filter(|(_, action)| action.executed && !action.is_async())
            .count();

        // if there's more executed actions than held actions, we forgot to delete one
        // we can have more actions than held actions still since
        // scripters don't need to attach card actions to entities
        // we also can ignore async actions
        assert!(held_action_count >= executed_action_count);
    }
}
