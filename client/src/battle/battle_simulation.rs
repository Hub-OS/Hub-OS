use super::rollback_vm::RollbackVM;
use super::*;
use crate::bindable::*;
use crate::lua_api::{create_augment_table, create_entity_table};
use crate::packages::{PackageId, PackageNamespace};
use crate::render::ui::{FontStyle, PlayerHealthUI, Text};
use crate::render::*;
use crate::resources::*;
use crate::saves::{BlockGrid, Card, Deck};
use framework::prelude::*;
use generational_arena::Arena;
use packets::structures::BattleStatistics;
use rand::SeedableRng;
use rand_xoshiro::Xoshiro256PlusPlus;
use std::cell::RefCell;

const DEFAULT_PLAYER_LAYOUTS: [[(i32, i32); 4]; 4] = [
    [(2, 2), (0, 0), (0, 0), (0, 0)],
    [(2, 2), (5, 2), (0, 0), (0, 0)],
    [(2, 2), (4, 3), (6, 1), (0, 0)],
    [(1, 3), (3, 1), (4, 3), (6, 1)],
];

pub struct BattleSimulation {
    pub battle_started: bool,
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
    pub entities: hecs::World,
    pub generation_tracking: Vec<hecs::Entity>,
    pub queued_attacks: Vec<AttackBox>,
    pub defense_judge: DefenseJudge,
    pub animators: Arena<BattleAnimator>,
    pub card_actions: Arena<CardAction>,
    pub time_freeze_tracker: TimeFreezeTracker,
    pub components: Arena<Component>,
    pub pending_callbacks: Vec<BattleCallback>,
    pub local_player_id: EntityId,
    pub local_health_ui: PlayerHealthUI,
    pub player_spawn_positions: Vec<(i32, i32)>,
    pub perspective_flipped: bool,
    pub intro_complete: bool,
    pub is_resimulation: bool,
    pub exit: bool,
}

impl BattleSimulation {
    pub fn new(game_io: &GameIO, background: Background, player_count: usize) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(Vec2::new(0.0, 10.0));

        let spawn_count = player_count.min(4);
        let mut player_spawn_positions = DEFAULT_PLAYER_LAYOUTS[spawn_count - 1].to_vec();
        player_spawn_positions.resize(spawn_count, (0, 0));

        let game_run_duration = game_io.frame_start_instant() - game_io.game_start_instant();
        let default_seed = game_run_duration.as_secs();

        let assets = &game_io.resource::<Globals>().unwrap().assets;

        Self {
            battle_started: false,
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
            entities: hecs::World::new(),
            generation_tracking: Vec::new(),
            queued_attacks: Vec::new(),
            defense_judge: DefenseJudge::new(),
            animators: Arena::new(),
            card_actions: Arena::new(),
            time_freeze_tracker: TimeFreezeTracker::new(),
            components: Arena::new(),
            pending_callbacks: Vec::new(),
            local_player_id: EntityId::DANGLING,
            local_health_ui: PlayerHealthUI::new(game_io),
            player_spawn_positions,
            perspective_flipped: false,
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
            battle_started: self.battle_started,
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
            entities,
            generation_tracking: self.generation_tracking.clone(),
            queued_attacks: self.queued_attacks.clone(),
            defense_judge: self.defense_judge,
            animators: self.animators.clone(),
            card_actions: self.card_actions.clone(),
            time_freeze_tracker: self.time_freeze_tracker.clone(),
            components: self.components.clone(),
            pending_callbacks: self.pending_callbacks.clone(),
            local_player_id: self.local_player_id,
            local_health_ui: self.local_health_ui.clone(),
            player_spawn_positions: self.player_spawn_positions.clone(),
            perspective_flipped: self.perspective_flipped,
            intro_complete: self.intro_complete,
            is_resimulation: self.is_resimulation,
            exit: self.exit,
        }
    }

    pub fn initialize_uninitialized(&mut self) {
        self.field.initialize_uninitialized();

        type PlayerQuery<'a> = (&'a mut Entity, &'a Player, &'a Living);

        for (_, (entity, player, living)) in self.entities.query_mut::<PlayerQuery>() {
            if player.local {
                self.local_player_id = entity.id;
                self.local_health_ui.snap_health(living.health);
            }

            let pos = self
                .player_spawn_positions
                .get(player.index)
                .cloned()
                .unwrap_or_default();

            entity.x = pos.0;
            entity.y = pos.1;

            let animator = &mut self.animators[entity.animator_index];

            if animator.current_state().is_none() {
                let callbacks = animator.set_state(Player::IDLE_STATE);
                animator.set_loop_mode(AnimatorLoopMode::Loop);
                self.pending_callbacks.extend(callbacks);
            }
        }
    }

    pub fn play_sound(&self, game_io: &GameIO, sound_buffer: &SoundBuffer) {
        if !self.is_resimulation {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(sound_buffer);
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

        for (_, action) in &mut self.card_actions {
            if !action.executed {
                continue;
            }

            let Ok(entity) = self.entities.query_one_mut::<&mut Entity>(action.entity.into()) else {
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

    fn spawn_pending(&mut self) {
        for (_, entity) in self.entities.query_mut::<&mut Entity>() {
            if !entity.pending_spawn {
                continue;
            }

            entity.pending_spawn = false;
            entity.spawned = true;
            entity.on_field = true;

            let tile = self.field.tile_at_mut((entity.x, entity.y)).unwrap();
            tile.handle_auto_reservation_addition(&self.card_actions, entity);

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
        for (_, action) in &mut self.card_actions {
            if !action.executed {
                continue;
            }

            let Ok(entity) = self.entities.query_one_mut::<&mut Entity>(action.entity.into()) else {
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
        let mut card_actions_pending_removal = Vec::new();

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

            for (index, card_action) in &mut self.card_actions {
                if card_action.entity == entity.id {
                    card_actions_pending_removal.push(index);
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

        for index in card_actions_pending_removal {
            // action_end callbacks would already be handled by delete listeners
            self.card_actions.remove(index);
        }
    }

    fn update_ui(&mut self) {
        if let Ok(living) = (self.entities).query_one_mut::<&Living>(self.local_player_id.into()) {
            self.local_health_ui.set_health(living.health);
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

    pub fn use_card_action(
        &mut self,
        game_io: &GameIO,
        entity_id: EntityId,
        index: generational_arena::Index,
    ) -> bool {
        let Ok(entity) = self.entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
            return false;
        };

        let time_is_frozen = self.time_freeze_tracker.time_is_frozen();

        if !time_is_frozen && entity.card_action_index.is_some() {
            // already set
            return false;
        }

        // validate index as it may be coming from lua
        let Some(card_action) = self.card_actions.get_mut(index) else {
            log::error!("Received invalid CardAction index {index:?}");
            return false;
        };

        if time_is_frozen && !card_action.properties.time_freeze {
            return false;
        }

        card_action.used = true;

        if card_action.properties.time_freeze {
            let time_freeze_tracker = &mut self.time_freeze_tracker;

            if time_is_frozen && !self.is_resimulation {
                // must be countering, play sfx
                let globals = game_io.resource::<Globals>().unwrap();
                globals.audio.play_sound(&globals.trap_sfx);
            }

            time_freeze_tracker.set_team_action(entity.team, index);
        } else {
            entity.card_action_index = Some(index);
        }

        true
    }

    pub fn delete_card_actions(
        &mut self,
        game_io: &GameIO,
        vms: &[RollbackVM],
        delete_indices: &[generational_arena::Index],
    ) {
        for index in delete_indices {
            let Some(card_action) = self.card_actions.get_mut(*index) else {
                continue;
            };

            if card_action.deleted {
                // avoid callbacks calling delete_card_actions on this card action
                continue;
            }

            card_action.deleted = true;

            // remove the index from the entity
            let entity = self
                .entities
                .query_one_mut::<&mut Entity>(card_action.entity.into())
                .unwrap();

            if entity.card_action_index == Some(*index) {
                card_action.complete_sync(
                    &mut self.entities,
                    &mut self.animators,
                    &mut self.pending_callbacks,
                    &mut self.field,
                );
            }

            // end callback
            if let Some(callback) = card_action.end_callback.clone() {
                callback.call(game_io, self, vms, ());
            }

            let card_action = self.card_actions.get(*index).unwrap();

            // remove attachments from the entity
            let entity = self
                .entities
                .query_one_mut::<&mut Entity>(card_action.entity.into())
                .unwrap();

            entity.sprite_tree.remove(card_action.sprite_index);

            for attachment in &card_action.attachments {
                self.animators.remove(attachment.animator_index);
            }

            // finally remove the card action
            self.card_actions.remove(*index);
        }

        self.call_pending_callbacks(game_io, vms);
    }

    pub fn delete_entity(&mut self, game_io: &GameIO, vms: &[RollbackVM], id: EntityId) {
        let entity = match self.entities.query_one_mut::<&mut Entity>(id.into()) {
            Ok(entity) => entity,
            _ => return,
        };

        if entity.deleted {
            return;
        }

        let card_indices: Vec<_> = (self.card_actions)
            .iter()
            .filter(|(_, action)| action.entity == id && action.used)
            .map(|(index, _)| index)
            .collect();

        entity.deleted = true;

        let listener_callbacks = std::mem::take(&mut entity.delete_callbacks);
        let delete_callback = entity.delete_callback.clone();

        // delete player modifiers
        if let Ok(player) = self.entities.query_one_mut::<&mut Player>(id.into()) {
            let modifier_iter = player.modifiers.iter();
            let modifier_callbacks =
                modifier_iter.map(|(_, modifier)| modifier.delete_callback.clone());

            self.pending_callbacks.extend(modifier_callbacks);
            self.call_pending_callbacks(game_io, vms);
        }

        // delete card actions
        self.delete_card_actions(game_io, vms, &card_indices);

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

    fn reserve_entity(&mut self) -> EntityId {
        let id = self.entities.reserve_entity();

        let match_id = |stored: &&mut hecs::Entity| stored.id() == id.id();

        if let Some(stored) = self.generation_tracking.iter_mut().find(match_id) {
            *stored = id;
        } else {
            self.generation_tracking.push(id);
        }

        id.into()
    }

    fn create_entity(&mut self, game_io: &GameIO) -> EntityId {
        let id: EntityId = self.reserve_entity();

        let mut animator = BattleAnimator::new();
        animator.set_target(id, GenerationalIndex::tree_root());
        animator.disable();

        let animator_index = self.animators.insert(animator);

        self.entities
            .spawn_at(id.into(), (Entity::new(game_io, id, animator_index),));

        id
    }

    pub fn find_vm(
        vms: &[RollbackVM],
        package_id: &PackageId,
        namespace: PackageNamespace,
    ) -> rollback_mlua::Result<usize> {
        let vm_index = namespace
            .find_with_fallback(|namespace| {
                vms.iter()
                    .position(|vm| vm.package_id == *package_id && vm.namespace == namespace)
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

    fn create_character(
        &mut self,
        game_io: &GameIO,
        rank: CharacterRank,
        namespace: PackageNamespace,
    ) -> rollback_mlua::Result<EntityId> {
        let id = self.create_entity(game_io);

        self.entities
            .insert(
                id.into(),
                (Character::new(rank, namespace), Living::default()),
            )
            .unwrap();

        let entity = self
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        // characters should own their tiles by default
        entity.share_tile = false;
        entity.auto_reserves_tiles = true;

        entity.can_move_to_callback = BattleCallback::new(move |_, simulation, _, dest| {
            if simulation.field.is_edge(dest) {
                // can't walk on edge tiles
                return false;
            }

            let tile = match simulation.field.tile_at_mut(dest) {
                Some(tile) => tile,
                None => return false,
            };

            let entity = simulation
                .entities
                .query_one_mut::<&Entity>(id.into())
                .unwrap();

            if !entity.ignore_hole_tiles && tile.state().is_hole() {
                // can't walk on holes
                return false;
            }

            if tile.team() != entity.team && tile.team() != Team::Other {
                // tile can't belong to the opponent team
                return false;
            }

            if entity.share_tile {
                return true;
            }

            for entity_id in tile.reservations().iter().cloned() {
                if entity_id == id {
                    continue;
                }

                let other_entity = simulation
                    .entities
                    .query_one_mut::<&Entity>(entity_id.into())
                    .unwrap();

                if !other_entity.share_tile {
                    // another entity is reserving this tile and refusing to share
                    return false;
                }
            }

            true
        });

        entity.delete_callback = BattleCallback::new(move |_, simulation, _, _| {
            super::delete_character_animation(simulation, id, None);
        });

        Ok(id)
    }

    pub fn load_player(
        &mut self,
        game_io: &GameIO,
        vms: &[RollbackVM],
        setup: PlayerSetup,
    ) -> rollback_mlua::Result<EntityId> {
        let PlayerSetup {
            player_package,
            index,
            local,
            deck: Deck { cards, .. },
            blocks,
            ..
        } = setup;

        // namespace for using cards / attacks
        let namespace = if local {
            PackageNamespace::Local
        } else {
            PackageNamespace::Remote(index)
        };

        let id = self.create_character(game_io, CharacterRank::V1, namespace)?;

        let (entity, living) = self
            .entities
            .query_one_mut::<(&mut Entity, &mut Living)>(id.into())
            .unwrap();

        // spawn immediately
        entity.pending_spawn = true;

        // use preloaded package properties
        entity.element = player_package.element;
        entity.name = player_package.name.clone();
        living.status_director.set_input_index(index);

        // derive states
        let move_anim_state = BattleAnimator::derive_state(
            &mut self.animators,
            "PLAYER_MOVE",
            Player::MOVE_FRAMES.to_vec(),
            entity.animator_index,
        );

        let flinch_anim_state = BattleAnimator::derive_state(
            &mut self.animators,
            "PLAYER_HIT",
            Player::HIT_FRAMES.to_vec(),
            entity.animator_index,
        );

        entity.move_anim_state = Some(move_anim_state);
        living.flinch_anim_state = Some(flinch_anim_state);

        let charge_index =
            (entity.sprite_tree).insert_root_child(SpriteNode::new(game_io, SpriteColorMode::Add));
        let charge_sprite = &mut entity.sprite_tree[charge_index];
        charge_sprite.set_texture(game_io, ResourcePaths::BATTLE_CHARGE.to_string());
        charge_sprite.set_visible(false);
        charge_sprite.set_layer(-2);
        charge_sprite.set_offset(Vec2::new(0.0, -20.0));

        // delete callback
        entity.delete_callback = BattleCallback::new(move |game_io, simulation, _, _| {
            super::delete_player_animation(game_io, simulation, id);
        });

        // flinch callback
        living.register_status_callback(
            HitFlag::FLINCH,
            BattleCallback::new(move |game_io, simulation, vms, _| {
                let (entity, living, player) = simulation
                    .entities
                    .query_one_mut::<(&mut Entity, &Living, &mut Player)>(id.into())
                    .unwrap();

                if !living.status_director.is_dragged() {
                    // cancel movement
                    entity.move_action = None;
                }

                player.charging_time = 0;

                let animator = &mut simulation.animators[entity.animator_index];

                let callbacks = animator.set_state(living.flinch_anim_state.as_ref().unwrap());
                simulation.pending_callbacks.extend(callbacks);

                // on complete will return to idle
                animator.on_complete(BattleCallback::new(move |game_io, simulation, vms, _| {
                    let entity = simulation
                        .entities
                        .query_one_mut::<&Entity>(id.into())
                        .unwrap();

                    let animator = &mut simulation.animators[entity.animator_index];

                    let callbacks = animator.set_state(Player::IDLE_STATE);
                    animator.set_loop_mode(AnimatorLoopMode::Loop);
                    simulation.pending_callbacks.extend(callbacks);

                    simulation.call_pending_callbacks(game_io, vms);
                }));

                simulation.play_sound(game_io, &game_io.resource::<Globals>().unwrap().hurt_sfx);
                simulation.call_pending_callbacks(game_io, vms);
            }),
        );

        // hit callback for decross
        living.register_hit_callback(BattleCallback::new(
            move |game_io, simulation, _, hit_props: HitProperties| {
                if hit_props.damage == 0 {
                    return;
                }

                let (entity, living, player) = simulation
                    .entities
                    .query_one_mut::<(&Entity, &Living, &mut Player)>(id.into())
                    .unwrap();

                if living.health <= 0 || entity.deleted {
                    // skip decross if we're already deleted
                    return;
                }

                if !entity.element.is_weak_to(hit_props.element)
                    && !entity.element.is_weak_to(hit_props.secondary_element)
                {
                    // not super effective
                    return;
                }

                // deactivate form
                let Some(index) = player.active_form.take() else {
                    return;
                };

                simulation.time_freeze_tracker.start_decross();

                let form = &mut player.forms[index];
                form.deactivated = true;

                if let Some(callback) = form.deactivate_callback.clone() {
                    simulation.pending_callbacks.push(callback);
                }

                // spawn shine fx
                let mut full_position = entity.full_position();
                full_position.offset += Vec2::new(0.0, -entity.height * 0.5);

                let shine_id = simulation.create_transformation_shine(game_io);
                let shine_entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(shine_id.into())
                    .unwrap();

                shine_entity.copy_full_position(full_position);
                shine_entity.pending_spawn = true;

                // play revert sfx
                let revert_sfx = &game_io.resource::<Globals>().unwrap().transform_revert_sfx;
                simulation.play_sound(game_io, revert_sfx);
            },
        ));

        // resolve health boost
        // todo: move to Augment?
        let grid = BlockGrid::new(namespace).with_blocks(game_io, blocks);

        let health_boost = grid
            .valid_packages(game_io)
            .fold(0, |acc, package| acc + package.health_boost);

        living.max_health = setup.base_health + health_boost;
        living.set_health(setup.health);

        // insert entity
        self.entities
            .insert(
                id.into(),
                (Player::new(game_io, index, local, charge_index, cards),),
            )
            .unwrap();

        // call init function
        let package_info = &player_package.package_info;
        let vm_index = Self::find_vm(vms, &package_info.id, package_info.namespace)?;

        self.call_global(game_io, vms, vm_index, "player_init", move |lua| {
            create_entity_table(lua, id)
        })?;

        // init blocks
        for package in grid.valid_packages(game_io) {
            let package_info = &package.package_info;
            let vm_index = Self::find_vm(vms, &package_info.id, package_info.namespace)?;

            let player = self
                .entities
                .query_one_mut::<&mut Player>(id.into())
                .unwrap();
            let index = player.modifiers.insert(Augment::from(package));

            let lua = &vms[vm_index].lua;
            let has_init = lua
                .globals()
                .contains_key("augment_init")
                .unwrap_or_default();

            if has_init {
                let result = self.call_global(game_io, vms, vm_index, "augment_init", move |lua| {
                    create_augment_table(lua, id, index)
                });

                if let Err(e) = result {
                    log::error!("{e}");
                }
            }
        }

        Ok(id)
    }

    pub fn load_character(
        &mut self,
        game_io: &GameIO,
        vms: &[RollbackVM],
        package_id: &PackageId,
        namespace: PackageNamespace,
        rank: CharacterRank,
    ) -> rollback_mlua::Result<EntityId> {
        let id = self.create_character(game_io, rank, namespace)?;

        let vm_index = Self::find_vm(vms, package_id, namespace)?;
        self.call_global(game_io, vms, vm_index, "character_init", move |lua| {
            create_entity_table(lua, id)
        })?;

        Ok(id)
    }

    pub fn create_artifact(&mut self, game_io: &GameIO) -> EntityId {
        let id = self.create_entity(game_io);

        self.entities
            .insert(id.into(), (Artifact::default(),))
            .unwrap();

        let entity = self
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.ignore_hole_tiles = true;
        entity.ignore_tile_effects = true;
        entity.can_move_to_callback = BattleCallback::stub(true);

        id
    }

    pub fn create_spell(&mut self, game_io: &GameIO) -> EntityId {
        let id = self.create_entity(game_io);

        self.entities
            .insert(id.into(), (Spell::default(),))
            .unwrap();

        let entity = self
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.ignore_hole_tiles = true;
        entity.ignore_tile_effects = true;
        entity.can_move_to_callback = BattleCallback::stub(true);

        id
    }

    pub fn create_obstacle(&mut self, game_io: &GameIO) -> EntityId {
        let id = self.create_entity(game_io);

        let entity = self
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        // obstacles should own their tile by default
        entity.share_tile = false;
        entity.auto_reserves_tiles = true;

        self.entities
            .insert(
                id.into(),
                (Spell::default(), Obstacle::default(), Living::default()),
            )
            .unwrap();

        id
    }

    pub fn create_animated_artifact(
        &mut self,
        game_io: &GameIO,
        texture_path: &str,
        animation_path: &str,
    ) -> EntityId {
        let id = self.create_artifact(game_io);

        let entity = self
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        // load texture
        let texture_string = ResourcePaths::absolute(texture_path);
        let sprite_root = entity.sprite_tree.root_mut();
        sprite_root.set_texture(game_io, texture_string);

        // load animation
        let animator = &mut self.animators[entity.animator_index];
        let _ = animator.load(game_io, animation_path);
        let _ = animator.set_state("DEFAULT");

        // delete when the animation completes
        animator.on_complete(BattleCallback::new(move |_, simulation, _, _| {
            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(id.into())
                .unwrap();
            entity.erased = true;
        }));

        id
    }

    pub fn create_explosion(&mut self, game_io: &GameIO) -> EntityId {
        let id = self.create_animated_artifact(
            game_io,
            ResourcePaths::BATTLE_EXPLOSION,
            ResourcePaths::BATTLE_EXPLOSION_ANIMATION,
        );

        let entity = self
            .entities
            .query_one_mut::<&mut Entity>(id.into())
            .unwrap();

        entity.spawn_callback = BattleCallback::new(|game_io, simulation, _, _| {
            simulation.play_sound(game_io, &game_io.resource::<Globals>().unwrap().explode_sfx);
        });

        id
    }

    pub fn create_transformation_shine(&mut self, game_io: &GameIO) -> EntityId {
        self.create_animated_artifact(
            game_io,
            ResourcePaths::BATTLE_TRANSFORM_SHINE,
            ResourcePaths::BATTLE_TRANSFORM_SHINE_ANIMATION,
        )
    }

    pub fn create_splash(&mut self, game_io: &GameIO) -> EntityId {
        self.create_animated_artifact(
            game_io,
            ResourcePaths::BATTLE_SPLASH,
            ResourcePaths::BATTLE_SPLASH_ANIMATION,
        )
    }

    pub fn draw(&mut self, game_io: &mut GameIO, render_pass: &mut RenderPass) {
        let mut blind_filter = None;

        // resolve perspective
        if let Ok((entity, living)) =
            (self.entities).query_one_mut::<(&Entity, &Living)>(self.local_player_id.into())
        {
            //Bool assignment. If entity is team blue set the fact that they're perspective flipped to true. -Dawn
            self.perspective_flipped = entity.team == Team::Blue;

            if living.status_director.remaining_status_time(HitFlag::BLIND) > 0 {
                blind_filter = Some(entity.team);
            }
        }

        let assets = &game_io.resource::<Globals>().unwrap().assets;

        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::default());

        // draw field
        self.field
            .draw(game_io, &mut sprite_queue, self.perspective_flipped);

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

        // reusing vec to avoid realloctions
        let mut sprite_nodes_recycled = Vec::new();

        for (id, _) in sorted_entities {
            let entity = self.entities.query_one_mut::<&mut Entity>(id).unwrap();
            let mut sprite_nodes = sprite_nodes_recycled;

            // offset for calculating initial placement position
            let mut offset: Vec2 = entity.corrected_offset(self.perspective_flipped);

            // elevation
            offset.y -= entity.elevation;
            let shadow_node = &mut entity.sprite_tree[entity.shadow_index];
            shadow_node.set_offset(Vec2::new(shadow_node.offset().x, entity.elevation));

            let tile_center =
                (self.field).calc_tile_center((entity.x, entity.y), self.perspective_flipped);
            let initial_position = tile_center + offset;

            // true if only one is true, since flipping twice causes us to no longer be flipped
            let flipped = self.perspective_flipped ^ entity.flipped();

            // offset each child by parent node accounting for perspective
            (entity.sprite_tree).inherit_from_parent(initial_position, flipped);

            // capture root values before mutable reference
            let root_node = entity.sprite_tree.root();
            let root_palette = root_node.palette().cloned();
            let root_color_mode = root_node.color_mode();
            let root_color = root_node.color();

            // sort nodes
            sprite_nodes.extend(entity.sprite_tree.values_mut());
            sprite_nodes.sort_by_key(|node| -node.layer());

            // draw nodes
            for node in sprite_nodes.iter_mut() {
                if !node.inherited_visible() {
                    // could possibly filter earlier,
                    // but not expecting huge trees of invisible nodes
                    continue;
                }

                // resolve shader
                let palette;
                let color_mode;
                let color;
                let original_color = node.color();

                if node.using_parent_shader() {
                    palette = root_palette.clone();
                    color_mode = root_color_mode;
                    color = root_color;
                } else {
                    palette = node.palette().cloned();
                    color_mode = node.color_mode();
                    color = node.color();
                }

                if let Some(texture) = palette {
                    sprite_queue.set_shader_effect(SpriteShaderEffect::Palette);
                    sprite_queue.set_palette(texture);
                } else {
                    sprite_queue.set_shader_effect(SpriteShaderEffect::Default);
                }

                sprite_queue.set_color_mode(color_mode);
                node.set_color(color);

                // finally drawing the sprite
                sprite_queue.draw_sprite(node.sprite());

                node.set_color(original_color);
            }

            sprite_nodes_recycled = recycle_vec(sprite_nodes);
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

                let tile_position = (entity.x, entity.y);
                let tile_center =
                    (self.field).calc_tile_center(tile_position, self.perspective_flipped);

                let entity_offset = entity.corrected_offset(self.perspective_flipped);

                hp_text.text = living.health.to_string();
                let text_size = hp_text.measure().size;

                (hp_text.style.bounds).set_position(tile_center + entity_offset);
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

            let offset = entity.corrected_offset(self.perspective_flipped);
            let tile_center =
                (self.field).calc_tile_center((entity.x, entity.y), self.perspective_flipped);

            let base_position = tile_center + vec2(offset.x, -entity.height - 16.0);

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
    }

    #[cfg(debug_assertions)]
    fn assertions(&mut self) {
        // make sure card actions are being deleted
        let held_action_count = self
            .entities
            .query_mut::<&Entity>()
            .into_iter()
            .filter(|(_, entity)| entity.card_action_index.is_some())
            .count();

        let executed_action_count = self
            .card_actions
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

#[allow(clippy::needless_lifetimes)]
pub fn recycle_vec<'a, 'b, T: ?Sized>(mut data: Vec<&'a mut T>) -> Vec<&'b mut T> {
    data.clear();
    unsafe { std::mem::transmute(data) }
}
