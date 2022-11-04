use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::ease::inverse_lerp;
use crate::lua_api::create_entity_table;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use std::cell::RefCell;

#[derive(Clone)]
pub struct BattleState {
    complete: bool,
    message: Option<(&'static str, FrameTime)>,
}

impl State for BattleState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, game_io: &GameIO<Globals>) -> Option<Box<dyn State>> {
        if self.complete {
            Some(Box::new(CardSelectState::new(game_io)))
        } else {
            None
        }
    }

    fn update(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        self.detect_battle_start(game_io, simulation, vms);

        // update tiles
        self.update_field(game_io, simulation, vms);

        // new: process action queues
        self.process_action_queues(game_io, simulation, vms);

        // update spells
        self.update_spells(game_io, simulation, vms);

        // todo: handle spells attacking on tiles they're not on?
        // this can unintentionally occur by queueing an attack outside of update_spells,
        // such as in a BattleStep component or callback

        // execute attacks
        self.execute_attacks(game_io, simulation, vms);

        // process 0 HP
        self.mark_deleted(game_io, simulation, vms);

        // update artifacts
        self.update_artifacts(game_io, simulation, vms);

        // new: process player input
        self.process_input(game_io, simulation, vms);

        // new: update living, processes statuses
        self.update_living(game_io, simulation, vms);

        // update battle step components
        for (_, component) in &simulation.components {
            if component.lifetime == ComponentLifetime::BattleStep {
                simulation
                    .pending_callbacks
                    .push(component.update_callback.clone());
            }
        }

        simulation.call_pending_callbacks(game_io, vms);

        if self.message.is_none() {
            // skip updating time if the battle has ended
            simulation.battle_time += 1;
            simulation.statistics.time += 1;
        }

        self.detect_success_or_failure(simulation);
        self.update_turn_guage(game_io, simulation);
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO<Globals>,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        if let Some((text, start_time)) = self.message {
            const MESSAGE_INTRO_TIME: FrameTime = 10;

            let mut style = TextStyle::new(game_io, FontStyle::Battle);
            style.letter_spacing = 0.0;
            style.scale.y = inverse_lerp!(0, MESSAGE_INTRO_TIME, simulation.time - start_time);

            let size = style.measure(text).size;
            let position = (RESOLUTION_F - size * style.scale) * 0.5;
            style.bounds.set_position(position);

            style.draw(game_io, sprite_queue, text);
        }

        simulation.turn_guage.draw(sprite_queue);
    }
}

impl BattleState {
    pub fn new() -> Self {
        Self {
            complete: false,
            message: None,
        }
    }

    fn update_turn_guage(&mut self, game_io: &GameIO<Globals>, simulation: &mut BattleSimulation) {
        let previously_incomplete = !simulation.turn_guage.is_complete();

        simulation.turn_guage.increment_time();

        if !simulation.turn_guage.is_complete() || self.message.is_some() {
            // don't check for input if the battle has ended, or if the turn guage isn't complete
            return;
        }

        if previously_incomplete {
            // just completed, play a sfx
            let globals = game_io.globals();
            globals.audio.play_sound(&globals.turn_guage_sfx);
        }

        self.complete = simulation
            .entities
            .query_mut::<(&Entity, &mut Player)>()
            .into_iter()
            .any(|(_, (entity, player))| {
                if entity.deleted {
                    return false;
                }

                let input = &simulation.inputs[player.index];

                // todo: create a new input for ending the turn?
                input.was_just_pressed(Input::ShoulderL) || input.was_just_pressed(Input::ShoulderR)
            });
    }

    fn detect_success_or_failure(&mut self, simulation: &mut BattleSimulation) {
        const TOTAL_MESSAGE_TIME: FrameTime = 5 * 60;

        if let Some((_, time)) = self.message {
            if simulation.time - time >= TOTAL_MESSAGE_TIME {
                simulation.exit = true;
            }
            return;
        }

        // detect failure + find team for success detection
        const FAILED_MESSAGE: &str = "<_FAILED_>";
        let local_team;

        if let Ok(entity) =
            (simulation.entities).query_one_mut::<&Entity>(simulation.local_player_id.into())
        {
            if entity.deleted {
                self.message = Some((FAILED_MESSAGE, simulation.time));
                return;
            }

            local_team = entity.team;
        } else {
            self.message = Some((FAILED_MESSAGE, simulation.time));
            return;
        }

        // detect success
        // todo: score screen
        const SUCCESS_MESSAGE: &str = "<_SUCCESS_>";

        let enemies_alive = simulation
            .entities
            .query_mut::<(&Entity, &Character)>()
            .into_iter()
            .any(|(_, (entity, _))| entity.team != local_team);

        if !enemies_alive {
            self.message = Some((SUCCESS_MESSAGE, simulation.time));
        }
    }

    fn detect_battle_start(
        &self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        if simulation.battle_started {
            return;
        }

        simulation.battle_started = true;

        for (_, entity) in simulation.entities.query_mut::<&mut Entity>() {
            simulation
                .pending_callbacks
                .push(entity.battle_start_callback.clone())
        }

        simulation.call_pending_callbacks(game_io, vms);
    }

    pub fn update_field(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        let cols = simulation.field.cols() as i32;
        let rows = simulation.field.rows() as i32;

        for row in 0..rows {
            for col in 0..cols {
                let tile = simulation.field.tile_at_mut((col, row)).unwrap();

                tile.reset_highlight();
            }
        }

        for (id, (entity, living)) in simulation.entities.query_mut::<(&Entity, &mut Living)>() {
            if entity.ignore_tile_effects || !entity.on_field || entity.deleted {
                continue;
            }

            let tile_pos = (entity.x, entity.y);
            let tile = simulation.field.tile_at_mut(tile_pos).unwrap();

            match tile.state() {
                TileState::Poison => {
                    if simulation.battle_time > 0 && simulation.battle_time % POISON_INTERVAL == 0 {
                        let callback = BattleCallback::new(move |game_io, simulation, vms, _| {
                            let mut hit_props = HitProperties::blank();
                            hit_props.damage = 1;

                            Living::process_hit(game_io, simulation, vms, id.into(), hit_props);
                        });

                        simulation.pending_callbacks.push(callback);
                    }
                }
                TileState::Grass => {
                    if entity.element == Element::Wood && living.health < living.max_health {
                        // todo: should entities have individual timers?
                        let heal_interval = if living.health >= 9 {
                            GRASS_HEAL_INTERVAL
                        } else {
                            GRASS_SLOWED_HEAL_INTERVAL
                        };

                        if simulation.battle_time > 0 && simulation.battle_time % heal_interval == 0
                        {
                            living.health += 1;
                        }
                    }
                }
                TileState::Lava => {
                    if entity.element != Element::Fire {
                        let callback = BattleCallback::new(move |game_io, simulation, vms, _| {
                            // todo: spawn explosion artifact

                            let hit_props = HitProperties {
                                damage: 50,
                                element: Element::Fire,
                                ..Default::default()
                            };

                            Living::process_hit(game_io, simulation, vms, id.into(), hit_props);
                        });

                        simulation.pending_callbacks.push(callback);
                        tile.set_state(TileState::Normal);
                    }
                }
                TileState::DirectionLeft
                | TileState::DirectionRight
                | TileState::DirectionUp
                | TileState::DirectionDown => {
                    if entity.move_action.is_none()
                        && simulation.battle_time - entity.last_successful_move
                            >= CONVEYOR_MOVEMENT_DELAY
                    {
                        let direction = tile.state().direction();
                        let offset = direction.i32_vector();
                        let dest = (entity.x + offset.0, entity.y + offset.1);

                        let can_move_to_callback =
                            entity.current_can_move_to_callback(&simulation.card_actions);

                        let callback = BattleCallback::new(move |game_io, simulation, vms, _| {
                            let can_move_to =
                                can_move_to_callback.call(game_io, simulation, vms, dest);

                            if can_move_to {
                                let entity = simulation
                                    .entities
                                    .query_one_mut::<&mut Entity>(id)
                                    .unwrap();

                                let move_action = MoveAction::slide(dest, 4);
                                entity.move_action = Some(move_action);
                            }
                        });

                        simulation.pending_callbacks.push(callback);
                    }
                }
                _ => {}
            }
        }

        simulation.call_pending_callbacks(game_io, vms)
    }

    fn update_spells(
        &self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        let mut callbacks = Vec::new();

        for (_, (entity, spell)) in simulation.entities.query_mut::<(&mut Entity, &Spell)>() {
            if let Some(tile) = simulation.field.tile_at_mut((entity.x, entity.y)) {
                tile.set_highlight(spell.requested_highlight);
            }

            if entity.time_is_frozen || entity.updated || !entity.spawned || entity.deleted {
                continue;
            }

            callbacks.push(entity.update_callback.clone());

            for index in entity.local_components.iter().cloned() {
                let component = simulation.components.get(index).unwrap();

                callbacks.push(component.update_callback.clone());
            }

            entity.updated = true;
        }

        // execute update functions
        for callback in callbacks {
            callback.call(game_io, simulation, vms, ());
        }
    }

    fn execute_attacks(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        for (_, living) in simulation.entities.query_mut::<&mut Living>() {
            living.hit = false;
        }

        // interactions between attack boxes and tiles
        for attack_box in &simulation.queued_attacks {
            let tile = simulation
                .field
                .tile_at_mut((attack_box.x, attack_box.y))
                .unwrap();
            tile.attempt_wash(attack_box.props.element);
            tile.attempt_wash(attack_box.props.secondary_element);

            if attack_box.highlight {
                tile.set_highlight(TileHighlight::Solid);
            }
        }

        // interactions between attack boxes and entities
        let mut needs_processing = Vec::new();

        for (id, (entity, living)) in simulation.entities.query_mut::<(&Entity, &Living)>() {
            if !entity.on_field || entity.deleted || !living.hitbox_enabled || living.health <= 0 {
                // entity can't be hit
                continue;
            }

            let mut attack_boxes = Vec::new();

            for attack_box in &simulation.queued_attacks {
                if entity.x != attack_box.x || entity.y != attack_box.y {
                    // not on the same tile
                    continue;
                }

                if attack_box.attacker_id == entity.id {
                    // can't hit self
                    continue;
                }

                if attack_box.team == entity.team {
                    // can't hit members of the same team
                    continue;
                }

                if living.ignore_common_aggressor
                    && Some(attack_box.props.aggressor) == living.aggressor
                {
                    // ignore common aggressor
                    // some entities shouldn't collide if they come from the same enemy (like Bubbles from the same Starfish)
                    continue;
                }

                attack_boxes.push(attack_box.clone());
            }

            needs_processing.push((
                id,
                living.intangibility.is_enabled(),
                living.defense_rules.clone(),
                attack_boxes,
            ));
        }

        for (id, intangible, defense_rules, attack_boxes) in needs_processing {
            for attack_box in attack_boxes {
                let AttackBox {
                    attacker_id,
                    props: hit_props,
                    ..
                } = attack_box;

                // collision callback
                let spell_entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(attacker_id.into())
                    .unwrap();

                let collision_callback = spell_entity.collision_callback.clone();
                let attack_callback = spell_entity.attack_callback.clone();

                collision_callback.call(game_io, simulation, vms, id.into());

                // defense check against DefenseOrder::Always
                simulation.defense_judge = DefenseJudge::new();

                DefenseJudge::judge(
                    game_io,
                    simulation,
                    vms,
                    id.into(),
                    attacker_id,
                    &defense_rules,
                    false,
                );

                if simulation.defense_judge.damage_blocked {
                    let tile = simulation
                        .field
                        .tile_at_mut((attack_box.x, attack_box.y))
                        .unwrap();
                    tile.ignore_attacker(attacker_id);
                }

                // test tangibility
                if intangible {
                    let living = simulation
                        .entities
                        .query_one_mut::<&mut Living>(id)
                        .unwrap();

                    if !living.intangibility.try_pierce(&hit_props) {
                        continue;
                    }
                }

                // defense check against DefenseOrder::CollisionOnly
                DefenseJudge::judge(
                    game_io,
                    simulation,
                    vms,
                    id.into(),
                    attacker_id,
                    &defense_rules,
                    false,
                );

                if simulation.defense_judge.damage_blocked {
                    continue;
                }

                let tile = simulation
                    .field
                    .tile_at_mut((attack_box.x, attack_box.y))
                    .unwrap();
                tile.ignore_attacker(attacker_id);

                Living::process_hit(game_io, simulation, vms, id.into(), hit_props);

                // spell attack callback
                attack_callback.call(game_io, simulation, vms, id.into());
            }

            let living = simulation
                .entities
                .query_one_mut::<&mut Living>(id)
                .unwrap();

            if living.intangibility.is_retangible() {
                living.intangibility.disable();

                if let Some(callback) = living.intangibility.deactivate_callback.take() {
                    callback.call(game_io, simulation, vms, ());
                }
            }
        }

        simulation.field.resolve_wash();
        simulation.queued_attacks.clear();
    }

    fn mark_deleted(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        let mut pending_deletion = Vec::new();

        for (_, (entity, living)) in simulation.entities.query_mut::<(&mut Entity, &Living)>() {
            if living.max_health == 0 || living.health > 0 {
                continue;
            }

            pending_deletion.push(entity.id);
        }

        for id in pending_deletion {
            simulation.delete_entity(game_io, vms, id);
        }
    }

    fn update_artifacts(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        let mut callbacks = Vec::new();

        for (_, (entity, _artifact)) in simulation.entities.query_mut::<(&mut Entity, &Artifact)>()
        {
            if entity.updated || !entity.spawned || entity.deleted {
                continue;
            }

            callbacks.push(entity.update_callback.clone());

            for index in entity.local_components.iter().cloned() {
                let component = simulation.components.get(index).unwrap();

                callbacks.push(component.update_callback.clone());
            }

            entity.updated = true;
        }

        // execute update functions
        for callback in callbacks {
            callback.call(game_io, simulation, vms, ());
        }
    }

    fn process_input(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        let mut card_action_callbacks = Vec::new();
        let mut movement_tests = Vec::new();

        for (id, (entity, player, character)) in
            simulation
                .entities
                .query_mut::<(&mut Entity, &mut Player, &mut Character)>()
        {
            let input = &simulation.inputs[player.index];

            // normal attack and charged attack
            let charge_sprite_node = &mut entity
                .sprite_tree
                .get_mut(player.charge_sprite_index)
                .unwrap();

            if entity.deleted {
                charge_sprite_node.set_visible(false);
                continue;
            }

            let action_active = (simulation.card_actions)
                .iter()
                .any(|(_, action)| action.used && action.entity == entity.id);

            let mut charging = false;

            if !action_active {
                if input.was_just_pressed(Input::UseCard) && !character.cards.is_empty() {
                    // using a card

                    let card_props = character.cards.pop().unwrap();

                    let entity_id = entity.id;
                    let package_id = card_props.package_id.clone();
                    let namespace = player.namespace();

                    let callback = BattleCallback::new(move |game_io, simulation, vms, _: ()| {
                        let vm_index = match BattleSimulation::find_vm(vms, &package_id, namespace)
                        {
                            Ok(vm_index) => vm_index,
                            _ => {
                                log::error!("failed to find vm for {package_id}");
                                return None;
                            }
                        };

                        let lua = &vms[vm_index].lua;
                        let card_init: rollback_mlua::Function =
                            match lua.globals().get("card_create_action") {
                                Ok(card_init) => card_init,
                                _ => {
                                    log::error!("{package_id} is missing card_create_action()");
                                    return None;
                                }
                            };

                        let api_ctx = RefCell::new(BattleScriptContext {
                            vm_index,
                            vms,
                            game_io,
                            simulation,
                        });

                        let lua_api = &game_io.globals().battle_api;
                        let mut id: Option<GenerationalIndex> = None;

                        lua_api.inject_dynamic(lua, &api_ctx, |lua| {
                            let entity_table = create_entity_table(lua, entity_id)?;
                            let table: rollback_mlua::Table = card_init.call(entity_table)?;

                            id = Some(table.raw_get("#id")?);
                            Ok(())
                        });

                        id
                    });

                    card_action_callbacks.push((id, callback));
                } else if input.was_just_pressed(Input::Special) {
                    let callback = player.special_attack_callback.clone();

                    card_action_callbacks.push((id, callback));
                }

                if input.is_down(Input::Shoot) {
                    // charging
                    player.charge_animator.update();

                    if player.charging_time == Player::CHARGE_DELAY {
                        // charging
                        player.charge_animator.set_state("CHARGING");
                        player.charge_animator.set_loop_mode(AnimatorLoopMode::Loop);
                        charge_sprite_node.set_color(Color::BLACK);
                        charge_sprite_node.set_visible(true);
                    } else if player.charging_time
                        == player.max_charging_time + Player::CHARGE_DELAY
                    {
                        // charged
                        player.charge_animator.set_state("CHARGED");
                        player.charge_animator.set_loop_mode(AnimatorLoopMode::Loop);
                        charge_sprite_node.set_color(player.charged_color);
                    }

                    // todo: sfx

                    charging = true;
                    player.charging_time += 1;
                    charge_sprite_node.apply_animation(&player.charge_animator);
                } else if player.charging_time > 0 {
                    // shooting

                    let callback =
                        if player.charging_time < player.max_charging_time + Player::CHARGE_DELAY {
                            // uncharged
                            player.normal_attack_callback.clone()
                        } else {
                            // charged
                            player.charged_attack_callback.clone()
                        };

                    card_action_callbacks.push((id, callback));
                }
            }

            if !charging {
                player.charging_time = 0;
                charge_sprite_node.set_visible(false);
            }

            // movement
            // can't move if there's a blocking card action
            if entity.card_action_index.is_some() {
                continue;
            }

            let anim = &simulation.animators[entity.animator_index];

            // can only move if there's no move action queued and the current animation is PLAYER_IDLE
            if entity.move_action.is_some() || anim.current_state() != Some("PLAYER_IDLE") {
                continue;
            }

            // todo: flip offsets from confusion

            let mut x_offset =
                input.is_down(Input::Right) as i32 - input.is_down(Input::Left) as i32;

            if entity.team == Team::Blue {
                // flipped perspective
                x_offset = -x_offset;
            }

            let tile_exists = simulation
                .field
                .tile_at_mut((entity.x + x_offset, entity.y))
                .is_some();

            if x_offset != 0 && tile_exists {
                movement_tests.push((
                    id,
                    entity.can_move_to_callback.clone(),
                    (entity.x + x_offset, entity.y),
                    player.slide_when_moving,
                ));
            }

            let y_offset = input.is_down(Input::Down) as i32 - input.is_down(Input::Up) as i32;
            let tile_exists = simulation
                .field
                .tile_at_mut((entity.x, entity.y + y_offset))
                .is_some();

            if y_offset != 0 && tile_exists {
                movement_tests.push((
                    id,
                    entity.can_move_to_callback.clone(),
                    (entity.x, entity.y + y_offset),
                    player.slide_when_moving,
                ));
            }
        }

        // try activating a card action
        for (id, card_action_callback) in card_action_callbacks {
            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(id)
                .unwrap();

            if entity.card_action_index.is_some() {
                // already set by a previous run
                continue;
            }

            if let Some(index) = card_action_callback.call(game_io, simulation, vms, ()) {
                let index = index.into();

                // validate index as it's coming straight from lua
                if let Some(card_action) = simulation.card_actions.get_mut(index) {
                    card_action.used = true;

                    let entity = simulation
                        .entities
                        .query_one_mut::<&mut Entity>(id)
                        .unwrap();
                    entity.card_action_index = Some(index);
                } else {
                    log::error!("received invalid CardAction index {index:?}");
                }
            }
        }

        // try movement
        for (id, can_move_to_callback, dest, slide) in movement_tests {
            // movement test
            if can_move_to_callback.call(game_io, simulation, vms, dest) {
                // statistics
                if let Ok(player) = simulation.entities.query_one_mut::<&Player>(id) {
                    if player.local {
                        simulation.statistics.movements += 1;
                    }
                }

                // actual movement
                let entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(id)
                    .unwrap();

                if slide {
                    entity.move_action = Some(MoveAction::slide(dest, 14));
                } else {
                    let mut move_event = MoveAction::teleport(dest);
                    move_event.delay_frames = 6;
                    move_event.endlag_frames = 7;

                    let anim_index = entity.animator_index;
                    let move_state = entity.move_anim_state.clone();

                    move_event.on_begin = Some(BattleCallback::new(move |_, simulation, _, _| {
                        let anim = &mut simulation.animators[anim_index];

                        if let Some(move_state) = move_state.as_ref() {
                            let callbacks = anim.set_state(move_state);
                            simulation.pending_callbacks.extend(callbacks);
                        }

                        // reset to PLAYER_IDLE when movement finishes
                        anim.on_complete(BattleCallback::new(move |_, simulation, _, _| {
                            let anim = &mut simulation.animators[anim_index];
                            let callbacks = anim.set_state("PLAYER_IDLE");
                            anim.set_loop_mode(AnimatorLoopMode::Loop);

                            simulation.pending_callbacks.extend(callbacks);
                        }));
                    }));

                    entity.move_action = Some(move_event);
                }
            }
        }
    }

    fn update_living(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        let mut callbacks = Vec::new();

        for (_, (entity, living)) in simulation
            .entities
            .query_mut::<(&mut Entity, &mut Living)>()
        {
            if !entity.time_is_frozen && !entity.updated && entity.spawned && !entity.deleted {
                callbacks.push(entity.update_callback.clone());

                for index in entity.local_components.iter().cloned() {
                    let component = simulation.components.get(index).unwrap();

                    callbacks.push(component.update_callback.clone());
                }

                entity.updated = true;
                // todo: process statuses

                living.intangibility.update();
            }

            if living.hit {
                let root_node = entity.sprite_tree.root_mut();
                root_node.set_color(Color::WHITE);
                root_node.set_color_mode(SpriteColorMode::Add);
            }
        }

        // execute update functions
        for callback in callbacks {
            callback.call(game_io, simulation, vms, ());
        }
    }

    fn process_action_queues(
        &mut self,
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
    ) {
        let tile_size = simulation.field.tile_size();
        let mut moving_entities = Vec::new();
        let mut actions_pending_deletion = Vec::new();

        for (id, entity) in simulation.entities.query_mut::<&mut Entity>() {
            if !entity.spawned || entity.deleted {
                continue;
            }

            if entity.move_action.is_some() {
                moving_entities.push(id);
            }
        }

        // movement
        // maybe this should happen in a separate update before everything else updates
        // that way scripts see accurate offsets
        for id in moving_entities {
            let mut entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(id)
                .unwrap();

            let mut move_action = match entity.move_action.as_mut() {
                Some(move_action) => move_action,
                None => continue,
            };

            if move_action.progress == 0 {
                move_action.source = (entity.x, entity.y);
            }

            if let Some(callback) = move_action.on_begin.take() {
                simulation.pending_callbacks.push(callback);
            }

            move_action.progress += 1;
            let animation_progress = move_action.animation_progress_percent();

            if animation_progress >= 0.5 && !move_action.success {
                // test to see if the movement is valid
                let dest = move_action.dest;
                let can_move_to_callback =
                    entity.current_can_move_to_callback(&simulation.card_actions);

                if simulation.field.tile_at_mut(dest).is_none()
                    || !can_move_to_callback.call(game_io, simulation, vms, dest)
                {
                    let entity = simulation
                        .entities
                        .query_one_mut::<&mut Entity>(id)
                        .unwrap();
                    entity.move_action = None;
                    continue;
                }

                // refetch entity to satisfy lifetime requirements
                entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(id)
                    .unwrap();
                move_action = match entity.move_action.as_mut() {
                    Some(move_action) => move_action,
                    None => continue,
                };

                // update tile position
                entity.x = dest.0;
                entity.y = dest.1;
                entity.last_successful_move = simulation.battle_time;

                let start_tile = simulation.field.tile_at_mut(move_action.source).unwrap();
                start_tile.unignore_attacker(entity.id);
                start_tile.entity_count -= 1;

                #[allow(clippy::collapsible_if)]
                if !entity.ignore_tile_effects {
                    if start_tile.state() == TileState::Cracked && start_tile.entity_count == 0 {
                        start_tile.set_state(TileState::Broken);
                    }
                }

                let current_tile = simulation.field.tile_at_mut((entity.x, entity.y)).unwrap();
                current_tile.entity_count += 1;

                move_action.success = true;
            }

            if move_action.is_complete() {
                let move_action = entity.move_action.take().unwrap();

                if move_action.success && !entity.ignore_tile_effects {
                    let current_tile = simulation.field.tile_at_mut((entity.x, entity.y)).unwrap();

                    match current_tile.state() {
                        TileState::Ice => {
                            let direction = move_action.aligned_direction();
                            let offset = direction.i32_vector();
                            let dest = (entity.x + offset.0, entity.y + offset.1);

                            let can_move_to_callback =
                                entity.current_can_move_to_callback(&simulation.card_actions);
                            let can_move_to =
                                can_move_to_callback.call(game_io, simulation, vms, dest);

                            if can_move_to {
                                entity = simulation
                                    .entities
                                    .query_one_mut::<&mut Entity>(id)
                                    .unwrap();
                                entity.move_action = Some(MoveAction::slide(dest, 4));
                            }
                        }
                        TileState::Sea => {
                            // todo: apply root
                        }
                        _ => {}
                    }
                }
            } else {
                // apply jump height
                let height_multiplier = crate::ease::quadratic(animation_progress);
                entity.tile_offset.y -= move_action.height * height_multiplier;

                // calculate slide
                let start = IVec2::from(move_action.source).as_vec2();
                let dest = IVec2::from(move_action.dest).as_vec2();

                let current = if animation_progress < 0.5 {
                    (dest - start) * animation_progress
                } else {
                    (dest - start) * (animation_progress - 1.0)
                };

                entity.tile_offset.x += current.x * tile_size.x;
                entity.tile_offset.y += current.y * tile_size.y;
            }
        }

        simulation.call_pending_callbacks(game_io, vms);

        let card_action_indices: Vec<_> = (simulation.card_actions)
            .iter()
            .filter(|(_, action)| action.used)
            .map(|(index, _)| index)
            .collect();

        // card actions
        for action_index in card_action_indices {
            let mut card_action = &mut simulation.card_actions[action_index];

            let entity = match simulation
                .entities
                .query_one_mut::<&mut Entity>(card_action.entity.into())
            {
                Ok(entity) => entity,
                _ => continue,
            };

            if !entity.spawned || entity.deleted {
                continue;
            }

            let animator_index = entity.animator_index;

            if card_action.startup_delay > 1 {
                if entity.move_action.is_none() {
                    card_action.startup_delay -= 1;
                }
                continue;
            }

            if card_action.completed {
                actions_pending_deletion.push(action_index);
                continue;
            }

            // execute
            if !card_action.executed {
                let entity_id = entity.id;

                // animations
                let animator = &mut simulation.animators[animator_index];
                card_action.prev_state = animator.current_state().map(String::from);

                if let Some(derived_frames) = card_action.derived_frames.take() {
                    card_action.state = animator.derive_state(&card_action.state, derived_frames);
                }

                let callbacks = animator.set_state(&card_action.state);
                simulation.pending_callbacks.extend(callbacks);

                // update entity sprite
                let sprite_node = entity.sprite_tree.root_mut();
                animator.apply(sprite_node);

                // execute callback
                if let Some(callback) = card_action.execute_callback.take() {
                    callback.call(game_io, simulation, vms, ());
                }

                // setup frame callbacks
                let card_action = &mut simulation.card_actions[action_index];
                let entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(entity_id.into())
                    .unwrap();
                let animator = &mut simulation.animators[animator_index];

                for (frame_index, callback) in std::mem::take(&mut card_action.frame_callbacks) {
                    animator.on_frame(frame_index, callback);
                }

                // animation end callback
                let animation_end_callback =
                    BattleCallback::new(move |game_io, simulation, vms, _| {
                        let card_action = match simulation.card_actions.get_mut(action_index) {
                            Some(card_action) => card_action,
                            None => return,
                        };

                        if let Some(callback) = card_action.animation_end_callback.clone() {
                            callback.call(game_io, simulation, vms, ());
                        }
                    });

                animator.on_complete(animation_end_callback.clone());
                animator.on_interrupt(animation_end_callback);

                // update attachments
                if let Some(sprite) = entity.sprite_tree.get_mut(card_action.sprite_index) {
                    sprite.set_visible(true);
                }

                for attachment in &mut card_action.attachments {
                    attachment.apply_animation(&mut entity.sprite_tree, &mut simulation.animators);
                }

                card_action.executed = true;
            }

            // update callback
            let card_action = &mut simulation.card_actions[action_index];

            if let Some(callback) = card_action.update_callback.clone() {
                callback.call(game_io, simulation, vms, ());
            }

            // steps
            let card_action = &mut simulation.card_actions[action_index];

            while card_action.step_index < card_action.steps.len() {
                let step = &mut card_action.steps[card_action.step_index];

                if !step.completed {
                    let callback = step.callback.clone();

                    callback.call(game_io, simulation, vms, ());
                    break;
                }

                card_action.step_index += 1;
            }

            // end
            let mut card_action = &mut simulation.card_actions[action_index];

            let is_complete = match card_action.lockout_type {
                ActionLockout::Animation => simulation.animators[animator_index].is_complete(),
                ActionLockout::Sequence => card_action.step_index >= card_action.steps.len(),
                ActionLockout::Async(frames) => card_action.active_frames >= frames,
            };

            card_action.active_frames += 1;

            // queue deletion
            if is_complete {
                actions_pending_deletion.push(action_index);
            }
        }

        simulation.delete_card_actions(game_io, vms, &actions_pending_deletion);
    }
}
