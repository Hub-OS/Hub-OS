use super::State;
use crate::battle::*;
use crate::bindable::*;
use crate::ease::inverse_lerp;
use crate::render::ui::{FontStyle, TextStyle};
use crate::render::*;
use crate::resources::*;
use framework::prelude::*;
use rand::Rng;

const GRACE_TIME: FrameTime = 5;

#[derive(Clone)]
pub struct BattleState {
    time: FrameTime,
    complete: bool,
    message: Option<(&'static str, FrameTime)>,
}

impl State for BattleState {
    fn clone_box(&self) -> Box<dyn State> {
        Box::new(self.clone())
    }

    fn next_state(&self, game_io: &GameIO) -> Option<Box<dyn State>> {
        if self.complete {
            Some(Box::new(CardSelectState::new(game_io)))
        } else {
            None
        }
    }

    fn allows_animation_updates(&self) -> bool {
        true
    }

    fn update(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        self.detect_battle_start(game_io, resources, simulation);

        // reset frame temporary variables
        self.prepare_updates(simulation);

        // allow cards to mutate
        Character::mutate_cards(game_io, resources, simulation);

        // new: process player input
        self.process_input(game_io, resources, simulation);

        Action::process_queues(game_io, resources, simulation);

        // update time freeze first as it affects the rest of the updates
        self.update_time_freeze(game_io, resources, simulation);

        // new: process movement and actions
        self.process_movement(game_io, resources, simulation);
        self.process_actions(game_io, resources, simulation);

        // update tiles
        self.update_field(game_io, resources, simulation);

        // update spells
        self.update_spells(game_io, resources, simulation);

        // todo: handle spells attacking on tiles they're not on?
        // this can unintentionally occur by queueing an attack outside of update_spells,
        // such as in a BattleStep component or callback

        // execute attacks
        self.execute_attacks(game_io, resources, simulation);

        // process 0 HP
        self.mark_deleted(game_io, resources, simulation);

        // new: update living, processes statuses
        self.update_living(game_io, resources, simulation);

        // update artifacts
        self.update_artifacts(game_io, resources, simulation);

        // update battle step components
        if !simulation.time_freeze_tracker.time_is_frozen() {
            for component in simulation.components.values() {
                if component.lifetime == ComponentLifetime::BattleStep {
                    let callback = component.update_callback.clone();
                    simulation.pending_callbacks.push(callback);
                }
            }
        }

        self.apply_status_vfx(game_io, resources, simulation);

        simulation.call_pending_callbacks(game_io, resources);

        if self.message.is_none() && !simulation.time_freeze_tracker.time_is_frozen() {
            // only update the time statistic if the battle is still going for the local player
            // and if time is not frozen
            simulation.statistics.time += 1;
        }

        // other players may still be in battle, and some components make use of this
        simulation.battle_time += 1;
        self.time += 1;

        self.detect_success_or_failure(simulation);
        self.update_turn_gauge(game_io, simulation);
    }

    fn draw_ui<'a>(
        &mut self,
        game_io: &'a GameIO,
        simulation: &mut BattleSimulation,
        sprite_queue: &mut SpriteColorQueue<'a>,
    ) {
        // win / lose message
        if let Some((text, start_time)) = self.message {
            const MESSAGE_INTRO_TIME: FrameTime = 10;

            let mut style = TextStyle::new(game_io, FontStyle::Battle);
            style.letter_spacing = 0.0;
            style.scale.y = inverse_lerp!(0, MESSAGE_INTRO_TIME, simulation.time - start_time);

            let size = style.measure(text).size;
            let position = (RESOLUTION_F - size * style.scale) * 0.5;
            style.bounds.set_position(position);

            style.draw(game_io, sprite_queue, text);
        } else {
            // turn gauge
            simulation.turn_gauge.draw(sprite_queue);
        }

        // time freeze
        let time_freeze_tracker = &simulation.time_freeze_tracker;
        time_freeze_tracker.draw_ui(game_io, simulation, sprite_queue);

        // trap indicators
        DefenseRule::draw_trap_ui(game_io, simulation, sprite_queue);

        // render top card text
        let entities = &mut simulation.entities;
        let entity_id = simulation.local_player_id;

        if let Ok(character) = entities.query_one_mut::<&Character>(entity_id.into()) {
            let mut actions_iter = simulation.actions.iter();
            let action_active =
                actions_iter.any(|(_, action)| action.entity == entity_id && action.used);

            if !action_active {
                // only render if there's no active / queued actions
                if let Some(card_props) = character.cards.last() {
                    // render on the bottom left
                    const MARGIN: Vec2 = Vec2::new(1.0, -1.0);

                    let line_height = TextStyle::new(game_io, FontStyle::Thick).line_height();
                    let position = Vec2::new(0.0, RESOLUTION_F.y - line_height) + MARGIN;

                    card_props.draw_summary(game_io, sprite_queue, position, false);
                }
            }
        }
    }
}

impl BattleState {
    pub fn new() -> Self {
        Self {
            time: 0,
            complete: false,
            message: None,
        }
    }

    fn update_turn_gauge(&mut self, game_io: &GameIO, simulation: &mut BattleSimulation) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            return;
        }

        let previously_incomplete = !simulation.turn_gauge.is_complete();

        simulation.turn_gauge.increment_time();

        if !simulation.turn_gauge.is_complete() || self.message.is_some() {
            // don't check for input if the battle has ended, or if the turn guage isn't complete
            return;
        }

        if previously_incomplete {
            // just completed, play a sfx
            let globals = game_io.resource::<Globals>().unwrap();
            simulation.play_sound(game_io, &globals.sfx.turn_gauge);
        }

        if simulation.config.turn_limit == Some(simulation.statistics.turns) {
            self.fail(simulation);
            return;
        }

        if simulation.config.automatic_turn_end {
            self.complete = true;
            return;
        }

        let mut player_iter = simulation
            .entities
            .query_mut::<(&Entity, &mut Player)>()
            .into_iter();

        self.complete = player_iter.any(|(_, (entity, player))| {
            if entity.deleted {
                return false;
            }

            let input = &simulation.inputs[player.index];
            input.was_just_pressed(Input::EndTurn)
        });
    }

    fn detect_success_or_failure(&mut self, simulation: &mut BattleSimulation) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            // allow the time freeze action to finish
            return;
        }

        const TOTAL_MESSAGE_TIME: FrameTime = 3 * 60;

        if let Some((_, time)) = self.message {
            if simulation.time - time >= TOTAL_MESSAGE_TIME {
                simulation.exit = true;
            }
            return;
        }

        // detect failure + find team for success detection
        let local_team;

        if let Ok(entity) =
            (simulation.entities).query_one_mut::<&Entity>(simulation.local_player_id.into())
        {
            if entity.deleted {
                self.fail(simulation);
                return;
            }

            local_team = entity.team;
        } else {
            self.fail(simulation);
            return;
        }

        // detect success
        // todo: score screen

        let enemies_alive = simulation
            .entities
            .query_mut::<(&Entity, &Character)>()
            .into_iter()
            .any(|(_, (entity, _))| entity.team != local_team);

        if !enemies_alive {
            self.succeed(simulation);
        }
    }

    fn fail(&mut self, simulation: &BattleSimulation) {
        self.message = Some(("<_FAILED_>", simulation.time));
    }

    fn succeed(&mut self, simulation: &BattleSimulation) {
        self.message = Some(("<_SUCCESS_>", simulation.time));
    }

    fn detect_battle_start(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
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

        simulation.call_pending_callbacks(game_io, resources);
    }

    pub fn update_time_freeze(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let time_freeze_tracker = &mut simulation.time_freeze_tracker;

        if !time_freeze_tracker.time_is_frozen() {
            return;
        }

        // update fade color
        const FADE_COLOR: Color = Color::new(0.0, 0.0, 0.0, 0.3);

        let fade_alpha = time_freeze_tracker.fade_alpha();
        let fade_color = FADE_COLOR.multiply_alpha(fade_alpha);
        simulation.fade_sprite.set_color(fade_color);

        // detect freeze start
        if time_freeze_tracker.should_freeze() {
            // freeze non artifacts
            let entities = &mut simulation.entities;

            for (_, entity) in entities.query_mut::<hecs::Without<&mut Entity, &Artifact>>() {
                entity.time_frozen_count += 1;
            }

            if time_freeze_tracker.is_action_freeze() {
                // play sfx
                simulation.play_sound(
                    game_io,
                    &game_io.resource::<Globals>().unwrap().sfx.time_freeze,
                );
            }
        }

        // detect time freeze counter
        let time_freeze_tracker = &mut simulation.time_freeze_tracker;

        if time_freeze_tracker.can_counter() {
            let entities = &mut simulation.entities;

            let player_ids: Vec<_> = entities
                .query_mut::<&Player>()
                .into_iter()
                .map(|(e, _)| e)
                .collect();

            let last_team = time_freeze_tracker.last_team().unwrap();

            for id in player_ids {
                let (entity, player, character) = entities
                    .query_one_mut::<(&Entity, &Player, &Character)>(id)
                    .unwrap();

                if entity.deleted || entity.team == last_team {
                    // can't counter
                    // can't counter a card from the same team
                    continue;
                }

                if !simulation.inputs[player.index].was_just_pressed(Input::UseCard) {
                    // didn't try to counter
                    continue;
                }

                if let Some(card_props) = character.cards.last() {
                    if !card_props.time_freeze {
                        // must counter with a time freeze card
                        continue;
                    }
                } else {
                    // no cards to counter with
                    continue;
                }

                Character::use_card(game_io, resources, simulation, id.into());
                break;
            }
        }

        // detect action start
        let time_freeze_tracker = &mut simulation.time_freeze_tracker;

        if time_freeze_tracker.action_should_start() {
            let action_index = time_freeze_tracker.active_action().unwrap();

            if let Some(action) = simulation.actions.get(action_index) {
                let entity_id = action.entity;

                // unfreeze our entity
                if let Ok((entity, living)) = simulation
                    .entities
                    .query_one_mut::<(&mut Entity, &mut Living)>(entity_id.into())
                {
                    entity.time_frozen_count = 0;

                    // back up their data
                    let animator = &mut simulation.animators[entity.animator_index];

                    // delete old status sprites
                    for (_, index) in living.status_director.take_status_sprites() {
                        entity.sprite_tree.remove(index);
                    }

                    time_freeze_tracker.back_up_character(
                        entity_id,
                        entity.action_index,
                        animator.clone(),
                        std::mem::take(&mut living.status_director),
                    );

                    entity.action_index = Some(action_index);

                    // reset callbacks as they'll run when the animator is reverted
                    animator.clear_callbacks();
                } else {
                    // entity erased?
                    time_freeze_tracker.end_action();
                    log::error!("Time freeze entity erased, yet action still exists?");
                }
            } else {
                // action deleted?
                time_freeze_tracker.end_action();
            }
        }

        // detect action end
        if let Some(index) = time_freeze_tracker.active_action() {
            if simulation.actions.get(index).is_none() {
                // action completed, update tracking
                time_freeze_tracker.end_action();
            }
        }

        // detect expiration
        let mut actions_pending_removal = Vec::new();
        time_freeze_tracker.increment_time();

        if time_freeze_tracker.action_out_of_time() {
            if let Some((entity_id, action_index, animator, status_director)) =
                time_freeze_tracker.take_character_backup()
            {
                if let Ok((entity, living)) = simulation
                    .entities
                    .query_one_mut::<(&mut Entity, &mut Living)>(entity_id.into())
                {
                    // freeze the entity again
                    entity.time_frozen_count = 1;

                    // delete action if it still exists
                    if let Some(index) = entity.action_index.take() {
                        actions_pending_removal.push(index);
                    }

                    entity.action_index = action_index;

                    // restore animator
                    simulation.animators[entity.animator_index] = animator;

                    // merge to retain statuses gained from time freeze
                    living.status_director.merge(status_director);
                }
            }

            time_freeze_tracker.advance_action();
        }

        // detect completion
        if time_freeze_tracker.should_defrost() {
            // unfreeze all entities
            for (_, entity) in simulation.entities.query_mut::<&mut Entity>() {
                if entity.time_frozen_count > 0 {
                    entity.time_frozen_count -= 1;
                }
            }
        }

        simulation.delete_actions(game_io, resources, actions_pending_removal);
    }

    fn prepare_updates(&self, simulation: &mut BattleSimulation) {
        // entities should only update once, clearing the flag that tracks this
        for (_, entity) in simulation.entities.query_mut::<&mut Entity>() {
            entity.updated = false;

            let sprite_node = entity.sprite_tree.root_mut();

            // reset frame temp properties
            entity.tile_offset = Vec2::ZERO;
            sprite_node.set_color(Color::BLACK);
            sprite_node.set_color_mode(SpriteColorMode::Add);
        }
    }

    pub fn update_field(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        simulation.field.reset_highlight();

        if simulation.time_freeze_tracker.time_is_frozen() {
            // skip tile effect processing if time is frozen
            return;
        }

        simulation.field.update_tiles(&mut simulation.entities);
        Field::apply_side_effects(game_io, resources, simulation);
    }

    fn update_spells(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut callbacks = Vec::new();

        for (_, (entity, spell)) in simulation.entities.query_mut::<(&mut Entity, &Spell)>() {
            if let Some(tile) = simulation.field.tile_at_mut((entity.x, entity.y)) {
                tile.set_highlight(spell.requested_highlight);
            }

            if entity.time_frozen_count > 0 || entity.updated || !entity.spawned || entity.deleted {
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
            callback.call(game_io, resources, simulation, ());
        }
    }

    fn execute_attacks(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        for (_, (entity, living)) in simulation.entities.query_mut::<(&Entity, &mut Living)>() {
            if entity.time_frozen_count == 0 {
                living.hit = false;
            }
        }

        let queued_attacks = std::mem::take(&mut simulation.queued_attacks);

        // interactions between attack boxes and tiles
        for attack_box in &queued_attacks {
            let field = &mut simulation.field;
            let Some(tile) = field.tile_at_mut((attack_box.x, attack_box.y)) else {
                continue;
            };

            // resolve wash
            let tile_state = &simulation.tile_states[tile.state_index()];
            let cleanser_element = tile_state.cleanser_element;

            let element = attack_box.props.element;
            let secondary_element = attack_box.props.secondary_element;

            if cleanser_element == Some(element) || cleanser_element == Some(secondary_element) {
                tile.set_washed(true);
            }

            // highlight
            if attack_box.highlight {
                tile.set_highlight(TileHighlight::Solid);
            }

            tile.acknowledge_attacker(attack_box.attacker_id);
        }

        // interactions between attack boxes and entities
        let mut needs_processing = Vec::new();
        let mut all_living_ids = Vec::new();

        for (id, (entity, living)) in simulation.entities.query_mut::<(&Entity, &Living)>() {
            all_living_ids.push(id);

            if !entity.on_field || entity.deleted || !living.hitbox_enabled || living.health <= 0 {
                // entity can't be hit
                continue;
            }

            let mut attack_boxes = Vec::new();

            for attack_box in &queued_attacks {
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
                let attacker_id = attack_box.attacker_id;

                // collision callback
                let entities = &mut simulation.entities;
                let Ok(spell) = entities.query_one_mut::<&Spell>(attacker_id.into()) else {
                    continue;
                };

                let collision_callback = spell.collision_callback.clone();
                let attack_callback = spell.attack_callback.clone();

                // defense check against DefenseOrder::Always
                simulation.defense_judge = DefenseJudge::new();

                DefenseJudge::judge(
                    game_io,
                    resources,
                    simulation,
                    id.into(),
                    &attack_box,
                    &defense_rules,
                    false,
                );

                if simulation.defense_judge.damage_blocked {
                    if let Some(tile) = simulation.field.tile_at_mut((attack_box.x, attack_box.y)) {
                        tile.ignore_attacker(attacker_id);
                    }
                }

                // test tangibility
                if intangible {
                    if let Ok(living) = simulation.entities.query_one_mut::<&mut Living>(id) {
                        if !living.intangibility.try_pierce(&attack_box.props) {
                            continue;
                        }
                    }
                }

                collision_callback.call(game_io, resources, simulation, id.into());

                // defense check against DefenseOrder::CollisionOnly
                DefenseJudge::judge(
                    game_io,
                    resources,
                    simulation,
                    id.into(),
                    &attack_box,
                    &defense_rules,
                    true,
                );

                if simulation.defense_judge.damage_blocked {
                    continue;
                }

                if let Some(tile) = simulation.field.tile_at_mut((attack_box.x, attack_box.y)) {
                    tile.ignore_attacker(attacker_id);
                }

                if let Ok(living) = simulation.entities.query_one_mut::<&mut Living>(id) {
                    living.pending_hits.push(attack_box.props);
                }

                // spell attack callback
                attack_callback.call(game_io, resources, simulation, id.into());
            }
        }

        for id in all_living_ids {
            Living::process_hits(game_io, resources, simulation, id.into());
        }

        simulation.field.resolve_wash_and_ignored_attackers();
    }

    fn mark_deleted(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut pending_deletion = Vec::new();

        for (_, (entity, living)) in simulation.entities.query_mut::<(&mut Entity, &Living)>() {
            if living.max_health == 0 || living.health > 0 {
                continue;
            }

            pending_deletion.push(entity.id);
        }

        for id in pending_deletion {
            simulation.delete_entity(game_io, resources, id);
        }
    }

    fn update_artifacts(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
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
            callback.call(game_io, resources, simulation, ());
        }
    }

    fn process_input(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        self.process_action_input(game_io, resources, simulation);
        self.process_movement_input(game_io, resources, simulation);
    }

    fn process_action_input(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let status_registry = &resources.status_registry;
        let entities = &mut simulation.entities;

        let player_ids: Vec<_> = entities
            .query_mut::<&Player>()
            .into_iter()
            .map(|(e, _)| e)
            .collect();

        for &id in &player_ids {
            if simulation.time_freeze_tracker.time_is_frozen() {
                // stop adding card actions if time freeze is starting
                // this way time freeze cards aren't eaten
                break;
            }

            let entities = &mut simulation.entities;

            let (entity, player, living, character) = entities
                .query_one_mut::<(&mut Entity, &mut Player, &Living, &mut Character)>(id)
                .unwrap();

            let input = &simulation.inputs[player.index];

            // normal attack and charged attack
            if entity.deleted || living.status_director.input_locked_out(status_registry) {
                player.cancel_charge();
                continue;
            }

            let action_active = (simulation.actions)
                .values()
                .any(|action| action.used && action.entity == entity.id);

            if action_active {
                player.cancel_charge();
                continue;
            }

            let animator = &simulation.animators[entity.animator_index];
            let is_idle = animator.current_state() == Some(Player::IDLE_STATE);

            if character.cards.is_empty() {
                character.card_use_requested = false;
            }

            if character.card_use_requested
                && entity.movement.is_none()
                && is_idle
                && self.time > GRACE_TIME
            {
                // wait until movement ends before adding a card action
                // this is to prevent time freeze cards from applying during movement
                // process_action_queues only prevents non time freeze actions from starting until movements end
                character.card_use_requested = false;

                Player::use_card(game_io, resources, simulation, id.into());
                continue;
            }

            // ignore other actions if we're waiting on a card
            if character.card_use_requested {
                continue;
            }

            if input.was_just_pressed(Input::Special) {
                Player::use_special_attack(game_io, resources, simulation, id.into());
            } else {
                Player::handle_charging(game_io, resources, simulation, id.into());
            }
        }

        // update charge sprites
        for &id in &player_ids {
            let entities = &mut simulation.entities;

            let (entity, player) = entities
                .query_one_mut::<(&mut Entity, &mut Player)>(id)
                .unwrap();

            player.card_charge.update_sprite(&mut entity.sprite_tree);
            player.buster_charge.update_sprite(&mut entity.sprite_tree);
        }
    }

    fn process_movement_input(
        &self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            // shouldn't move during time freeze
            return;
        }

        let mut movement_tests = Vec::new();

        let status_registry = &resources.status_registry;
        let entities = &mut simulation.entities;

        for (id, (entity, living, player)) in
            entities.query_mut::<(&mut Entity, &Living, &mut Player)>()
        {
            // can't move if there's a blocking action or immoble
            if entity.action_index.is_some()
                || !entity.action_queue.is_empty()
                || living.status_director.is_immobile(status_registry)
            {
                continue;
            }

            let input = &simulation.inputs[player.index];
            let anim = &simulation.animators[entity.animator_index];

            // test for flip requests
            let face_left = input.was_just_pressed(Input::FaceLeft);
            let face_right = input.was_just_pressed(Input::FaceRight);

            player.flip_requested |= (face_left && face_right)
                || (face_left && entity.facing != Direction::Left)
                || (face_right && entity.facing != Direction::Right);

            // can only move if there's no move action queued and the current animation is PLAYER_IDLE
            if entity.movement.is_some() || anim.current_state() != Some(Player::IDLE_STATE) {
                continue;
            }

            let confused = (living.status_director).remaining_status_time(HitFlag::CONFUSE) > 0;

            let mut x_offset =
                input.is_down(Input::Right) as i32 - input.is_down(Input::Left) as i32;

            if entity.team == Team::Blue {
                // flipped perspective
                x_offset = -x_offset;
            }

            if confused {
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

            let mut y_offset = input.is_down(Input::Down) as i32 - input.is_down(Input::Up) as i32;

            if confused {
                y_offset = -y_offset;
            }

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

            // handle flipping
            if player.flip_requested && player.can_flip {
                entity.facing = entity.facing.reversed();
            }
            player.flip_requested = false;
        }

        // try movement
        for (id, can_move_to_callback, dest, slide) in movement_tests {
            // movement test
            if can_move_to_callback.call(game_io, resources, simulation, dest) {
                // statistics
                if simulation.local_player_id == EntityId::from(id) {
                    simulation.statistics.movements += 1;
                }

                // actual movement
                let entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(id)
                    .unwrap();

                if slide {
                    entity.movement = Some(Movement::slide(dest, 14));
                } else {
                    let mut move_event = Movement::teleport(dest);
                    move_event.delay = 5;
                    move_event.endlag = 7;

                    let anim_index = entity.animator_index;
                    let move_state = entity.move_anim_state.clone();

                    move_event.on_begin = Some(BattleCallback::new(move |_, _, simulation, _| {
                        let anim = &mut simulation.animators[anim_index];

                        if let Some(move_state) = move_state.as_ref() {
                            let callbacks = anim.set_state(move_state);
                            simulation.pending_callbacks.extend(callbacks);
                        }

                        // reset to PLAYER_IDLE when movement finishes
                        anim.on_complete(BattleCallback::new(move |_, _, simulation, _| {
                            let anim = &mut simulation.animators[anim_index];
                            let callbacks = anim.set_state(Player::IDLE_STATE);
                            anim.set_loop_mode(AnimatorLoopMode::Loop);

                            simulation.pending_callbacks.extend(callbacks);
                        }));
                    }));

                    entity.movement = Some(move_event);
                }
            }
        }
    }

    fn update_living(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut callbacks = Vec::new();

        let status_registry = &resources.status_registry;
        let entities = &mut simulation.entities;

        for (id, (entity, living)) in entities.query::<(&mut Entity, &mut Living)>().into_iter() {
            if entity.time_frozen_count > 0 || entity.updated || !entity.spawned || entity.deleted {
                continue;
            }

            if !living.status_director.is_inactionable(status_registry) {
                callbacks.push(entity.update_callback.clone());

                let mut player_query = entities.query_one::<&Player>(id).unwrap();

                if let Some(player) = player_query.get() {
                    if let Some(callback) = player.active_form_update_callback() {
                        callbacks.push(callback.clone());
                    }
                }

                for index in entity.local_components.iter().cloned() {
                    let component = simulation.components.get(index).unwrap();

                    callbacks.push(component.update_callback.clone());
                }
            }

            entity.updated = true;

            if living.status_director.is_dragged() && entity.movement.is_none() {
                // let the status director know we're no longer being dragged
                living.status_director.end_drag()
            }

            // process statuses as long as the entity isn't being dragged
            if !living.status_director.is_dragged() {
                let status_director = &mut living.status_director;
                status_director.update(status_registry, &simulation.inputs);

                // status destructors
                callbacks.extend(status_director.take_ready_destructors());

                // new status callbacks
                for hit_flag in status_director.take_new_statuses() {
                    if hit_flag & HitFlag::FLASH != HitFlag::NONE {
                        // apply intangible

                        // callback will keep the status director in sync when intangibility is pierced
                        let callback = BattleCallback::new(move |_, _, simulation, _| {
                            let living = simulation
                                .entities
                                .query_one_mut::<&mut Living>(id)
                                .unwrap();

                            living.status_director.remove_status(HitFlag::FLASH)
                        });

                        living.intangibility.enable(IntangibleRule {
                            duration: living.status_director.remaining_status_time(hit_flag),
                            deactivate_callback: Some(callback),
                            ..Default::default()
                        });
                    }

                    if let Some(callback) = status_registry.status_constructor(hit_flag) {
                        callbacks.push(callback.bind(id.into()));
                    }

                    // call registered status callbacks
                    if let Some(status_callbacks) = living.status_callbacks.get(&hit_flag) {
                        callbacks.extend(status_callbacks.iter().cloned());
                    }
                }
            }

            // update intangibility
            living.intangibility.update();

            let deactivate_callbacks = living.intangibility.take_deactivate_callbacks();
            callbacks.extend(deactivate_callbacks);
        }

        // execute update functions
        for callback in callbacks {
            callback.call(game_io, resources, simulation, ());
        }
    }

    fn process_movement(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let status_registry = &resources.status_registry;
        let tile_size = simulation.field.tile_size();
        let mut moving_entities = Vec::new();

        for (id, entity) in simulation.entities.query::<&Entity>().into_iter() {
            let mut update_progress =
                entity.spawned && !entity.deleted && entity.time_frozen_count == 0;

            if let Some(living) = simulation.entities.query_one::<&Living>(id).unwrap().get() {
                if living.status_director.is_immobile(status_registry) {
                    update_progress = false;
                }
            }

            if entity.movement.is_some() {
                moving_entities.push((id, update_progress));
            }
        }

        // movement
        // maybe this should happen in a separate update before everything else updates
        // that way scripts see accurate offsets
        for (id, update_progress) in moving_entities {
            let mut entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(id)
                .unwrap();

            let Some(movement) = entity.movement.as_mut() else {
                continue;
            };

            if movement.elapsed == 0 {
                entity.last_movement_time = simulation.battle_time;
                movement.source = (entity.x, entity.y);
            }

            if let Some(callback) = movement.on_begin.take() {
                simulation.pending_callbacks.push(callback);
            }

            if update_progress {
                movement.elapsed += 1;
            }

            if !movement.is_in_endlag() {
                entity.last_movement_time = simulation.battle_time;
            }

            let animation_progress = movement.animation_progress_percent();

            if animation_progress >= 0.5 && !movement.success {
                // test to see if the movement is valid
                let dest = movement.dest;
                let can_move_to_callback = entity.current_can_move_to_callback(&simulation.actions);

                if simulation.field.tile_at_mut(dest).is_none()
                    || !can_move_to_callback.call(game_io, resources, simulation, dest)
                {
                    let entity = simulation
                        .entities
                        .query_one_mut::<&mut Entity>(id)
                        .unwrap();
                    entity.movement = None;
                    continue;
                }

                // refetch entity to satisfy lifetime requirements
                entity = simulation
                    .entities
                    .query_one_mut::<&mut Entity>(id)
                    .unwrap();
                let Some(movement) = entity.movement.as_mut() else {
                    continue;
                };

                // update tile position
                entity.x = dest.0;
                entity.y = dest.1;
                movement.success = true;

                let start_tile = simulation.field.tile_at_mut(movement.source).unwrap();
                start_tile.handle_auto_reservation_removal(&simulation.actions, entity);

                if !entity.ignore_negative_tile_effects {
                    let movement = entity.movement.clone().unwrap();

                    // process stepping off the tile
                    let tile_state = &simulation.tile_states[start_tile.state_index()];
                    let tile_callback = tile_state.entity_leave_callback.clone();
                    let params = (entity.id, movement);

                    let callback =
                        BattleCallback::new(move |game_io, resources, simulation, ()| {
                            tile_callback.call(game_io, resources, simulation, params.clone());
                        });

                    simulation.pending_callbacks.push(callback);

                    // process stepping onto the new tile
                    let tile_state = &simulation.tile_states[start_tile.state_index()];
                    let tile_callback = tile_state.entity_enter_callback.clone();
                    let params = entity.id;

                    let callback =
                        BattleCallback::new(move |game_io, resources, simulation, ()| {
                            tile_callback.call(game_io, resources, simulation, params);
                        });

                    simulation.pending_callbacks.push(callback);
                }

                let current_tile = simulation.field.tile_at_mut((entity.x, entity.y)).unwrap();
                current_tile.handle_auto_reservation_addition(&simulation.actions, entity);
            }

            let Some(movement) = entity.movement.as_mut() else {
                continue;
            };

            if movement.is_complete() {
                let movement = entity.movement.take().unwrap();

                if movement.success && !entity.ignore_negative_tile_effects {
                    let current_tile = simulation.field.tile_at_mut((entity.x, entity.y)).unwrap();

                    let tile_state = &simulation.tile_states[current_tile.state_index()];
                    let tile_callback = tile_state.entity_stop_callback.clone();
                    let entity_id = entity.id;

                    let callback =
                        BattleCallback::new(move |game_io, resources, simulation, ()| {
                            tile_callback.call(
                                game_io,
                                resources,
                                simulation,
                                (entity_id, movement.clone()),
                            );
                        });

                    simulation.pending_callbacks.push(callback);
                }
            } else {
                // apply jump height
                let height_multiplier = crate::ease::quadratic(animation_progress);
                entity.tile_offset.y -= movement.height * height_multiplier;

                // calculate slide
                let start = IVec2::from(movement.source).as_vec2();
                let dest = IVec2::from(movement.dest).as_vec2();

                let current = if animation_progress < 0.5 {
                    (dest - start) * animation_progress
                } else {
                    (dest - start) * (animation_progress - 1.0)
                };

                entity.tile_offset.x += current.x * tile_size.x;
                entity.tile_offset.y += current.y * tile_size.y;
            }
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    fn process_actions(
        &mut self,
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let mut actions_pending_deletion = Vec::new();
        let time_is_frozen = simulation.time_freeze_tracker.time_is_frozen();

        let action_indices: Vec<_> = (simulation.actions)
            .iter()
            .filter(|(_, action)| action.used)
            .map(|(index, _)| index)
            .collect();

        // card actions
        for action_index in action_indices {
            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            if time_is_frozen && !action.properties.time_freeze {
                // non time freeze action in time freeze
                continue;
            }

            let entities = &mut simulation.entities;
            let entity_id = action.entity;

            let Ok(entity) = entities.query_one_mut::<&mut Entity>(entity_id.into()) else {
                continue;
            };

            if !entity.spawned || entity.deleted || entity.time_frozen_count > 0 {
                continue;
            }

            let animator_index = entity.animator_index;

            // execute
            if !action.executed {
                if entity.movement.is_some() {
                    continue;
                }

                if !simulation.is_entity_actionable(resources, entity_id) {
                    continue;
                }

                Action::execute(game_io, resources, simulation, action_index);
            }

            // update callback
            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            if let Some(callback) = action.update_callback.clone() {
                callback.call(game_io, resources, simulation, ());
            }

            // steps
            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            while action.step_index < action.steps.len() {
                let step = &mut action.steps[action.step_index];

                if !step.completed {
                    let callback = step.callback.clone();

                    callback.call(game_io, resources, simulation, ());
                    break;
                }

                action.step_index += 1;
            }

            // handling async card actions
            let Some(action) = simulation.actions.get_mut(action_index) else {
                continue;
            };

            let animation_completed = simulation.animators[animator_index].is_complete();

            let entity = simulation
                .entities
                .query_one_mut::<&mut Entity>(action.entity.into())
                .unwrap();

            if action.is_async() && animation_completed && entity.action_index == Some(action_index)
            {
                // async action completed sync portion
                action.complete_sync(
                    &mut simulation.entities,
                    &mut simulation.animators,
                    &mut simulation.pending_callbacks,
                    &mut simulation.field,
                );
            }

            // detecting end
            let is_complete = match action.lockout_type {
                ActionLockout::Animation => animation_completed,
                ActionLockout::Sequence => action.step_index >= action.steps.len(),
                ActionLockout::Async(frames) => action.active_frames >= frames,
            };

            action.active_frames += 1;

            if is_complete {
                // queue deletion
                actions_pending_deletion.push(action_index);
            }
        }

        simulation.delete_actions(game_io, resources, actions_pending_deletion);
    }

    fn apply_status_vfx(
        &self,
        game_io: &GameIO,
        shared_assets: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        for (_, (entity, living)) in simulation
            .entities
            .query_mut::<(&mut Entity, &mut Living)>()
        {
            let sprite_tree = &mut entity.sprite_tree;
            let status_director = &mut living.status_director;

            if status_director.status_lifetime(HitFlag::FREEZE).is_some() {
                let root_sprite = sprite_tree.root_mut();
                root_sprite.set_color_mode(SpriteColorMode::Add);
                root_sprite.set_color(Color::new(0.7, 0.8, 0.9, 1.0));
            }

            if let Some(lifetime) = status_director.status_lifetime(HitFlag::PARALYZE) {
                if (lifetime / 2) % 2 == 0 {
                    let root_sprite = sprite_tree.root_mut();
                    root_sprite.set_color_mode(SpriteColorMode::Add);
                    root_sprite.set_color(Color::YELLOW);
                }
            }

            if let Some(lifetime) = status_director.status_lifetime(HitFlag::ROOT) {
                if (lifetime / 2) % 2 == 0 {
                    let root_sprite = sprite_tree.root_mut();
                    root_sprite.set_color_mode(SpriteColorMode::Multiply);
                    root_sprite.set_color(Color::BLACK);
                }
            }

            if let Some(lifetime) = status_director.status_lifetime(HitFlag::FLASH) {
                if (lifetime / 2) % 2 == 0 {
                    let root_sprite = sprite_tree.root_mut();
                    root_sprite.set_color(Color::TRANSPARENT);
                }
            }

            if status_director.is_shaking() {
                entity.tile_offset.x += simulation.rng.gen_range(-1..=1) as f32;
                status_director.decrement_shake_time();
            }

            if living.hit {
                let root_node = entity.sprite_tree.root_mut();
                root_node.set_color(Color::WHITE);
                root_node.set_color_mode(SpriteColorMode::Add);
            }

            status_director.update_status_sprites(game_io, shared_assets, entity);
        }
    }
}
