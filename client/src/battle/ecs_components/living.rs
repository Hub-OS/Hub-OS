use crate::battle::*;
use crate::bindable::*;
use crate::resources::*;
use crate::structures::DenseSlotMap;
use framework::prelude::*;
use std::collections::HashMap;

#[derive(Clone)]
pub struct Living {
    pub hit: bool, // used for flashing white
    pub hitbox_enabled: bool,
    pub counterable: bool,
    pub health: i32,
    pub max_health: i32,
    pub pending_damage: i32,
    pub intangibility: Intangibility,
    pub defense_rules: Vec<DefenseRule>,
    pub status_director: StatusDirector,
    pub status_callbacks: HashMap<HitFlags, Vec<BattleCallback>>,
    pub countered_callback: BattleCallback,
    pub aux_props: DenseSlotMap<AuxProp>,
    pub pending_hits: Vec<HitProperties>,
}

impl Default for Living {
    fn default() -> Self {
        Self {
            hit: false,
            hitbox_enabled: true,
            counterable: false,
            health: 0,
            max_health: 0,
            pending_damage: 0,
            intangibility: Intangibility::default(),
            defense_rules: Vec::new(),
            status_director: StatusDirector::default(),
            status_callbacks: HashMap::new(),
            countered_callback: BattleCallback::default(),
            aux_props: Default::default(),
            pending_hits: Vec::new(),
        }
    }
}

impl Living {
    pub fn set_health(&mut self, mut health: i32) {
        health = health.max(0);

        if self.max_health == 0 {
            self.max_health = health;
        }

        self.health = health.min(self.max_health);
    }

    pub fn register_status_callback(&mut self, hit_flag: HitFlags, callback: BattleCallback) {
        let callbacks = self.status_callbacks.entry(hit_flag).or_default();
        callbacks.push(callback);
    }

    pub fn add_aux_prop(&mut self, aux_prop: AuxProp) -> GenerationalIndex {
        self.aux_props.insert(aux_prop)
    }

    fn pre_hit_aux_props(aux_props: &mut DenseSlotMap<AuxProp>) -> Vec<&mut AuxProp> {
        let mut aux_props: Vec<_> = aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().execute_before_hit())
            .collect();

        aux_props.sort_by_key(|aux_props| aux_props.priority());

        aux_props
    }

    fn on_hit_aux_props(aux_props: &mut DenseSlotMap<AuxProp>) -> Vec<&mut AuxProp> {
        let mut aux_props: Vec<_> = aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().execute_on_hit())
            .collect();

        aux_props.sort_by_key(|aux_props| aux_props.priority());

        aux_props
    }

    fn post_hit_aux_props(aux_props: &mut DenseSlotMap<AuxProp>) -> Vec<&mut AuxProp> {
        let mut aux_props: Vec<_> = aux_props
            .values_mut()
            .filter(|aux_prop| aux_prop.effect().execute_after_hit())
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
        let Ok((entity, living, player, character)) =
            entities.query_one_mut::<(&Entity, &mut Living, Option<&Player>, Option<&Character>)>(
                entity_id.into(),
            )
        else {
            return;
        };

        let mut hit_prop_list = std::mem::take(&mut living.pending_hits);

        // aux props
        let mut total_damage: i32 = hit_prop_list.iter().map(|hit_props| hit_props.damage).sum();

        for aux_prop in living.aux_props.values_mut() {
            // using battle_time as auxprops can be frame temporary
            // thus can't depend on their own timers
            let time_frozen = simulation.time_freeze_tracker.time_is_frozen();
            aux_prop.process_time(time_frozen, simulation.battle_time);
            aux_prop.process_body(player, character, entity);

            for hit_props in &mut hit_prop_list {
                aux_prop.process_hit(entity, living.health, living.max_health, hit_props);
            }
        }

        living.status_director.clear_immunity();

        // apply pre hit aux props
        for aux_prop in Living::pre_hit_aux_props(&mut living.aux_props) {
            aux_prop.process_health_calculations(living.health, living.max_health, total_damage);
            aux_prop.mark_tested();

            if !aux_prop.passed_all_tests() {
                continue;
            }

            aux_prop.mark_activated();

            match aux_prop.effect() {
                AuxEffect::StatusImmunity(hit_flags) => {
                    living.status_director.add_immunity(*hit_flags)
                }
                AuxEffect::ApplyStatus(hit_flag, duration) => {
                    living.status_director.apply_status(*hit_flag, *duration)
                }
                AuxEffect::RemoveStatus(hit_flag) => {
                    living.status_director.remove_status(*hit_flag)
                }
                _ => log::error!("Engine error: Unexpected AuxEffect!"),
            }

            simulation
                .pending_callbacks
                .extend(aux_prop.callbacks().iter().cloned());
        }

        // start processing hits
        let defense_rules = living.defense_rules.clone();

        for hit_props in &mut hit_prop_list {
            // filter statuses through defense rules
            DefenseJudge::filter_statuses(
                game_io,
                resources,
                simulation,
                hit_props,
                &defense_rules,
            );

            let entities = &mut simulation.entities;
            let Ok((entity, living)) =
                entities.query_one_mut::<(&Entity, &mut Living)>(entity_id.into())
            else {
                return;
            };

            let mut hit_damage = hit_props.damage;

            // super effective bonus
            if hit_props.is_super_effective(entity.element) {
                hit_damage += hit_props.damage;
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

                        if result < hit_props.damage {
                            log::warn!("An AuxProp is decreasing damage with an increasing effect");
                        }

                        hit_damage += result - hit_props.damage;
                    }
                    AuxEffect::DecreaseHitDamage(expr) => {
                        let result = expr.eval(AuxVariable::create_resolver(
                            living.health,
                            living.max_health,
                            hit_props.damage,
                        )) as i32;

                        if result > hit_props.damage {
                            log::warn!("An AuxProp is increasing damage with a decreasing effect");
                        }

                        hit_damage += result - hit_props.damage;
                    }
                    _ => log::error!("Engine error: Unexpected AuxEffect!"),
                }

                simulation
                    .pending_callbacks
                    .extend(aux_prop.callbacks().iter().cloned());
            }

            // update total damage
            total_damage += hit_damage - hit_props.damage;

            if hit_damage == 0 {
                // no hit flags can apply if damage is 0
                continue;
            }

            // time freeze effects
            if entity.time_frozen {
                hit_props.flags |= HitFlag::SHAKE;
                hit_props.context.flags |= HitFlag::NO_COUNTER;
            }

            if hit_props.flags & HitFlag::IMPACT != 0 {
                // used for flashing white
                living.hit = true
            }

            // handle counter
            if living.counterable
                && !living.status_director.is_inactionable(status_registry)
                && (hit_props.flags & HitFlag::IMPACT) == HitFlag::IMPACT
                && (hit_props.context.flags & HitFlag::NO_COUNTER) == 0
            {
                living.status_director.apply_status(HitFlag::PARALYZE, 150);
                living.counterable = false;

                // notify self
                let self_callback = living.countered_callback.clone();
                simulation.pending_callbacks.push(self_callback);

                // notify aggressor
                let aggressor_id = hit_props.context.aggressor;

                let notify_aggressor =
                    BattleCallback::new(move |game_io, resources, simulation, ()| {
                        let entities = &mut simulation.entities;
                        let Ok(aggressor_entity) =
                            entities.query_one_mut::<&Entity>(aggressor_id.into())
                        else {
                            return;
                        };

                        let callback = aggressor_entity.counter_callback.clone();
                        callback.call(game_io, resources, simulation, entity_id);

                        // play counter sfx if the attack was caused by the local player
                        if simulation.local_player_id == aggressor_id {
                            let globals = game_io.resource::<Globals>().unwrap();
                            simulation.play_sound(game_io, &globals.sfx.counter_hit);
                        }
                    });

                simulation.pending_callbacks.push(notify_aggressor);
            }

            // apply statuses
            let status_director = &mut living.status_director;
            status_director.apply_hit_flags(status_registry, hit_props.flags);

            // handle drag
            if hit_props.drags() && entity.movement.is_none() {
                let can_move_to_callback = entity.can_move_to_callback.clone();
                let delta: IVec2 = hit_props.drag.direction.i32_vector().into();

                let mut dest = IVec2::new(entity.x, entity.y);
                let mut duration = 0;

                for _ in 0..hit_props.drag.count {
                    dest += delta;

                    let tile_exists = simulation.field.tile_at_mut(dest.into()).is_some();

                    if !tile_exists
                        || !can_move_to_callback.call(game_io, resources, simulation, dest.into())
                    {
                        dest -= delta;
                        break;
                    }

                    duration += DRAG_PER_TILE_DURATION;
                }

                if duration != 0 {
                    let entity = (simulation.entities)
                        .query_one_mut::<&mut Entity>(entity_id.into())
                        .unwrap();

                    entity.movement = Some(Movement::slide(dest.into(), duration));
                }
            }
        }

        let living = simulation
            .entities
            .query_one_mut::<&mut Living>(entity_id.into())
            .unwrap();

        // apply post hit aux props
        let mut health_modifier = 0;

        for aux_prop in Living::post_hit_aux_props(&mut living.aux_props) {
            aux_prop.process_health_calculations(living.health, living.max_health, total_damage);
            aux_prop.mark_tested();

            if !aux_prop.passed_all_tests() {
                continue;
            }

            aux_prop.mark_activated();

            match aux_prop.effect() {
                AuxEffect::DecreaseDamageSum(expr) => {
                    let damage = expr.eval(AuxVariable::create_resolver(
                        living.health,
                        living.max_health,
                        total_damage,
                    )) as i32;

                    if total_damage > 0 && damage <= 0 {
                        // final damage should be at least 1 if it was already higher than zero
                        total_damage = 1;
                    } else {
                        total_damage = damage;
                    }
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

    pub fn intercept_action(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        mut index: GenerationalIndex,
    ) -> Option<GenerationalIndex> {
        // handle aux props which intercept actions
        // loops until the action stops being replaced
        loop {
            let entities = &mut simulation.entities;
            let Ok(living) = entities.query_one_mut::<&mut Living>(entity_id.into()) else {
                // not Living, without any auxprops we can just skip this loop
                return None;
            };

            let mut intercept_callback = None;

            // validate index as it may be coming from lua
            let Some(action) = simulation.actions.get_mut(index) else {
                // invalid action
                return None;
            };

            for aux_prop in living.aux_props.values_mut() {
                if !aux_prop.effect().action_queue_related() || aux_prop.activated() {
                    // skip unrelated aux_props
                    // also skip already activated aux_props to prevent infinite loops
                    continue;
                }

                aux_prop.process_action(Some(action));

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
                Action::delete_multi(game_io, resources, simulation, [index]);
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
            if !aux_prop.effect().action_related() {
                continue;
            }

            aux_prop.process_action(Some(action));

            if !aux_prop.passed_all_tests() {
                continue;
            }

            match aux_prop.effect() {
                AuxEffect::InterruptAction(callback) => {
                    let callback = callback.clone();
                    callback.call(game_io, resources, simulation, action_index);

                    Action::delete_multi(game_io, resources, simulation, [action_index]);
                }
                _ => log::error!("Engine error: Unexpected AuxEffect!"),
            }

            return;
        }
    }
}
