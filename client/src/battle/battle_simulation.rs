use super::external_events::ExternalEvent;
use super::*;
use crate::bindable::*;
use crate::lua_api::{call_encounter_end_listeners, pass_server_message};
use crate::packages::{CardPackage, PackageNamespace};
use crate::render::ui::{BattleBannerPopup, FontName, PlayerHealthUi, Text};
use crate::render::*;
use crate::resources::*;
use crate::scenes::BattleEvent;
use crate::structures::{DenseSlotMap, SlotMap};
use framework::prelude::*;
use packets::structures::{BattleStatistics, BattleSurvivor};
use packets::NetplaySignal;
use rand::SeedableRng;
use rand_xoshiro::Xoshiro256PlusPlus;
use std::cell::RefCell;

crate::structures::sequential_enum! {
    pub BattleProgress: u8 {
        Initializing,
        CompletedIntro,
        BattleStarted,
        BattleEnded,
        Exiting
    }
}

pub struct BattleSimulation {
    pub statistics: BattleStatistics,
    pub rng: Xoshiro256PlusPlus,
    pub inputs: Vec<PlayerInput>,
    pub time: FrameTime,
    pub battle_time: FrameTime,
    pub camera: Camera,
    pub background: Background,
    pub turn_gauge: TurnGauge,
    pub banner_popups: SlotMap<BattleBannerPopup>,
    pub field: Field,
    pub tile_states: Vec<TileState>,
    pub entities: hecs::World,
    pub generation_tracking: Vec<u32>,
    pub ownership_tracking: OwnershipTracking,
    pub queued_attacks: Vec<AttackBox>,
    pub defense: Defense,
    pub sprite_trees: SlotMap<Tree<SpriteNode>>,
    pub animators: SlotMap<BattleAnimator>,
    pub actions: DenseSlotMap<Action>,
    pub time_freeze_tracker: TimeFreezeTracker,
    pub components: DenseSlotMap<Component>,
    pub pending_callbacks: Vec<BattleCallback>,
    pub external_outbox: Vec<String>,
    pub local_player_index: usize,
    pub local_player_id: EntityId,
    pub local_health_ui: PlayerHealthUi,
    pub local_team: Team,
    pub progress: BattleProgress,
    pub is_resimulation: bool,
}

impl BattleSimulation {
    pub fn new(game_io: &GameIO, meta: &BattleMeta) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(BATTLE_CAMERA_OFFSET);

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        fade_sprite.set_color(Color::TRANSPARENT);

        let mut sprite_trees: SlotMap<Tree<SpriteNode>> = Default::default();
        // root hud node
        sprite_trees.insert(Tree::new(SpriteNode::new(game_io, Default::default())));

        Self {
            statistics: BattleStatistics::new(),
            rng: Xoshiro256PlusPlus::seed_from_u64(meta.seed),
            time: 0,
            battle_time: 0,
            inputs: vec![PlayerInput::new(); meta.player_count],
            camera,
            background: meta.background.clone(),
            turn_gauge: TurnGauge::new(game_io),
            banner_popups: Default::default(),
            field: Field::new(game_io),
            tile_states: TileState::create_registry(game_io),
            entities: hecs::World::new(),
            generation_tracking: Vec::new(),
            ownership_tracking: Default::default(),
            queued_attacks: Vec::new(),
            defense: Defense::new(),
            sprite_trees,
            animators: Default::default(),
            actions: Default::default(),
            time_freeze_tracker: TimeFreezeTracker::new(),
            components: Default::default(),
            pending_callbacks: Vec::new(),
            external_outbox: Default::default(),
            local_player_index: meta
                .player_setups
                .iter()
                .find(|setup| setup.local)
                .map(|setup| setup.index)
                .unwrap_or_default(),
            local_player_id: EntityId::DANGLING,
            local_health_ui: PlayerHealthUi::new(game_io),
            local_team: Team::Unset,
            progress: BattleProgress::Initializing,
            is_resimulation: false,
        }
    }

    pub fn clone(&mut self, game_io: &GameIO) -> Self {
        let mut entities = hecs::World::new();

        // spawn + remove blank entities to restore generations on dead entities
        // otherwise if there's no living entity holding an id
        // a new entity can spawn reusing an id a script may be tracking
        // the most obvious sign of this is seeing the camera flip after a player dies and enemy spawns
        // (enemy reuses the player id, making the engine think the player changed teams)
        for (i, generation) in self.generation_tracking.iter().cloned().enumerate() {
            let id = hecs::Entity::from_bits(((generation as u64) << 32) | i as u64).unwrap();
            entities.spawn_at(id, ());
            let _ = entities.despawn(id);
        }

        // starting with Entity as every entity will have Entity
        for (id, entity) in self.entities.query_mut::<&Entity>() {
            entities.spawn_at(id, (entity.clone(),));
        }

        // cloning every component
        macro_rules! clone_component {
            ($($component: ty),*) => {
                $(for (id, component) in self.entities.query_mut::<&$component>() {
                    let _ = entities.insert_one(id, component.clone());
                })+
            };
        }

        clone_component!(Artifact, Character, Living, Obstacle, Player, Spell);
        clone_component!(EntityName, EntityShadow, EntityShadowHidden, HpDisplay);
        clone_component!(ActionQueue, AttackContext, Movement);
        clone_component!(SpawnCallback, IntroCallback, UpdateCallback, DeleteCallback);
        clone_component!(CanMoveToCallback, IdleCallback);
        clone_component!(CollisionCallback, AttackCallback);
        clone_component!(CounterCallback, CounteredCallback);
        clone_component!(BattleStartCallback, BattleEndCallback);

        Self {
            statistics: self.statistics.clone(),
            inputs: self.inputs.clone(),
            rng: self.rng.clone(),
            time: self.time,
            battle_time: self.battle_time,
            camera: self.camera.clone(game_io),
            background: self.background.clone(),
            turn_gauge: self.turn_gauge.clone(),
            banner_popups: self.banner_popups.clone(),
            field: self.field.clone(),
            tile_states: self.tile_states.clone(),
            entities,
            generation_tracking: self.generation_tracking.clone(),
            ownership_tracking: self.ownership_tracking.clone(),
            queued_attacks: self.queued_attacks.clone(),
            defense: self.defense,
            sprite_trees: self.sprite_trees.clone(),
            animators: self.animators.clone(),
            actions: self.actions.clone(),
            time_freeze_tracker: self.time_freeze_tracker.clone(),
            components: self.components.clone(),
            pending_callbacks: self.pending_callbacks.clone(),
            // the outbox isn't shared between frames, and is used once canonicalized / synced
            external_outbox: Default::default(),
            local_player_index: self.local_player_index,
            local_player_id: self.local_player_id,
            local_health_ui: self.local_health_ui.clone(),
            local_team: self.local_team,
            progress: self.progress,
            is_resimulation: self.is_resimulation,
        }
    }

    pub fn play_sound(&self, game_io: &GameIO, sound_buffer: &SoundBuffer) {
        if !self.is_resimulation {
            let globals = game_io.resource::<Globals>().unwrap();
            globals.audio.play_sound(sound_buffer);
        }
    }

    pub fn play_sound_with_behavior(
        &self,
        game_io: &GameIO,
        sound_buffer: &SoundBuffer,
        behavior: AudioBehavior,
    ) {
        if !self.is_resimulation {
            let globals = game_io.resource::<Globals>().unwrap();
            let audio = &globals.audio;
            audio.play_sound_with_behavior(sound_buffer, behavior);
        }
    }

    pub fn stop_music(&self, game_io: &GameIO, resources: &SharedBattleResources) {
        let globals = game_io.resource::<Globals>().unwrap();

        if globals.audio.music_stack_len() != resources.music_stack_depth {
            return;
        }

        globals.audio.stop_music();
    }

    pub fn play_music(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        sound_buffer: &SoundBuffer,
        loops: bool,
    ) {
        let globals = game_io.resource::<Globals>().unwrap();

        if globals.audio.music_stack_len() != resources.music_stack_depth {
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

        for (id, (entity, living, name)) in
            entities.query_mut::<(&Entity, &Living, Option<&EntityName>)>()
        {
            let entity_id: EntityId = id.into();

            if entity_id != self.local_player_id {
                continue;
            }

            let survivor_list = if entity.team == self.local_team {
                &mut self.statistics.ally_survivors
            } else if entity.team != Team::Other {
                &mut self.statistics.enemy_survivors
            } else {
                &mut self.statistics.neutral_survivors
            };

            let name = name.map(|n| n.0.clone()).unwrap_or_default();

            survivor_list.push(BattleSurvivor {
                name: name.to_string(),
                health: living.health,
            });
        }

        self.statistics.calculate_score();
    }

    pub fn handle_local_signals(&mut self, local_index: usize, resources: &SharedBattleResources) {
        if self.is_resimulation {
            return;
        }

        let input = &mut self.inputs[local_index];

        // handle flee
        if input.has_signal(NetplaySignal::AttemptingFlee) {
            // todo: check for success using some method in battle_init, as long as the player is not a spectator
            resources
                .event_sender
                .send(BattleEvent::FleeResult(true))
                .unwrap();
        }

        // allow spectators to flee
        if self.local_player_id == EntityId::default() && input.was_just_pressed(Input::Pause) {
            let event = BattleEvent::RequestFlee;
            resources.event_sender.send(event).unwrap();
        }
    }

    fn handle_external_signals(&mut self, game_io: &GameIO, resources: &SharedBattleResources) {
        for event in resources.external_events.events_for_time(self.time) {
            match event {
                ExternalEvent::ServerMessage(message) => {
                    pass_server_message(game_io, resources, self, message);
                }
            }
        }
    }

    pub fn pre_update(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        state: &dyn State,
    ) {
        #[cfg(debug_assertions)]
        self.assertions();

        // remove dead entities
        self.cleanup_erased_entities();

        // update background
        self.background.update();

        self.update_camera();

        // reset fade colors
        resources.ui_fade_color.set(Color::TRANSPARENT);
        resources.battle_fade_color.set(Color::TRANSPARENT);

        if state.allows_animation_updates() {
            // update animations
            self.update_animations(resources);
        }

        // process disconnects
        self.process_disconnects();

        // spawn pending entities
        self.spawn_pending();

        // apply animations after spawning to display frame 0
        self.apply_animations();

        // animation + spawn callbacks
        self.call_pending_callbacks(game_io, resources);

        self.handle_external_signals(game_io, resources);
    }

    pub fn post_update(&mut self, game_io: &GameIO, resources: &SharedBattleResources) {
        // update scene components
        self.update_components(game_io, resources, ComponentLifetime::Scene);

        self.update_sync_nodes();

        // should this be called every time we invoke lua?
        self.call_pending_callbacks(game_io, resources);

        self.update_ui();

        self.field.update_animations();

        self.time += 1;
    }

    pub fn update_animations(&mut self, resources: &SharedBattleResources) {
        let status_registry = &resources.status_registry;
        let time_is_frozen = self.time_freeze_tracker.time_is_frozen();

        for (id, entity) in self.entities.query::<&Entity>().into_iter() {
            if entity.time_frozen {
                continue;
            }

            if let Some(living) = self.entities.query_one::<&Living>(id).unwrap().get() {
                if living.status_director.is_inactionable(status_registry) {
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

            if entity.time_frozen || (time_is_frozen && !action.properties.time_freeze) {
                continue;
            }

            if let Ok(living) = self.entities.query_one_mut::<&Living>(action.entity.into()) {
                if living.status_director.is_inactionable(status_registry) {
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
        if self.progress == BattleProgress::Exiting {
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
            Component::create_delayed_deleter(self, id.into(), ComponentLifetime::Battle, 0);
        }
    }

    fn spawn_pending(&mut self) {
        type Query<'a> = (
            &'a mut Entity,
            Option<&'a ActionQueue>,
            Option<&'a SpawnCallback>,
            Option<&'a BattleStartCallback>,
        );

        for (id, (entity, action_queue, spawn_callback, battle_start_callback)) in
            self.entities.query_mut::<Query>()
        {
            if !entity.pending_spawn {
                continue;
            }

            let entity_id = id.into();

            entity.pending_spawn = false;
            entity.spawned = true;
            entity.on_field = true;

            self.animators[entity.animator_index].enable();

            if let Some(callback) = spawn_callback {
                self.pending_callbacks.push(callback.0.clone());
            }

            for component in self.components.values() {
                if component.entity == entity_id {
                    self.pending_callbacks.push(component.init_callback.clone());
                }
            }

            if self.progress >= BattleProgress::BattleStarted {
                if let Some(callback) = battle_start_callback {
                    self.pending_callbacks.push(callback.0.clone());
                }
            }

            if let Some(tile) = self.field.tile_at_mut((entity.x, entity.y)) {
                tile.handle_auto_reservation_addition(
                    &self.actions,
                    entity_id,
                    entity,
                    action_queue,
                );

                if entity.team == Team::Unset {
                    entity.team = tile.team();
                }

                if entity.facing == Direction::None {
                    entity.facing = tile.direction();
                }

                let tile_state = &self.tile_states[tile.state_index()];
                let tile_callback = tile_state.entity_enter_callback.clone();
                self.pending_callbacks.push(tile_callback.bind(entity_id));
            }
        }
    }

    fn apply_animations(&mut self) {
        for (_, entity) in self.entities.query_mut::<&mut Entity>() {
            let Some(sprite_tree) = self.sprite_trees.get_mut(entity.sprite_tree_index) else {
                continue;
            };
            let sprite_node = sprite_tree.root_mut();

            // update root sprite
            self.animators[entity.animator_index].apply(sprite_node);
        }

        // update attachment sprites
        // separate loop from entities to account for async actions
        for action in self.actions.values_mut() {
            if !action.executed {
                continue;
            }

            let entities = &mut self.entities;
            let Ok(entity) = entities.query_one_mut::<&mut Entity>(action.entity.into()) else {
                continue;
            };

            let Some(sprite_tree) = self.sprite_trees.get_mut(entity.sprite_tree_index) else {
                continue;
            };

            for attachment in &mut action.attachments {
                attachment.apply_animation(sprite_tree, &mut self.animators);
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
                &mut self.sprite_trees,
                &mut self.pending_callbacks,
                animator_index,
            );
        }
    }

    pub fn update_components(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        lifetime: ComponentLifetime,
    ) {
        for (_, component) in &self.components {
            if component.lifetime == lifetime {
                let callback = component.update_callback.clone();
                self.pending_callbacks.push(callback);
            }
        }

        self.call_pending_callbacks(game_io, resources);
    }

    pub fn call_pending_callbacks(&mut self, game_io: &GameIO, resources: &SharedBattleResources) {
        let callbacks = std::mem::take(&mut self.pending_callbacks);

        for callback in callbacks {
            callback.call(game_io, resources, self, ());
        }
    }

    fn update_camera(&mut self) {
        self.camera.update();

        let scale = self.field.best_fitting_scale();

        if scale == Vec2::ONE {
            // no need to manage the camera if we can fit everything on screen
            return;
        }

        // resolve field size
        let tile_size = self.field.tile_size();
        let field_visible_tile_count =
            (Vec2::new(self.field.cols() as _, self.field.rows() as _) - 2.0).max(Vec2::ONE);
        let field_size = field_visible_tile_count * tile_size;
        let half_field_width = field_size.x * 0.5;

        // prevent the camera from escaping the field if we can
        let shift_for_zoom = |mut position: Vec2, zoom: f32| {
            let half_zoom_width = RESOLUTION_F.x * 0.5 / zoom;

            let lower_bound = (-half_field_width + half_zoom_width).ceil();
            let higher_bound = (half_field_width - half_zoom_width).floor();

            if lower_bound > higher_bound {
                position.x = 0.0;
            } else {
                position.x = position.x.clamp(lower_bound, higher_bound);
            }

            position
        };

        if field_size.x >= RESOLUTION_F.x {
            let current_position = self.camera.position();
            let current_zoom = self.camera.scale().x;
            let position = shift_for_zoom(current_position, current_zoom);
            self.camera.snap(position);
        };

        // resolve the smallest box that can fit every character
        let mut min_x = f32::INFINITY;
        let mut max_x = f32::NEG_INFINITY;
        let mut min_y = f32::INFINITY;
        let mut max_y = f32::NEG_INFINITY;

        let entities = &mut self.entities;
        let perspective_flipped = self.local_team.flips_perspective();
        let mut count = 0;

        for (_, (entity, _)) in entities.query_mut::<(&Entity, &Character)>() {
            if entity.spawned {
                let mut position = entity.screen_position(&self.field, perspective_flipped);

                // reapply a limited vertical movement offset, to avoid rapid screen movement from :jump()s
                let vertical_movement_offset = entity.movement_offset.y;
                position.y -= vertical_movement_offset;
                position.y += vertical_movement_offset.max(-self.field.tile_size().y);

                min_x = position.x.min(min_x);
                max_x = position.x.max(max_x);
                min_y = position.y.min(min_y);
                max_y = position.y.max(max_y);
                count += 1;
            }
        }

        if count == 0 {
            let scale = self.field.best_fitting_scale();
            self.camera.slide(scale, BATTLE_PAN_MOTION);
            self.camera.zoom(BATTLE_CAMERA_OFFSET, BATTLE_ZOOM_MOTION);
            return;
        }

        // pad the box
        let padding = tile_size * BATTLE_CAMERA_TILE_PADDING;
        min_x -= padding.x;
        max_x += padding.x;
        min_y -= padding.y * 3.0; // extra padding for UI
        max_y += padding.y;

        // clip the box to the field (only on the x axis)
        min_x = min_x.max(-half_field_width);
        max_x = max_x.min(half_field_width);

        // calculate center
        let fit_width = max_x - min_x;
        let fit_height = max_y - min_y;
        let mut center = Vec2::new(min_x + fit_width * 0.5, min_y + fit_height * 0.5);

        // calculate zoom
        let fit_zoom = (RESOLUTION_F / Vec2::new(fit_width, fit_height))
            .min_element()
            .min(1.0);

        // adjust center
        if field_size.x >= RESOLUTION_F.x {
            center = shift_for_zoom(center, fit_zoom);
        };

        self.camera.slide(center, BATTLE_PAN_MOTION);
        self.camera.zoom(Vec2::splat(fit_zoom), BATTLE_ZOOM_MOTION);
    }

    fn cleanup_erased_entities(&mut self) {
        let mut pending_removal = Vec::new();
        let mut components_pending_removal = Vec::new();
        let mut actions_pending_removal = Vec::new();

        for (id, entity) in self.entities.query_mut::<&Entity>() {
            if !entity.erased {
                continue;
            }

            let entity_id = id.into();
            self.field.drop_entity(entity_id);

            for (index, component) in &mut self.components {
                if component.entity == entity_id {
                    components_pending_removal.push(index);
                }
            }

            for (index, action) in &mut self.actions {
                if action.entity == entity_id {
                    actions_pending_removal.push(index);
                }
            }

            self.sprite_trees.remove(entity.sprite_tree_index);
            self.animators.remove(entity.animator_index);

            pending_removal.push(id);
        }

        for id in pending_removal {
            // delete shadow
            EntityShadow::delete(self, id.into());

            // delete entity
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

        // update player ui
        if let Ok((player, living)) =
            entities.query_one_mut::<(&mut Player, &Living)>(self.local_player_id.into())
        {
            self.local_health_ui.set_health(living.health);
            self.local_health_ui.set_max_health(living.max_health);
            player.emotion_window.update();
        } else {
            self.local_health_ui.set_health(0);
        }

        self.local_health_ui.update();

        // update battle hp displays
        HpDisplay::update(self);

        // update banners
        self.banner_popups.retain(|_, banner| {
            banner.update();
            banner.remaining_time() != Some(0)
        });
    }

    pub fn is_entity_actionable(
        &mut self,
        resources: &SharedBattleResources,
        entity_id: EntityId,
    ) -> bool {
        let entities = &mut self.entities;

        let time_is_frozen = self.time_freeze_tracker.time_is_frozen();

        if time_is_frozen {
            let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                return false;
            };

            if entity.time_frozen {
                return false;
            }
        }

        if let Ok(living) = entities.query_one_mut::<&Living>(entity_id.into()) {
            let status_registry = &resources.status_registry;
            return !living.status_director.is_inactionable(status_registry);
        };

        true
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

    pub fn mark_battle_end(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        won: bool,
    ) {
        if self.progress >= BattleProgress::BattleEnded {
            return;
        }

        self.progress = BattleProgress::BattleEnded;
        self.statistics.won = won;

        call_encounter_end_listeners(game_io, resources, self, won);

        for (_, callback) in self.entities.query_mut::<&mut BattleEndCallback>() {
            let callback = callback.0.clone();
            self.pending_callbacks
                .push(callback.bind(self.statistics.won));
        }

        self.call_pending_callbacks(game_io, resources);
    }

    pub fn call_global<'lua, F, M>(
        &mut self,
        game_io: &GameIO,
        resources: &'lua SharedBattleResources,
        vm_index: usize,
        fn_name: &str,
        param_generator: F,
    ) -> rollback_mlua::Result<()>
    where
        F: FnOnce(&'lua rollback_mlua::Lua) -> rollback_mlua::Result<M>,
        M: rollback_mlua::IntoLuaMulti<'lua>,
    {
        let vms = resources.vm_manager.vms();
        let lua = &vms[vm_index].lua;

        let global_fn: rollback_mlua::Function = lua
            .globals()
            .get(fn_name)
            .map_err(|e| rollback_mlua::Error::RuntimeError(format!("{e}: {fn_name}")))?;

        let api_ctx = RefCell::new(BattleScriptContext {
            vm_index,
            resources,
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

    pub fn draw(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        render_pass: &mut RenderPass,
        draw_player_indices: bool,
    ) {
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

        // store whether the player's team flips the perspective into a shorter variable
        let perspective_flipped = self.local_team.flips_perspective();

        let assets = &game_io.resource::<Globals>().unwrap().assets;

        // draw background
        self.background.draw(game_io, render_pass);

        let mut sprite_queue =
            SpriteColorQueue::new(game_io, &self.camera, SpriteColorMode::default());

        // draw field
        self.field.draw(
            game_io,
            &mut sprite_queue,
            &mut self.tile_states,
            perspective_flipped,
        );

        // draw battle fade color
        let fade_color = resources.battle_fade_color.get();
        resources.draw_fade_sprite(&mut sprite_queue, fade_color);

        // draw entities, sorting by tile, movement offset, and layer
        let mut sorted_entities = Vec::with_capacity(self.entities.len() as usize);

        // filter characters + obstacles for blindness
        for (id, entity) in self.entities.query_mut::<hecs::With<&Entity, &Living>>() {
            if blind_filter.is_some() && blind_filter != Some(entity.team) {
                continue;
            }

            let Some(sprite_tree) = self.sprite_trees.get_mut(entity.sprite_tree_index) else {
                continue;
            };

            if entity.on_field && sprite_tree.root().visible() {
                sorted_entities.push((id, entity.sort_key(&self.sprite_trees)));
            }
        }

        // add everything else
        for (id, entity) in self.entities.query_mut::<hecs::Without<&Entity, &Living>>() {
            let Some(sprite_tree) = self.sprite_trees.get_mut(entity.sprite_tree_index) else {
                continue;
            };

            if entity.on_field && sprite_tree.root().visible() {
                sorted_entities.push((id, entity.sort_key(&self.sprite_trees)));
            }
        }

        sorted_entities.sort_by_key(|(_, key)| *key);

        // calculations and shadow rendering
        let mut entity_tree_render_params = Vec::with_capacity(sorted_entities.len());

        for (id, _) in sorted_entities {
            let (entity, movement, shadow, shadow_hidden) = self
                .entities
                .query_one_mut::<(
                    &mut Entity,
                    Option<&Movement>,
                    Option<&EntityShadow>,
                    Option<&EntityShadowHidden>,
                )>(id)
                .unwrap();

            // start at tile center
            let mut position =
                (self.field).calc_tile_center((entity.x, entity.y), perspective_flipped);

            // apply the entity offset
            position += entity.corrected_offset(perspective_flipped);

            // apply elevation
            position.y -= entity.elevation;

            // true if only one is true, since flipping twice causes us to no longer be flipped
            let flipped = perspective_flipped ^ entity.flipped();

            // render shadow
            if let (Some(shadow), None) = (shadow, shadow_hidden) {
                let mut shadow_y = entity.elevation;

                if let Some(movement) = movement {
                    let progress = movement.animation_progress_percent();
                    shadow_y += movement.interpolate_jump_height(progress);
                }

                let base_sprite = self
                    .sprite_trees
                    .get_mut(entity.sprite_tree_index)
                    .unwrap()
                    .root_mut();
                let color_mode = base_sprite.color_mode();
                let color = base_sprite.color();
                let shader_effect = base_sprite.shader_effect();

                let shadow_tree = self.sprite_trees.get_mut(shadow.sprite_tree_index).unwrap();
                let mut position = position;
                position.y += shadow_y;

                let sprite = shadow_tree.root_mut();
                sprite.set_color_mode(color_mode);
                sprite.set_color(color);
                sprite.set_shader_effect(shader_effect);
                shadow_tree.draw_with_offset(&mut sprite_queue, position, flipped);
            }

            entity_tree_render_params.push((entity.sprite_tree_index, position, flipped));
        }

        // rendering the entity sprite trees after shadows are drawn
        for (tree_index, position, flipped) in entity_tree_render_params {
            if let Some(sprite_tree) = self.sprite_trees.get_mut(tree_index) {
                sprite_tree.draw_with_offset(&mut sprite_queue, position, flipped);
            }
        }

        sprite_queue.set_shader_effect(SpriteShaderEffect::Default);

        // draw hp on living entities
        if self.progress >= BattleProgress::CompletedIntro {
            let mut hp_text = Text::new(game_io, FontName::EntityHp);
            hp_text.style.letter_spacing = 0.0;
            let tile_size = self.field.tile_size();

            for (id, (hp_display, entity)) in self.entities.query_mut::<(&HpDisplay, &Entity)>() {
                let entity_id: EntityId = id.into();

                if entity.deleted
                    || !entity.on_field
                    || !hp_display.initialized
                    || hp_display.value <= 0
                    || entity_id == self.local_player_id
                {
                    continue;
                }

                let Some(sprite_tree) = self.sprite_trees.get_mut(entity.sprite_tree_index) else {
                    continue;
                };

                if !sprite_tree.root().visible() {
                    continue;
                }

                if blind_filter.is_some() && blind_filter != Some(entity.team) {
                    // blindness filter
                    continue;
                }

                // resolve font / color
                hp_text.style.font = match hp_display.value.cmp(&hp_display.last_value) {
                    std::cmp::Ordering::Less => FontName::EntityHpRed,
                    std::cmp::Ordering::Equal => FontName::EntityHp,
                    std::cmp::Ordering::Greater => FontName::EntityHpGreen,
                };

                // resolve text
                hp_text.text = hp_display.value.to_string();

                // resolve position
                let entity_screen_position =
                    entity.screen_position(&self.field, perspective_flipped);

                hp_text.style.bounds.set_position(entity_screen_position);
                let text_size = hp_text.measure().size;
                hp_text.style.bounds.x -= text_size.x * 0.5;
                hp_text.style.bounds.y += tile_size.y * 0.5 - text_size.y;

                hp_text.draw(game_io, &mut sprite_queue);
            }
        }

        // draw player indices
        if draw_player_indices {
            let mut index_text = Text::new(game_io, FontName::Code);
            index_text.style.color = Color::GREEN;
            index_text.style.shadow_color = Color::BLACK;

            for (_, (entity, player)) in self.entities.query_mut::<(&Entity, &Player)>() {
                if !entity.on_field {
                    continue;
                }

                let Some(sprite_tree) = self.sprite_trees.get_mut(entity.sprite_tree_index) else {
                    continue;
                };

                if !sprite_tree.root().visible() {
                    continue;
                }

                if blind_filter.is_some() && blind_filter != Some(entity.team) {
                    // blindness filter
                    continue;
                }

                let entity_screen_position =
                    entity.screen_position(&self.field, perspective_flipped);

                index_text.text = player.index.to_string();
                let text_size = index_text.measure().size;

                (index_text.style.bounds).set_position(entity_screen_position);
                index_text.style.bounds.x -= text_size.x * 0.5;
                index_text.style.bounds.y -= text_size.y;
                index_text.draw(game_io, &mut sprite_queue);
            }
        }

        // draw card icons for the local player
        if let Ok((entity, character)) =
            (self.entities).query_one_mut::<(&Entity, &Character)>(self.local_player_id.into())
        {
            let sprite_tree = self.sprite_trees.get(entity.sprite_tree_index);

            if !entity.deleted && entity.on_field && sprite_tree.is_some_and(|t| t.root().visible())
            {
                sprite_queue.set_color_mode(SpriteColorMode::Multiply);

                let mut base_position = entity.screen_position(&self.field, perspective_flipped);
                base_position.y -= entity.height + 16.0;

                let mut border_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
                border_sprite.set_color(Color::BLACK);
                border_sprite.set_size(Vec2::new(16.0, 16.0));

                for i in 0..character.cards.len() {
                    let card = &character.cards[i];

                    let cards_before = character.cards.len() - i;
                    let card_offset = 2.0 * cards_before as f32;
                    let position = base_position - card_offset;

                    border_sprite.set_position(position - 1.0);
                    sprite_queue.draw_sprite(&border_sprite);

                    CardPackage::draw_icon(
                        game_io,
                        &mut sprite_queue,
                        card.namespace.unwrap_or(PackageNamespace::Local),
                        &card.package_id,
                        position,
                    )
                }
            }
        }

        render_pass.consume_queue(sprite_queue);
    }

    pub fn draw_hud_nodes(&mut self, sprite_queue: &mut SpriteColorQueue) {
        let sprite_tree = self.sprite_trees.values_mut().next().unwrap();
        sprite_tree.draw(sprite_queue);
    }

    pub fn draw_ui(&mut self, game_io: &GameIO, sprite_queue: &mut SpriteColorQueue) {
        let local_id: hecs::Entity = self.local_player_id.into();

        if local_id != hecs::Entity::DANGLING {
            // draw player ui
            self.local_health_ui.draw(game_io, sprite_queue);

            let entities = &mut self.entities;

            if let Ok(player) = entities.query_one_mut::<&mut Player>(local_id) {
                // draw emotion window relative to health ui
                let local_health_bounds = self.local_health_ui.bounds();
                let mut emotion_window_position = local_health_bounds.position();
                emotion_window_position.y += local_health_bounds.height + 2.0;

                player.emotion_window.set_position(emotion_window_position);
                player.emotion_window.draw(sprite_queue);
            }
        }

        // draw banners
        for (_, banner) in self.banner_popups.iter_mut() {
            banner.draw(game_io, sprite_queue);
        }
    }

    #[cfg(debug_assertions)]
    fn assertions(&mut self) {
        // make sure card actions are being deleted
        let held_action_count = self
            .entities
            .query_mut::<&ActionQueue>()
            .into_iter()
            .filter(|(_, queue)| queue.active.is_some())
            .count();

        let executed_action_count = self
            .actions
            .values()
            .filter(|action| action.executed && !action.is_async())
            .count();

        let time_is_frozen = self.time_freeze_tracker.time_is_frozen();

        // if there's more executed actions than held actions, we forgot to delete one
        // we can have more actions than held actions still since
        // scripters don't need to attach card actions to entities
        // we also can ignore async actions and time freeze
        assert!(held_action_count >= executed_action_count || time_is_frozen);
    }
}
