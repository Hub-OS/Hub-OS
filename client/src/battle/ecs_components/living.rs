use crate::battle::*;
use crate::bindable::*;
use crate::resources::*;
use crate::structures::SlotMap;
use framework::prelude::*;
use std::collections::HashMap;

#[derive(Clone)]
pub struct Living {
    pub hit: bool, // used for flashing white
    pub hitbox_enabled: bool,
    pub counterable: bool,
    pub health: i32,
    pub max_health: i32,
    pub intangibility: Intangibility,
    pub defense_rules: Vec<DefenseRule>,
    pub status_director: StatusDirector,
    pub status_callbacks: HashMap<HitFlags, Vec<BattleCallback>>,
    pub aux_props: SlotMap<AuxProp>,
    pub pending_defense: Vec<(Option<(EntityId, (i32, i32))>, HitProperties)>,
    /// (hit_props, modified_damage)
    pub pending_unblocked_hits: Vec<(HitProperties, i32)>,
}

impl Default for Living {
    fn default() -> Self {
        Self {
            hit: false,
            hitbox_enabled: true,
            counterable: false,
            health: 0,
            max_health: 0,
            intangibility: Intangibility::default(),
            defense_rules: Vec::new(),
            status_director: StatusDirector::default(),
            status_callbacks: HashMap::new(),
            aux_props: Default::default(),
            pending_defense: Vec::new(),
            pending_unblocked_hits: Vec::new(),
        }
    }
}

impl Living {
    pub fn set_health(&mut self, mut health: i32) {
        health = health.max(0);

        if self.max_health == 0 {
            self.max_health = health;
        } else if self.health <= 0 {
            // avoid healing entities pending deletion
            return;
        }

        self.health = health.min(self.max_health);
    }

    pub fn register_status_callback(&mut self, hit_flag: HitFlags, callback: BattleCallback) {
        let callbacks = self.status_callbacks.entry(hit_flag).or_default();
        callbacks.push(callback);
    }

    pub fn queue_hit(
        &mut self,
        attacker: Option<(EntityId, (i32, i32))>,
        mut hit_props: HitProperties,
    ) {
        // negative values are prevented as they may cause accidental healing
        hit_props.damage = hit_props.damage.max(0);
        self.pending_defense.push((attacker, hit_props));
    }

    pub fn queue_unblocked_hit(&mut self, mut hit_props: HitProperties, modified_damage: i32) {
        // negative values are prevented as they may cause accidental healing
        hit_props.damage = hit_props.damage.max(0);
        self.pending_unblocked_hits
            .push((hit_props, modified_damage));
    }

    pub fn add_aux_prop(&mut self, aux_prop: AuxProp) -> GenerationalIndex {
        self.aux_props.insert(aux_prop)
    }

    pub fn pre_hit_aux_props(aux_props: &mut SlotMap<AuxProp>) -> Vec<&mut AuxProp> {
        let mut aux_props: Vec<_> = aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().executes_pre_hit())
            .collect();

        aux_props.sort_by_key(|aux_props| aux_props.priority());

        aux_props
    }

    fn hit_prep_aux_props(aux_props: &mut SlotMap<AuxProp>) -> Vec<&mut AuxProp> {
        let mut aux_props: Vec<_> = aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().executes_hit_prep())
            .collect();

        aux_props.sort_by_key(|aux_props| aux_props.priority());

        aux_props
    }

    fn on_hit_aux_props(aux_props: &mut SlotMap<AuxProp>) -> Vec<&mut AuxProp> {
        let mut aux_props: Vec<_> = aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().executes_on_hit())
            .collect();

        aux_props.sort_by_key(|aux_props| aux_props.priority());

        aux_props
    }

    fn post_hit_aux_props(aux_props: &mut SlotMap<AuxProp>) -> Vec<&mut AuxProp> {
        let mut aux_props: Vec<_> = aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().executes_after_hit())
            .collect();

        aux_props.sort_by_key(|aux_props| aux_props.priority());

        aux_props
    }

    pub fn aux_prop_cleanup(simulation: &mut BattleSimulation, filter: impl Fn(&AuxProp) -> bool) {
        let entities = &mut simulation.entities;

        for (_, living) in entities.query_mut::<&mut Living>() {
            // mark auxprops as tested in case they haven't already been marked
            for aux_prop in living.aux_props.values_mut() {
                if filter(aux_prop) {
                    aux_prop.mark_tested();
                }
            }

            // delete completed auxprops
            living.aux_props.retain(|_, prop| !prop.completed());

            // reset tests for next run
            for aux_prop in living.aux_props.values_mut() {
                if filter(aux_prop) {
                    aux_prop.reset_tests();
                }
            }
        }
    }

    pub fn process_hits(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
    ) {
        let status_registry = &resources.status_registry;
        let entities = &mut simulation.entities;

        // gather body properties for aux props
        let Ok((entity, living, player, character, emotion_window, action_queue)) = entities
            .query_one_mut::<(
                &Entity,
                &mut Living,
                Option<&Player>,
                Option<&Character>,
                Option<&EmotionWindow>,
                Option<&ActionQueue>,
            )>(entity_id.into())
        else {
            return;
        };

        let mut hit_prop_list = std::mem::take(&mut living.pending_unblocked_hits);

        // aux props
        let mut total_damage: i32 = hit_prop_list
            .iter()
            .map(|(hit_props, _)| hit_props.damage)
            .sum();

        for aux_prop in living.aux_props.values_mut() {
            // using battle_time as auxprops can be frame temporary
            // thus can't depend on their own timers
            let time_frozen = simulation.time_freeze_tracker.time_is_frozen();
            aux_prop.process_time(time_frozen, simulation.battle_time);
            aux_prop.process_body(emotion_window, player, character, entity, action_queue);

            for (hit_props, _) in &hit_prop_list {
                aux_prop.process_hit(entity, living.health, living.max_health, hit_props);
            }
        }

        living.status_director.clear_immunity();

        // apply hit prep aux props
        for aux_prop in Living::hit_prep_aux_props(&mut living.aux_props) {
            aux_prop.process_health_calculations(living.health, living.max_health, total_damage);
            aux_prop.mark_tested();

            if !aux_prop.passed_all_tests() {
                continue;
            }

            aux_prop.mark_activated();

            match aux_prop.effect() {
                AuxEffect::StatusImmunity(hit_flags) => {
                    living.status_director.add_immunity(*hit_flags);
                }
                AuxEffect::ApplyStatus(hit_flag, duration) => {
                    living.status_director.apply_status(*hit_flag, *duration);
                }
                AuxEffect::RemoveStatus(hit_flag) => {
                    living.status_director.remove_statuses(*hit_flag);
                }
                _ => log::error!("Engine error: Unexpected AuxEffect!"),
            }

            simulation
                .pending_callbacks
                .extend(aux_prop.callbacks().iter().cloned());
        }

        // start processing hits
        let defense_rules = living.defense_rules.clone();
        let mut play_hurt_sfx = false;

        for (hit_props, modified_hit_damage) in &mut hit_prop_list {
            // filter statuses through defense rules
            let original_damage = hit_props.damage;

            Defense::filter_statuses(game_io, resources, simulation, hit_props, &defense_rules);

            let entities = &mut simulation.entities;
            let Ok((entity, living, countered_callback)) =
                entities.query_one_mut::<(&Entity, &mut Living, Option<&CounteredCallback>)>(
                    entity_id.into(),
                )
            else {
                return;
            };

            let mut modified_hit_damage = *modified_hit_damage;

            // super effective bonus
            if hit_props.is_super_effective(entity.element) {
                modified_hit_damage += hit_props.damage;
            }

            // apply hit modifying aux props
            for aux_prop in Living::on_hit_aux_props(&mut living.aux_props) {
                aux_prop.process_health_calculations(
                    living.health,
                    living.max_health,
                    total_damage,
                );
                aux_prop.mark_tested();

                if !aux_prop.hit_passes_tests(entity, living.health, living.max_health, hit_props) {
                    continue;
                }

                aux_prop.mark_activated();

                match aux_prop.effect() {
                    AuxEffect::IncreaseHitDamage(expr) => {
                        let result = expr.eval(AuxVariable::create_resolver(
                            living.health,
                            living.max_health,
                            hit_props.damage,
                        )) as i32;

                        modified_hit_damage += result.max(0);
                    }
                    AuxEffect::DecreaseHitDamage(expr) => {
                        let result = expr.eval(AuxVariable::create_resolver(
                            living.health,
                            living.max_health,
                            hit_props.damage,
                        )) as i32;

                        modified_hit_damage -= result.max(0);
                        // prevent accidental healing from stacking damage decreases
                        modified_hit_damage = modified_hit_damage.max(0);
                    }
                    _ => log::error!("Engine error: Unexpected AuxEffect!"),
                }

                simulation
                    .pending_callbacks
                    .extend(aux_prop.callbacks().iter().cloned());
            }

            // update total damage
            total_damage += modified_hit_damage - original_damage;

            if modified_hit_damage == 0 && original_damage != 0 {
                // no hit flags can apply if damage is changed to 0 by aux props
                continue;
            }

            // time freeze effects
            if entity.time_frozen {
                hit_props.context.flags |= HitFlag::NO_COUNTER;

                if hit_props.flags & HitFlag::DRAIN == 0 {
                    living.status_director.start_shake();
                }
            }

            if hit_props.flags & HitFlag::DRAIN == 0 {
                // used for flashing white
                living.hit = true
            }

            let countered = living.counterable
                && !living.status_director.is_inactionable(status_registry)
                && (hit_props.flags & HitFlag::DRAIN) == 0
                && (hit_props.context.flags & HitFlag::NO_COUNTER) == 0;

            if countered {
                // strip flashing if we've countered, to allow for a bonus hit
                hit_props.flags &= !HitFlag::FLASH;
            }

            // apply statuses
            let status_director = &mut living.status_director;
            status_director.apply_hit_flags(status_registry, hit_props.flags, &hit_props.durations);

            // handle counter
            if countered {
                living.status_director.apply_status(HitFlag::PARALYZE, 150);
                living.counterable = false;

                // notify self
                if let Some(callback) = countered_callback {
                    simulation.pending_callbacks.push(callback.0.clone());
                }

                // notify aggressor
                let aggressor_id = hit_props.context.aggressor;

                let notify_aggressor =
                    BattleCallback::new(move |game_io, resources, simulation, ()| {
                        let entities = &mut simulation.entities;
                        if let Ok(callback) =
                            entities.query_one_mut::<&CounterCallback>(aggressor_id.into())
                        {
                            let callback = callback.0.clone();
                            callback.call(game_io, resources, simulation, entity_id);
                        };

                        // play counter sfx if the attack was caused by the local player
                        if simulation.local_player_id == aggressor_id {
                            let globals = game_io.resource::<Globals>().unwrap();
                            simulation.play_sound(game_io, &globals.sfx.counter_hit);
                        }
                    });

                simulation.pending_callbacks.push(notify_aggressor);
            }

            // handle drag
            if hit_props.drags() && !living.status_director.is_dragged() {
                living.status_director.set_drag(hit_props.drag);

                // cancel movement
                Movement::cancel(simulation, entity_id);
            }

            play_hurt_sfx |= hit_props.flags & HitFlag::DRAIN == 0;
        }

        let (living, obstacle) = simulation
            .entities
            .query_one_mut::<(&mut Living, Option<&Obstacle>)>(entity_id.into())
            .unwrap();

        if play_hurt_sfx && !simulation.is_resimulation {
            let globals = game_io.resource::<Globals>().unwrap();

            if simulation.local_player_id == entity_id {
                globals.audio.play_sound(&globals.sfx.hurt);
            } else if obstacle.is_some() {
                globals.audio.play_sound(&globals.sfx.hurt_obstacle);
            } else {
                globals.audio.play_sound(&globals.sfx.hurt_opponent);
            }
        }

        // apply post hit aux props
        let mut health_modifier = 0;
        let mut modified_total_damage = total_damage;

        for aux_prop in Living::post_hit_aux_props(&mut living.aux_props) {
            aux_prop.process_health_calculations(living.health, living.max_health, total_damage);
            aux_prop.mark_tested();

            if !aux_prop.passed_all_tests() {
                continue;
            }

            aux_prop.mark_activated();

            match aux_prop.effect() {
                AuxEffect::DecreaseDamageSum(expr) => {
                    let result = expr.eval(AuxVariable::create_resolver(
                        living.health,
                        living.max_health,
                        modified_total_damage,
                    )) as i32;

                    modified_total_damage = (modified_total_damage - result.max(0)).max(0);
                }
                AuxEffect::DrainHP(drain) => health_modifier -= drain,
                AuxEffect::RecoverHP(recover) => health_modifier += recover,
                AuxEffect::None => {}
                _ => log::error!("Engine error: Unexpected AuxEffect!"),
            }

            simulation
                .pending_callbacks
                .extend(aux_prop.callbacks().iter().cloned())
        }

        if total_damage > 0 && modified_total_damage == 0 {
            // final damage should be at least 1 if it was already higher than zero
            total_damage = 1;
        } else {
            total_damage = modified_total_damage;
        }

        // apply damage and health modifier
        living.set_health(living.health - total_damage + health_modifier);

        // handle intangibility
        if living.intangibility.is_retangible() {
            living.intangibility.disable();

            for callback in living.intangibility.take_deactivate_callbacks() {
                callback.call(game_io, resources, simulation, ());
            }
        }

        simulation.call_pending_callbacks(game_io, resources);
    }

    pub fn cancel_actions_for_drag(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        if simulation.time_freeze_tracker.time_is_frozen() {
            return;
        }

        let mut pending_cancel = Vec::new();

        for (id, (entity, living)) in simulation.entities.query_mut::<(&Entity, &mut Living)>() {
            if !living.status_director.is_dragged() {
                continue;
            }

            let Some(animator) = simulation.animators.get(entity.animator_index) else {
                continue;
            };

            // only cancel actions if the entity supports flinching
            if animator.has_state("CHARACTER_HIT") {
                pending_cancel.push(id)
            }
        }

        for id in pending_cancel {
            Action::cancel_all(game_io, resources, simulation, id.into());
        }
    }

    pub fn queue_next_drag(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
    ) {
        let entities = &mut simulation.entities;
        let mut pending_movements = Vec::new();
        let mut pending_animation: Vec<EntityId> = Vec::new();

        for (id, (living, movement)) in entities.query_mut::<(&mut Living, Option<&Movement>)>() {
            if movement.is_some() {
                continue;
            }

            if !living.status_director.is_dragged() {
                continue;
            }

            let direction = living.status_director.take_next_drag_movement();

            if direction.is_none() {
                // drag ended, animate
                pending_animation.push(id.into());
                continue;
            }

            pending_movements.push((id, direction));
        }

        for (id, direction) in pending_movements {
            let entities = &mut simulation.entities;
            let Ok((entity, living)) = entities.query_one_mut::<(&Entity, &mut Living)>(id) else {
                continue;
            };

            let dest = IVec2::new(entity.x, entity.y) + IVec2::from(direction.i32_vector());

            // clear + backup lockout to allow can_move_to_funcs that check for immobilize to ignore it
            let status_director = &mut living.status_director;
            let old_lockout = status_director.remaining_drag_lockout();
            status_director.set_remaining_drag_lockout(0);

            let tile_exists = simulation.field.tile_at_mut(dest.into()).is_some();

            if !tile_exists
                || !Entity::can_move_to(game_io, resources, simulation, id.into(), dest.into())
            {
                let entities = &mut simulation.entities;

                if let Ok(living) = entities.query_one_mut::<&mut Living>(id) {
                    living.status_director.end_drag();

                    if old_lockout > 0 {
                        // restore old lockout
                        living
                            .status_director
                            .set_remaining_drag_lockout(old_lockout);
                    }

                    pending_animation.push(id.into());
                }

                continue;
            }

            let movement = Movement::slide(dest.into(), DRAG_PER_TILE_DURATION);
            simulation.entities.insert_one(id, movement).unwrap();
        }

        for entity_id in pending_animation {
            let entities = &mut simulation.entities;
            let Ok(entity) = entities.query_one_mut::<&Entity>(entity_id.into()) else {
                continue;
            };

            // display CHARACTER_HIT
            let animator_index = entity.animator_index;
            let animator = &mut simulation.animators[animator_index];

            if !animator.has_state("CHARACTER_HIT") {
                continue;
            }

            let callbacks = BattleAnimator::set_temp_derived_state(
                &mut simulation.animators,
                "CHARACTER_HIT",
                vec![(0, 0).into()],
                animator_index,
            );

            simulation.pending_callbacks.extend(callbacks);

            let animator = &mut simulation.animators[animator_index];
            animator.find_and_apply_to_target(&mut simulation.sprite_trees);

            BattleAnimator::sync_animators(
                &mut simulation.animators,
                &mut simulation.sprite_trees,
                &mut simulation.pending_callbacks,
                animator_index,
            );

            // queue an action to reset
            let state = String::from("CHARACTER_HIT");
            let Some(action_index) = Action::create(game_io, simulation, state, entity_id) else {
                continue;
            };

            let action = &mut simulation.actions[action_index];
            action.derived_frames = Some(vec![(0, 0).into()]);

            Action::queue_action(game_io, resources, simulation, entity_id, action_index);
        }
    }

    pub fn intercept_action(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        mut index: GenerationalIndex,
    ) -> Option<GenerationalIndex> {
        type Query<'a> = (
            &'a Entity,
            &'a mut Living,
            Option<&'a Character>,
            Option<&'a Player>,
            Option<&'a EmotionWindow>,
            Option<&'a ActionQueue>,
        );

        // handle aux props which intercept actions
        // loops until the action stops being replaced
        loop {
            let entities = &mut simulation.entities;
            let Ok((entity, living, character, player, emotion_window, action_queue)) =
                entities.query_one_mut::<Query>(entity_id.into())
            else {
                // not Living, without any auxprops we can just skip this loop
                return Some(index);
            };

            let mut intercept_callback = None;

            // validate index as it may be coming from lua
            let Some(action) = simulation.actions.get_mut(index) else {
                // invalid action
                return None;
            };

            for aux_prop in living.aux_props.values_mut() {
                if !aux_prop.effect().resolves_action() || aux_prop.activated() {
                    // skip unrelated aux_props
                    // also skip already activated aux_props to prevent infinite loops
                    continue;
                }

                aux_prop.process_body(emotion_window, player, character, entity, action_queue);
                aux_prop.process_card(Some(&action.properties));

                if !aux_prop.passed_all_tests() {
                    continue;
                }

                if matches!(aux_prop.effect(), AuxEffect::InterceptAction(_))
                    && intercept_callback.is_some()
                {
                    // action is already pending replacement
                    // we can try activation again in the next loop
                    continue;
                }

                aux_prop.mark_activated();

                match aux_prop.effect() {
                    AuxEffect::InterceptAction(callback) => {
                        intercept_callback = Some(callback.clone());
                    }
                    _ => log::error!("Engine error: Unexpected AuxEffect!"),
                }

                simulation
                    .pending_callbacks
                    .extend(aux_prop.callbacks().iter().cloned());
            }

            simulation.call_pending_callbacks(game_io, resources);

            let Some(intercept_callback) = intercept_callback else {
                // action remains the same, no need to loop
                return Some(index);
            };

            let new_index = intercept_callback.call(game_io, resources, simulation, index);

            if new_index != Some(index) {
                // delete the old action if we're not using it
                Action::delete_multi(game_io, resources, simulation, false, [index]);
            }

            if let Some(new_index) = new_index {
                // swap action
                index = new_index;
            } else {
                // resolved to no action
                return None;
            }
        }
    }

    #[must_use]
    pub fn modify_used_card(
        &mut self,
        card_properties: &mut CardProperties,
    ) -> Vec<BattleCallback> {
        let mut multiplier = 1.0;
        let mut callbacks = Vec::new();

        if card_properties.can_boost {
            for aux_prop in self.aux_props.values_mut() {
                let AuxEffect::IncreaseCardDamage(increase) = aux_prop.effect() else {
                    // skip unrelated aux_props
                    continue;
                };

                let increase = *increase;
                aux_prop.process_card(Some(card_properties));

                if !aux_prop.passed_all_tests() {
                    continue;
                }

                aux_prop.mark_activated();
                card_properties.damage += increase;
                card_properties.boosted_damage += increase;

                callbacks.extend(aux_prop.callbacks().iter().cloned());
            }
        }

        for aux_prop in self.aux_props.values_mut() {
            if !aux_prop.effect().executes_on_card_use() {
                // skip unrelated aux_props
                continue;
            }

            aux_prop.process_card(Some(card_properties));

            if !aux_prop.passed_all_tests() {
                continue;
            }

            aux_prop.mark_activated();

            match aux_prop.effect() {
                AuxEffect::IncreaseCardDamage(_) => {}
                AuxEffect::IncreaseCardMultiplier(increase) => multiplier += increase,
                _ => log::error!("Engine error: Unexpected AuxEffect!"),
            }

            callbacks.extend(aux_prop.callbacks().iter().cloned());
        }

        // todo: do :once() auxprops activate twice from not cleaning up on every call?

        // apply multiplier
        let original_damage = card_properties.damage - card_properties.boosted_damage;
        let new_damage = (card_properties.damage as f32 * multiplier.max(0.0)) as i32;

        card_properties.damage = new_damage;
        card_properties.boosted_damage = new_damage - original_damage;

        callbacks
    }

    pub fn predict_card_aux_damage(&self, card_properties: &CardProperties) -> i32 {
        let mut damage = 0;

        if !card_properties.can_boost {
            return 0;
        }

        for aux_prop in self.aux_props.values() {
            let AuxEffect::IncreaseCardDamage(increase) = aux_prop.effect() else {
                // skip unrelated aux_props
                continue;
            };

            // clone to avoid modifying the original
            let mut aux_prop = aux_prop.clone();

            aux_prop.process_card(Some(card_properties));

            if !aux_prop.passed_all_tests() {
                continue;
            }

            damage += increase;
        }

        damage
    }

    pub fn predict_card_multiplier(&self, card_properties: &CardProperties) -> f32 {
        let mut multiplier = 1.0;

        if card_properties.damage == 0 {
            // the multiplier doesn't matter if the damage is 0
            return multiplier;
        }

        for aux_prop in self.aux_props.values() {
            let AuxEffect::IncreaseCardMultiplier(increase) = aux_prop.effect() else {
                // skip unrelated aux_props
                continue;
            };

            // clone to avoid modifying the original
            let mut aux_prop = aux_prop.clone();

            aux_prop.process_card(Some(card_properties));

            if !aux_prop.passed_all_tests() {
                continue;
            }

            multiplier += increase;
        }

        multiplier.max(0.0)
    }

    pub fn attempt_interrupt(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        action_index: GenerationalIndex,
    ) {
        let Some(action) = simulation.actions.get_mut(action_index) else {
            return;
        };

        let entities = &mut simulation.entities;
        let Ok(living) = entities.query_one_mut::<&mut Living>(action.entity.into()) else {
            return;
        };

        for aux_prop in living.aux_props.values_mut() {
            if !aux_prop.effect().executes_on_current_action() {
                continue;
            }

            aux_prop.process_card(Some(&action.properties));

            if !aux_prop.passed_all_tests() {
                continue;
            }

            aux_prop.mark_activated();

            match aux_prop.effect() {
                AuxEffect::InterruptAction(callback) => {
                    let callback = callback.clone();
                    callback.call(game_io, resources, simulation, action_index);

                    Action::delete_multi(game_io, resources, simulation, true, [action_index]);
                }
                _ => log::error!("Engine error: Unexpected AuxEffect!"),
            }

            return;
        }
    }

    pub fn update_action_context(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        action_type: ActionTypes,
        entity_id: EntityId,
    ) {
        let entities = &mut simulation.entities;
        let id = entity_id.into();

        ActionQueue::ensure(entities, entity_id);

        // reset attack context
        let mut attack_context = AttackContext::new(entity_id);

        if entities.satisfies::<&Player>(id).unwrap_or_default() && action_type != ActionType::CARD
        {
            attack_context.flags = HitFlag::NO_COUNTER;
        }

        let _ = entities.insert_one(id, attack_context);

        // gather body properties for aux props
        let Ok((entity, living, player, character, emotion_window, action_queue)) = entities
            .query_one_mut::<(
                &mut Entity,
                &mut Living,
                Option<&Player>,
                Option<&Character>,
                Option<&EmotionWindow>,
                &mut ActionQueue,
            )>(id)
        else {
            return;
        };

        action_queue.action_type = action_type;

        let mut aux_props: Vec<_> = living
            .aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().resolves_action_context())
            .collect();
        aux_props.sort_by_key(|aux_prop| aux_prop.priority());

        for aux_prop in aux_props {
            aux_prop.process_body(
                emotion_window,
                player,
                character,
                entity,
                Some(action_queue),
            );

            if !aux_prop.passed_all_tests() {
                continue;
            }

            aux_prop.mark_activated();

            match aux_prop.effect() {
                AuxEffect::UpdateContext(callback) => {
                    let callback = callback.clone().bind(entity_id);
                    simulation.pending_callbacks.push(callback);
                }
                _ => log::error!("Engine error: Unexpected AuxEffect!"),
            }

            simulation
                .pending_callbacks
                .extend(aux_prop.callbacks().iter().cloned());
        }

        simulation.call_pending_callbacks(game_io, resources);
    }
}
