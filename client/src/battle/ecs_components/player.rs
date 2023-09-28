use crate::battle::*;
use crate::bindable::*;
use crate::memoize::ResultCacheSingle;
use crate::packages::PackageNamespace;
use crate::render::*;
use crate::resources::*;
use crate::saves::{BlockGrid, Card};
use crate::structures::DenseSlotMap;
use framework::prelude::*;

#[derive(Clone)]
pub struct Player {
    pub index: usize,
    pub local: bool,
    pub deck: Vec<Card>,
    pub has_regular_card: bool,
    pub can_flip: bool,
    pub attack_boost: u8,
    pub rapid_boost: u8,
    pub charge_boost: u8,
    pub hand_size_boost: i8,
    pub card_chargable_cache: ResultCacheSingle<Option<CardProperties>, bool>,
    pub card_charged: bool,
    pub card_charge: AttackCharge,
    pub buster_charge: AttackCharge,
    pub flinch_animation_state: String,
    pub movement_animation_state: String,
    pub slide_when_moving: bool,
    pub emotion_window: EmotionUi,
    pub forms: Vec<PlayerForm>,
    pub active_form: Option<usize>,
    pub augments: DenseSlotMap<Augment>,
    pub calculate_charge_time_callback: BattleCallback<u8, FrameTime>,
    pub normal_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub charged_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub special_attack_callback: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub can_charge_card_callback: Option<BattleCallback<CardProperties, bool>>,
    pub charged_card_callback: Option<BattleCallback<CardProperties, Option<GenerationalIndex>>>,
    pub movement_callback: BattleCallback<Direction>,
}

impl Player {
    pub const IDLE_STATE: &str = "PLAYER_IDLE";

    pub const MOVE_FRAMES: [DerivedFrame; 7] = [
        DerivedFrame::new(0, 1),
        DerivedFrame::new(1, 1),
        DerivedFrame::new(2, 1),
        DerivedFrame::new(3, 1),
        DerivedFrame::new(2, 1),
        DerivedFrame::new(1, 1),
        DerivedFrame::new(0, 1),
    ];

    pub const HIT_FRAMES: [DerivedFrame; 2] = [DerivedFrame::new(0, 15), DerivedFrame::new(1, 7)];

    fn new(
        game_io: &GameIO,
        setup: PlayerSetup,
        entity_id: EntityId,
        card_charge_sprite_index: TreeIndex,
        buster_charge_sprite_index: TreeIndex,
    ) -> Self {
        let assets = &game_io.resource::<Globals>().unwrap().assets;
        let player_package = setup.player_package(game_io);

        let mut deck = setup.deck;

        if let Some(index) = deck.regular_index {
            // move the regular card to the front
            deck.cards.swap(0, index);
        }

        Self {
            index: setup.index,
            local: setup.local,
            deck: deck.cards,
            has_regular_card: deck.regular_index.is_some(),
            can_flip: true,
            attack_boost: 0,
            rapid_boost: 0,
            charge_boost: 0,
            hand_size_boost: 0,
            card_chargable_cache: ResultCacheSingle::default(),
            card_charged: false,
            card_charge: AttackCharge::new(
                assets,
                card_charge_sprite_index,
                ResourcePaths::BATTLE_CARD_CHARGE_ANIMATION,
            ),
            buster_charge: AttackCharge::new(
                assets,
                buster_charge_sprite_index,
                ResourcePaths::BATTLE_CHARGE_ANIMATION,
            )
            .with_color(Color::MAGENTA),
            flinch_animation_state: String::new(),
            movement_animation_state: String::new(),
            slide_when_moving: false,
            emotion_window: EmotionUi::new(
                setup.emotion,
                assets.new_sprite(game_io, &player_package.emotions_texture_path),
                Animator::load_new(assets, &player_package.emotions_animation_path),
            ),
            forms: Vec::new(),
            active_form: None,
            augments: Default::default(),
            calculate_charge_time_callback: BattleCallback::new(|_, _, _, level| {
                Self::calculate_default_charge_time(level)
            }),
            normal_attack_callback: None,
            charged_attack_callback: None,
            special_attack_callback: None,
            charged_card_callback: None,
            can_charge_card_callback: None,
            movement_callback: BattleCallback::new(
                move |game_io, resources, simulation, direction: Direction| {
                    let entities = &mut simulation.entities;
                    let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                        return;
                    };

                    let mut dest = (entity.x, entity.y);
                    let offset = direction.i32_vector();

                    if offset.1 != 0 {
                        dest.1 += offset.1;
                    } else {
                        dest.0 += offset.0;
                    }

                    let can_move_to_callback = entity.can_move_to_callback.clone();

                    if can_move_to_callback.call(game_io, resources, simulation, dest) {
                        Self::queue_default_movement(simulation, entity_id, dest);
                    }
                },
            ),
        }
    }

    pub fn load(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        mut setup: PlayerSetup,
    ) -> rollback_mlua::Result<EntityId> {
        let local = setup.local;
        let player_package = setup.player_package(game_io);
        let blocks = std::mem::take(&mut setup.blocks);

        // namespace for using cards / attacks
        let namespace = setup.namespace();
        let id = Character::create(game_io, simulation, CharacterRank::V1, namespace)?;

        let (entity, living) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Living)>(id.into())
            .unwrap();

        // spawn immediately
        entity.pending_spawn = true;

        // prevent attacks from countering by default
        entity.hit_context.flags = HitFlag::NO_COUNTER;

        // use preloaded package properties
        entity.element = player_package.element;
        entity.name = player_package.name.clone();
        living.status_director.set_input_index(setup.index);

        // delete callback
        entity.delete_callback = BattleCallback::new(move |game_io, _, simulation, _| {
            delete_player_animation(game_io, simulation, id);
        });

        // flinch callback
        living.register_status_callback(
            HitFlag::FLINCH,
            BattleCallback::new(move |game_io, resources, simulation, _| {
                // cancel actions
                Action::cancel_all(game_io, resources, simulation, id);

                let (entity, living, player) = simulation
                    .entities
                    .query_one_mut::<(&mut Entity, &Living, &mut Player)>(id.into())
                    .unwrap();

                if !living.status_director.is_dragged() {
                    // cancel movement
                    entity.movement = None;
                }

                player.cancel_charge();

                // play flinch animation
                let animator = &mut simulation.animators[entity.animator_index];

                let callbacks = animator.set_state(&player.flinch_animation_state);
                simulation.pending_callbacks.extend(callbacks);

                // on complete will return to idle
                animator.on_complete(BattleCallback::new(
                    move |game_io, resources, simulation, _| {
                        let entity = simulation
                            .entities
                            .query_one_mut::<&Entity>(id.into())
                            .unwrap();

                        let animator = &mut simulation.animators[entity.animator_index];

                        let callbacks = animator.set_state(Player::IDLE_STATE);
                        animator.set_loop_mode(AnimatorLoopMode::Loop);
                        simulation.pending_callbacks.extend(callbacks);

                        simulation.call_pending_callbacks(game_io, resources);
                    },
                ));

                simulation.play_sound(game_io, &game_io.resource::<Globals>().unwrap().sfx.hurt);
                simulation.call_pending_callbacks(game_io, resources);
            }),
        );

        // AuxProp for hit statistics
        let statistics_aux_prop = AuxProp::new()
            .with_requirement(AuxRequirement::HitDamage(Comparison::GT, 0))
            .with_callback(BattleCallback::new(move |_, _, simulation, _| {
                if local {
                    simulation.statistics.hits_taken += 1;
                }
            }));
        living.add_aux_prop(statistics_aux_prop);

        // AuxProp for decross
        let decross_aux_prop = AuxProp::new()
            .with_requirement(AuxRequirement::HitDamage(Comparison::GT, 0))
            .with_requirement(AuxRequirement::HitElementIsWeakness)
            .with_callback(BattleCallback::new(move |_, _, simulation, _| {
                let entities = &mut simulation.entities;
                let player = entities.query_one_mut::<&Player>(id.into()).unwrap();

                // deactivate form
                let Some(form_index) = player.active_form else {
                    return;
                };

                simulation.time_freeze_tracker.queue_animation(
                    30,
                    BattleCallback::new(move |game_io, _, simulation, _| {
                        let entities = &mut simulation.entities;

                        let Ok((entity, living, player)) =
                            entities.query_one_mut::<(&Entity, &Living, &mut Player)>(id.into())
                        else {
                            return;
                        };

                        if living.health <= 0 || entity.deleted {
                            // skip decross if we're already deleted
                            return;
                        }

                        if player.active_form != Some(form_index) {
                            // this is a different form
                            // must have switched in cust with some frame perfect shenanigans
                            return;
                        }

                        player.active_form = None;

                        let form = &mut player.forms[form_index];
                        form.deactivated = true;

                        if let Some(callback) = form.deactivate_callback.clone() {
                            simulation.pending_callbacks.push(callback);
                        }

                        // spawn shine fx
                        let mut shine_position = entity.full_position();
                        shine_position.offset += Vec2::new(0.0, -entity.height * 0.5);

                        // play revert sfx
                        let sfx = &game_io.resource::<Globals>().unwrap().sfx;
                        simulation.play_sound(game_io, &sfx.transform_revert);

                        // actual shine creation as indicated above
                        let shine_id = Artifact::create_transformation_shine(game_io, simulation);
                        let shine_entity = simulation
                            .entities
                            .query_one_mut::<&mut Entity>(shine_id.into())
                            .unwrap();

                        // shine position, set to spawn
                        shine_entity.copy_full_position(shine_position);
                        shine_entity.pending_spawn = true;
                    }),
                );
            }));
        living.add_aux_prop(decross_aux_prop);

        // resolve health boost
        // todo: move to Augment?
        let grid = BlockGrid::new(namespace).with_blocks(game_io, blocks);

        let health_boost = grid.augments(game_io).fold(0, |acc, (package, level)| {
            acc + package.health_boost * level as i32
        });

        living.max_health = setup.base_health + health_boost;
        living.set_health(setup.health);

        // insert entity
        let script_enabled = setup.script_enabled;

        let card_charge_sprite_index = AttackCharge::create_sprite(
            game_io,
            &mut entity.sprite_tree,
            ResourcePaths::BATTLE_CARD_CHARGE,
        );

        let buster_charge_sprite_index = AttackCharge::create_sprite(
            game_io,
            &mut entity.sprite_tree,
            ResourcePaths::BATTLE_CHARGE,
        );

        let mut player = Player::new(
            game_io,
            setup,
            id,
            card_charge_sprite_index,
            buster_charge_sprite_index,
        );

        player.movement_animation_state = BattleAnimator::derive_state(
            &mut simulation.animators,
            "PLAYER_MOVE",
            Player::MOVE_FRAMES.to_vec(),
            entity.animator_index,
        );

        player.flinch_animation_state = BattleAnimator::derive_state(
            &mut simulation.animators,
            "PLAYER_HIT",
            Player::HIT_FRAMES.to_vec(),
            entity.animator_index,
        );

        simulation.entities.insert_one(id.into(), player).unwrap();

        // init player
        if script_enabled {
            // call init function
            let package_info = &player_package.package_info;

            let vm_manager = &resources.vm_manager;
            let vm_index = vm_manager.find_vm(&package_info.id, namespace)?;

            simulation.call_global(game_io, resources, vm_index, "player_init", move |lua| {
                crate::lua_api::create_entity_table(lua, id)
            })?;
        } else {
            // default init, sprites + animation only
            let (texture_path, animator) = player_package.resolve_battle_sprite(game_io);

            // regrab entity
            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(id.into())
                .unwrap();

            // adopt texture
            let sprite_node = entity.sprite_tree.root_mut();
            sprite_node.set_texture(game_io, texture_path);

            // adopt animator
            let battle_animator = &mut simulation.animators[entity.animator_index];
            let callbacks = battle_animator.copy_from_animator(&animator);
            battle_animator.find_and_apply_to_target(&mut simulation.entities);

            // callbacks
            simulation.pending_callbacks.extend(callbacks);
            simulation.call_pending_callbacks(game_io, resources);
        }

        // init blocks
        for (package, level) in grid.augments(game_io) {
            let package_info = &package.package_info;
            let vm_manager = &resources.vm_manager;
            let vm_index = vm_manager.find_vm(&package_info.id, namespace)?;

            let player = simulation
                .entities
                .query_one_mut::<&mut Player>(id.into())
                .unwrap();
            let index = player.augments.insert(Augment::from((package, level)));

            let vms = resources.vm_manager.vms();
            let lua = &vms[vm_index].lua;
            let lua_globals = lua.globals();
            let has_init = lua_globals.contains_key("augment_init").unwrap_or_default();

            if !has_init {
                continue;
            }

            let result =
                simulation.call_global(game_io, resources, vm_index, "augment_init", move |lua| {
                    crate::lua_api::create_augment_table(lua, id, index)
                });

            if let Err(e) = result {
                log::error!("{e}");
            }
        }

        Ok(id)
    }

    pub fn initialize_uninitialized(simulation: &mut BattleSimulation) {
        // resolve flippable defaults by team (Team::Other will always be true)
        let mut default_red_flippable = false;
        let mut default_blue_flippable = false;

        let field_col_count = simulation.field.cols();

        for ((col, _), tile) in simulation.field.iter_mut() {
            if col == 1 && tile.original_team() != Team::Red {
                default_red_flippable = true;
            }

            if col + 2 == field_col_count && tile.original_team() != Team::Blue {
                default_blue_flippable = true;
            }
        }

        // initialize uninitalized
        let config = &simulation.config;

        type PlayerQuery<'a> = (&'a mut Entity, &'a mut Player, &'a Living);

        for (_, (entity, player, living)) in simulation.entities.query_mut::<PlayerQuery>() {
            // track the local player's health
            if player.local {
                simulation.local_player_id = entity.id;
                simulation.local_health_ui.set_max_health(living.max_health);
                simulation.local_health_ui.snap_health(living.health);
            }

            // initialize position
            let pos = config
                .player_spawn_positions
                .get(player.index)
                .cloned()
                .unwrap_or_default();

            entity.x = pos.0;
            entity.y = pos.1;

            // initalize team
            let tile = simulation.field.tile_at_mut((entity.x, entity.y));
            entity.team = tile.map(|tile| tile.team()).unwrap_or_default();

            // initalize flippable
            let flippable_config = &config.player_flippable;
            let can_flip_setting = flippable_config.get(player.index).cloned().flatten();

            player.can_flip = can_flip_setting.unwrap_or(match entity.team {
                Team::Red => default_red_flippable,
                Team::Blue => default_blue_flippable,
                _ => true,
            });

            // initialize animation state
            let animator = &mut simulation.animators[entity.animator_index];

            if animator.current_state().is_none() {
                let callbacks = animator.set_state(Player::IDLE_STATE);
                animator.set_loop_mode(AnimatorLoopMode::Loop);
                simulation.pending_callbacks.extend(callbacks);
            }
        }
    }

    pub fn available_forms(&self) -> impl Iterator<Item = (usize, &PlayerForm)> {
        self.forms
            .iter()
            .enumerate()
            .filter(|(_, form)| !form.activated)
    }

    pub fn namespace(&self) -> PackageNamespace {
        PackageNamespace::Netplay(self.index as u8)
    }

    pub fn active_form_update_callback(&self) -> Option<&BattleCallback> {
        self.forms[self.active_form?].update_callback.as_ref()
    }

    pub fn hand_size(&self) -> usize {
        const BASE_HAND_SIZE: i8 = 5;

        let boosted_hand_size = BASE_HAND_SIZE.saturating_add(self.hand_size_boost);

        let augment_iter = self.augments.values();
        let augmented_hand_size = augment_iter.fold(boosted_hand_size, |acc, m| {
            acc.saturating_add(m.hand_size_boost * m.level as i8)
        });

        augmented_hand_size.clamp(1, 10) as usize
    }

    pub fn attack_level(&self) -> u8 {
        let augment_iter = self.augments.values();
        let base_attack = augment_iter
            .fold(1, |acc, m| acc + m.attack_boost as i32 * m.level as i32)
            .clamp(1, 5) as u8;

        base_attack + self.attack_boost
    }

    pub fn rapid_level(&self) -> u8 {
        let augment_iter = self.augments.values();
        let base_speed = augment_iter
            .fold(1, |acc, m| acc + m.rapid_boost as i32 * m.level as i32)
            .clamp(1, 5) as u8;

        base_speed + self.rapid_boost
    }

    pub fn charge_level(&self) -> u8 {
        let augment_iter = self.augments.values();
        let base_charge = augment_iter
            .fold(1, |acc, m| acc + m.charge_boost as i32 * m.level as i32)
            .clamp(1, 5) as u8;

        base_charge + self.charge_boost
    }

    pub fn calculate_default_charge_time(level: u8) -> FrameTime {
        match level {
            0 | 1 => 100,
            2 => 90,
            3 => 80,
            4 => 70,
            _ => 60,
        }
    }

    pub fn calculate_charge_time(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        level: Option<u8>,
    ) -> FrameTime {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return 0;
        };

        let level = level.unwrap_or_else(|| player.charge_level());

        let augment_iter = player.augments.values();
        let augment_callback = augment_iter
            .flat_map(|augment| augment.calculate_charge_time_callback.clone())
            .next();

        let callback = augment_callback
            .or_else(|| {
                player.active_form.and_then(|index| {
                    let form = player.forms.get(index)?;
                    form.calculate_charge_time_callback.clone()
                })
            })
            .unwrap_or_else(|| player.calculate_charge_time_callback.clone());

        callback.call(game_io, resources, simulation, level)
    }

    pub fn cancel_charge(&mut self) {
        self.buster_charge.cancel();
        self.card_charge.cancel();
    }

    pub fn handle_charging(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let max_charge_time =
            Player::calculate_charge_time(game_io, resources, simulation, entity_id, None);

        // test the next card to see if can be charged
        // cached to reduce lua calls + copying card props
        let entities = &mut simulation.entities;
        let (player, character) = entities
            .query_one_mut::<(&mut Player, &Character)>(entity_id.into())
            .unwrap();

        let mut card_chargable_cache = std::mem::take(&mut player.card_chargable_cache);
        let card_props = character.cards.last().cloned();
        let can_charge_card = card_chargable_cache.calculate(card_props, |card_props| {
            if let Some(card_props) = card_props {
                Self::resolve_card_charger(game_io, resources, simulation, entity_id, card_props)
                    .is_some()
            } else {
                false
            }
        });

        // update AttackCharge structs
        let entities = &mut simulation.entities;
        let (entity, player, character) = entities
            .query_one_mut::<(&Entity, &mut Player, &mut Character)>(entity_id.into())
            .unwrap();

        player.card_chargable_cache = card_chargable_cache;

        let play_sfx = !simulation.is_resimulation;
        let animator = &simulation.animators[entity.animator_index];
        let is_idle = animator.current_state() == Some(Player::IDLE_STATE);
        let input = &simulation.inputs[player.index];

        if can_charge_card && input.was_just_pressed(Input::UseCard) {
            // cancel buster charging
            player.buster_charge.cancel();
            player.card_charge.set_charging(true);
        } else if input.was_just_pressed(Input::Shoot) {
            // cancel card charging
            player.card_charge.cancel();
            player.buster_charge.set_charging(true);
        } else {
            // continue charging

            // block charging card if we weren't already
            // prevents accidental charge tracking from a previous non charged card use
            let charging_card = input.is_down(Input::UseCard)
                && can_charge_card
                && !player.buster_charge.charging()
                && player.card_charge.charging();
            player.card_charge.set_charging(charging_card);

            let charging_buster = input.is_down(Input::Shoot) && !player.card_charge.charging();
            player.buster_charge.set_charging(charging_buster);
        }

        player.card_charge.set_max_charge_time(max_charge_time);
        player.buster_charge.set_max_charge_time(max_charge_time);

        let card_used = if !can_charge_card && input.was_just_pressed(Input::UseCard) {
            Some(false)
        } else {
            player.card_charge.update(game_io, is_idle, play_sfx)
        };

        let buster_fired = player.buster_charge.update(game_io, is_idle, play_sfx);

        // update from results

        if let Some(fully_charged) = card_used {
            player.card_charged = fully_charged;
            character.card_use_requested = true;
        }

        if !character.card_use_requested {
            if let Some(fully_charged) = buster_fired {
                if fully_charged {
                    Player::use_charged_attack(game_io, resources, simulation, entity_id);
                } else {
                    Player::use_normal_attack(game_io, resources, simulation, entity_id)
                }
            }
        }
    }

    pub fn use_normal_attack(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        // resolve action for normal attack
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        // augment
        let augment_iter = player.augments.values();
        let mut callbacks: Vec<_> = augment_iter
            .flat_map(|augment| augment.normal_attack_callback.clone())
            .collect();

        // base
        if let Some(callback) = player.normal_attack_callback.clone() {
            callbacks.push(callback);
        }

        Action::queue_first_from_factories(game_io, resources, simulation, entity_id, callbacks);
    }

    pub fn use_charged_attack(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        // resolve action for charged attack
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        // augment
        let augment_iter = player.augments.values();
        let mut callbacks: Vec<_> = augment_iter
            .flat_map(|augment| augment.charged_attack_callback.clone())
            .collect();

        // form
        let form_callback = player
            .active_form
            .and_then(|index| player.forms.get(index))
            .and_then(|form| form.special_attack_callback.clone());

        if let Some(callback) = form_callback {
            callbacks.push(callback);
        }

        // base
        if let Some(callback) = player.charged_attack_callback.clone() {
            callbacks.push(callback);
        }

        Action::queue_first_from_factories(game_io, resources, simulation, entity_id, callbacks);
    }

    pub fn use_special_attack(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        // resolve action for special attack
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return;
        };

        // augment
        let augment_iter = player.augments.values();
        let mut callbacks: Vec<_> = augment_iter
            .flat_map(|augment| augment.special_attack_callback.clone())
            .collect();

        // form
        let form_callback = player
            .active_form
            .and_then(|index| player.forms.get(index))
            .and_then(|form| form.special_attack_callback.clone());

        if let Some(callback) = form_callback {
            callbacks.push(callback);
        }

        // base
        if let Some(callback) = player.special_attack_callback.clone() {
            callbacks.push(callback);
        }

        Action::queue_first_from_factories(game_io, resources, simulation, entity_id, callbacks);
    }

    fn resolve_card_charger(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        card_props: CardProperties,
    ) -> Option<BattleCallback<CardProperties, Option<GenerationalIndex>>> {
        let entities = &mut simulation.entities;
        let Ok(player) = entities.query_one_mut::<&Player>(entity_id.into()) else {
            return None;
        };

        // augment
        let augment_iter = player.augments.values();
        let mut callbacks: Vec<_> = augment_iter
            .flat_map(|augment| {
                Some((
                    augment.can_charge_card_callback.clone()?,
                    augment.charged_card_callback.clone()?,
                ))
            })
            .collect();

        // form
        let form_callback = player.active_form.and_then(|index| {
            let form = player.forms.get(index)?;

            Some((
                form.can_charge_card_callback.clone()?,
                form.charged_card_callback.clone()?,
            ))
        });

        if let Some(callback) = form_callback {
            callbacks.push(callback);
        }

        // base
        if let (Some(support_test), Some(callback)) = (
            player.can_charge_card_callback.clone(),
            player.charged_card_callback.clone(),
        ) {
            callbacks.push((support_test, callback));
        }

        callbacks
            .into_iter()
            .find(|(support_test, _)| {
                support_test.call(game_io, resources, simulation, card_props.clone())
            })
            .map(|(_, callback)| callback)
    }

    // setup context before + after calling this
    fn resolve_charged_card_action(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        card_props: CardProperties,
    ) -> Option<GenerationalIndex> {
        let callback = Self::resolve_card_charger(
            game_io,
            resources,
            simulation,
            entity_id,
            card_props.clone(),
        )?;

        callback
            .call(game_io, resources, simulation, card_props)
            .filter(|index| simulation.actions.get(*index).is_some())
    }

    pub fn use_card(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let (entity, character, player) = simulation
            .entities
            .query_one_mut::<(&mut Entity, &mut Character, &mut Player)>(entity_id.into())
            .unwrap();

        // allow attacks to counter
        let original_context_flags = entity.hit_context.flags;
        entity.hit_context.flags = HitFlag::NONE;

        // create card action
        let card_props = character.cards.pop().unwrap();
        let action_index = if player.card_charged {
            player.card_charged = false;

            Self::resolve_charged_card_action(game_io, resources, simulation, entity_id, card_props)
        } else {
            let namespace = character.namespace;

            Action::create_from_card_properties(
                game_io,
                resources,
                simulation,
                entity_id,
                namespace,
                &card_props,
            )
        };

        // revert context flags
        let entities = &mut simulation.entities;

        let entity = entities
            .query_one_mut::<&mut Entity>(entity_id.into())
            .unwrap();
        entity.hit_context.flags = original_context_flags;

        // spawn a poof if there's no action
        let Some(index) = action_index else {
            Artifact::create_card_poof(game_io, simulation, entity_id);
            return;
        };

        Action::queue_action(simulation, entity_id, index);
    }

    fn resolve_movement_callback(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) -> BattleCallback<Direction> {
        let entities = &mut simulation.entities;
        let player = entities.query_one_mut::<&Player>(entity_id.into()).unwrap();

        // augment
        let augment_iter = player.augments.values();
        let augment_callback = augment_iter
            .flat_map(|augment| augment.movement_callback.clone())
            .next();

        if let Some(callback) = augment_callback {
            return callback;
        }

        // form
        let form_callback = player.active_form.and_then(|index| {
            let form = player.forms.get(index)?;

            form.movement_callback.clone()
        });

        if let Some(callback) = form_callback {
            return callback;
        }

        // base
        player.movement_callback.clone()
    }

    pub fn handle_movement_input(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        type Query<'a> = (&'a mut Entity, &'a Living, &'a mut Player);

        let entities = &mut simulation.entities;
        let Ok((entity, living, player)) = entities.query_one_mut::<Query>(entity_id.into()) else {
            return;
        };

        // can't move if there's a blocking action or immoble
        let status_registry = &resources.status_registry;
        if entity.action_index.is_some()
            || !entity.action_queue.is_empty()
            || living.status_director.is_immobile(status_registry)
        {
            return;
        }

        let animator = &simulation.animators[entity.animator_index];

        // can only move if there's no move action queued and the current animation is PLAYER_IDLE
        if entity.movement.is_some() || animator.current_state() != Some(Player::IDLE_STATE) {
            return;
        }

        let input = &simulation.inputs[player.index];

        // handle flipping
        let face_left = input.was_just_pressed(Input::FaceLeft);
        let face_right = input.was_just_pressed(Input::FaceRight);

        let flip_requested = (face_left && face_right)
            || (face_left && entity.facing != Direction::Left)
            || (face_right && entity.facing != Direction::Right);

        if flip_requested && player.can_flip {
            entity.facing = entity.facing.reversed();
        }

        // handle movement
        let confused = (living.status_director).remaining_status_time(HitFlag::CONFUSE) > 0;

        let mut x_offset = input.is_down(Input::Right) as i32 - input.is_down(Input::Left) as i32;

        if entity.team == Team::Blue {
            // flipped perspective
            x_offset = -x_offset;
        }

        if confused {
            x_offset = -x_offset;
        }

        let mut y_offset = input.is_down(Input::Down) as i32 - input.is_down(Input::Up) as i32;

        if confused {
            y_offset = -y_offset;
        }

        let direction = Direction::from_i32_vector((x_offset, y_offset));

        if direction == Direction::None {
            return;
        }

        let movement_callback = Self::resolve_movement_callback(simulation, entity_id);
        movement_callback.call(game_io, resources, simulation, direction);

        // movement statistics
        if simulation.local_player_id == entity_id {
            let entities = &mut simulation.entities;
            let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                return;
            };

            if entity.movement.is_some() {
                simulation.statistics.movements += 1;
            }
        }
    }

    pub fn queue_default_movement(
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        dest: (i32, i32),
    ) {
        type Query<'a> = (&'a mut Entity, &'a mut Player);

        let entities = &mut simulation.entities;
        let Ok((entity, player)) = entities.query_one_mut::<Query>(entity_id.into()) else {
            return;
        };

        if player.slide_when_moving {
            entity.movement = Some(Movement::slide(dest, 14));
        } else {
            let mut move_event = Movement::teleport(dest);
            move_event.delay = 5;
            move_event.endlag = 7;

            let animator_index = entity.animator_index;
            let movement_state = player.movement_animation_state.clone();

            move_event.on_begin = Some(BattleCallback::new(move |_, _, simulation, _| {
                let anim = &mut simulation.animators[animator_index];

                let callbacks = anim.set_state(&movement_state);
                simulation.pending_callbacks.extend(callbacks);

                // reset to PLAYER_IDLE when movement finishes
                anim.on_complete(BattleCallback::new(move |_, _, simulation, _| {
                    let anim = &mut simulation.animators[animator_index];
                    let callbacks = anim.set_state(Player::IDLE_STATE);
                    anim.set_loop_mode(AnimatorLoopMode::Loop);

                    simulation.pending_callbacks.extend(callbacks);
                }));
            }));

            entity.movement = Some(move_event);
        }
    }
}
