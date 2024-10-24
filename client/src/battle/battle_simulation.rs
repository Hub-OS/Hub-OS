use super::*;
use crate::bindable::*;
use crate::packages::{CardPackage, PackageNamespace};
use crate::render::ui::{FontName, PlayerHealthUi, Text};
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

pub struct BattleSimulation {
    pub config: BattleConfig,
    pub statistics: BattleStatistics,
    pub rng: Xoshiro256PlusPlus,
    pub inputs: Vec<PlayerInput>,
    pub time: FrameTime,
    pub battle_time: FrameTime,
    pub camera: Camera,
    pub background: Background,
    pub turn_gauge: TurnGauge,
    pub field: Field,
    pub tile_states: Vec<TileState>,
    pub entities: hecs::World,
    pub generation_tracking: Vec<hecs::Entity>,
    pub ownership_tracking: OwnershipTracking,
    pub queued_attacks: Vec<AttackBox>,
    pub defense: Defense,
    pub sprite_trees: SlotMap<Tree<SpriteNode>>,
    pub animators: SlotMap<BattleAnimator>,
    pub actions: DenseSlotMap<Action>,
    pub time_freeze_tracker: TimeFreezeTracker,
    pub components: DenseSlotMap<Component>,
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
    pub fn new(game_io: &GameIO, props: &BattleProps) -> Self {
        let mut camera = Camera::new(game_io);
        camera.snap(BATTLE_CAMERA_OFFSET);

        let globals = game_io.resource::<Globals>().unwrap();
        let assets = &globals.assets;

        let mut fade_sprite = assets.new_sprite(game_io, ResourcePaths::WHITE_PIXEL);
        fade_sprite.set_color(Color::TRANSPARENT);

        let field = Field::new(game_io, 8, 5);

        let mut sprite_trees: SlotMap<Tree<SpriteNode>> = Default::default();
        // root hud node
        sprite_trees.insert(Tree::new(SpriteNode::new(game_io, Default::default())));

        Self {
            config: BattleConfig::new(globals, &field, props.player_setups.len()),
            statistics: BattleStatistics::new(),
            rng: Xoshiro256PlusPlus::seed_from_u64(props.seed),
            time: 0,
            battle_time: 0,
            inputs: vec![PlayerInput::new(); props.player_setups.len()],
            camera,
            background: props.background.clone(),
            turn_gauge: TurnGauge::new(game_io),
            field,
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
        macro_rules! clone_component {
            ($component: ty) => {
                for (id, component) in self.entities.query_mut::<&$component>() {
                    let _ = entities.insert_one(id, component.clone());
                }
            };
        }

        clone_component!(Artifact);
        clone_component!(Character);
        clone_component!(Living);
        clone_component!(Obstacle);
        clone_component!(Player);
        clone_component!(Spell);
        clone_component!(EntityName);
        clone_component!(EntityShadow);
        clone_component!(EntityShadowVisible);
        clone_component!(HpDisplay);
        clone_component!(Movement);
        clone_component!(AttackContext);
        clone_component!(ActionQueue);

        Self {
            config: self.config.clone(),
            statistics: self.statistics.clone(),
            inputs: self.inputs.clone(),
            rng: self.rng.clone(),
            time: self.time,
            battle_time: self.battle_time,
            camera: self.camera.clone(game_io),
            background: self.background.clone(),
            turn_gauge: self.turn_gauge.clone(),
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

    pub fn play_music(&self, game_io: &GameIO, sound_buffer: &SoundBuffer, loops: bool) {
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

        for (_, (entity, living, name)) in
            entities.query_mut::<(&Entity, &Living, Option<&EntityName>)>()
        {
            if entity.id != self.local_player_id {
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
                name,
                health: living.health,
            });
        }

        self.statistics.calculate_score();
    }

    pub fn handle_local_signals(&mut self, local_index: usize, resources: &SharedBattleResources) {
        let input = &self.inputs[local_index];

        // handle flee
        if !self.is_resimulation && input.has_signal(NetplaySignal::AttemptingFlee) {
            // todo: check for success using some method in battle_init
            resources
                .event_sender
                .send(BattleEvent::FleeResult(true))
                .unwrap();
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
            Component::create_delayed_deleter(self, id.into(), ComponentLifetime::Battle, 0);
        }
    }

    fn spawn_pending(&mut self) {
        type Query<'a> = (&'a mut Entity, Option<&'a ActionQueue>);

        for (_, (entity, action_queue)) in self.entities.query_mut::<Query>() {
            if !entity.pending_spawn {
                continue;
            }

            entity.pending_spawn = false;
            entity.spawned = true;
            entity.on_field = true;

            let tile = self.field.tile_at_mut((entity.x, entity.y)).unwrap();
            tile.handle_auto_reservation_addition(&self.actions, entity, action_queue);

            if entity.team == Team::Unset {
                entity.team = tile.team();
            }

            if entity.facing == Direction::None {
                entity.facing = tile.direction();
            }

            self.animators[entity.animator_index].enable();
            self.pending_callbacks.push(entity.spawn_callback.clone());

            for component in self.components.values() {
                if component.entity == entity.id {
                    self.pending_callbacks.push(component.init_callback.clone());
                }
            }

            if self.battle_started {
                self.pending_callbacks
                    .push(entity.battle_start_callback.clone())
            }

            let tile_state = &self.tile_states[tile.state_index()];
            let tile_callback = tile_state.entity_enter_callback.clone();
            self.pending_callbacks.push(tile_callback.bind(entity.id));
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

    fn cleanup_erased_entities(&mut self) {
        let mut pending_removal = Vec::new();
        let mut components_pending_removal = Vec::new();
        let mut actions_pending_removal = Vec::new();

        for (id, entity) in self.entities.query_mut::<&Entity>() {
            if !entity.erased {
                continue;
            }

            self.field.drop_entity(entity.id);

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
            let (entity, movement, shadow, shadow_visible) = self
                .entities
                .query_one_mut::<(
                    &mut Entity,
                    Option<&Movement>,
                    Option<&EntityShadow>,
                    Option<&EntityShadowVisible>,
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
            if let (Some(shadow), Some(_)) = (shadow, shadow_visible) {
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
        if self.intro_complete {
            let mut hp_text = Text::new(game_io, FontName::EntityHp);
            hp_text.style.letter_spacing = 0.0;
            let tile_size = self.field.tile_size();

            for (_, (hp_display, entity)) in self.entities.query_mut::<(&HpDisplay, &Entity)>() {
                if entity.deleted
                    || !entity.on_field
                    || !hp_display.initialized
                    || hp_display.value <= 0
                    || entity.id == self.local_player_id
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
            if entity.on_field {
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

        if local_id == hecs::Entity::DANGLING {
            return;
        }

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

        // make sure empty action queues are cleaned up
        assert!(
            self.entities
                .query_mut::<&ActionQueue>()
                .into_iter()
                .all(|(_, queue)| queue.active.is_some() || !queue.pending.is_empty())
                || time_is_frozen
        );
    }
}
