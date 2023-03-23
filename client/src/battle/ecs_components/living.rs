use crate::battle::*;
use crate::bindable::*;
use crate::resources::*;
use framework::prelude::*;
use std::collections::HashMap;

#[derive(Clone)]
pub struct Living {
    pub hit: bool, // used for flashing white
    pub hitbox_enabled: bool,
    pub counterable: bool,
    pub health: i32,
    pub max_health: i32,
    pub ignore_common_aggressor: bool,
    pub aggressor: Option<EntityId>,
    pub intangibility: Intangibility,
    pub defense_rules: Vec<DefenseRule>,
    pub flinch_anim_state: Option<String>,
    pub status_director: StatusDirector,
    pub status_callbacks: HashMap<HitFlags, Vec<BattleCallback>>,
    pub hit_callbacks: Vec<BattleCallback<HitProperties>>,
}

impl Default for Living {
    fn default() -> Self {
        Self {
            hit: false,
            hitbox_enabled: true,
            counterable: false,
            health: 0,
            max_health: 0,
            ignore_common_aggressor: false,
            aggressor: None,
            intangibility: Intangibility::default(),
            defense_rules: Vec::new(),
            flinch_anim_state: None,
            status_director: StatusDirector::default(),
            status_callbacks: HashMap::new(),
            hit_callbacks: Vec::new(),
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
        if let Some(callbacks) = self.status_callbacks.get_mut(&hit_flag) {
            callbacks.push(callback);
        } else {
            self.status_callbacks.insert(hit_flag, vec![callback]);
        }
    }

    pub fn register_hit_callback(&mut self, callback: BattleCallback<HitProperties>) {
        self.hit_callbacks.push(callback);
    }

    pub fn process_hit(
        game_io: &GameIO,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityId,
        mut hit_props: HitProperties,
    ) {
        let entities = &mut simulation.entities;
        let Ok((entity, living)) = entities.query_one_mut::<(&Entity, &mut Living)>(entity_id.into()) else {
            return;
        };

        let time_is_frozen = entity.time_frozen_count > 0;
        let tile_pos = (entity.x, entity.y);

        let defense_rules = living.defense_rules.clone();

        // filter statuses through defense rules
        DefenseJudge::filter_statuses(game_io, simulation, vms, &mut hit_props, &defense_rules);

        if time_is_frozen {
            hit_props.flags |= HitFlag::SHAKE;
            hit_props.flags |= HitFlag::NO_COUNTER;
        }

        let entities = &mut simulation.entities;
        let entity = entities.query_one_mut::<&Entity>(entity_id.into()).unwrap();

        let original_damage = hit_props.damage;

        // super effective bonus
        if hit_props.is_super_effective(entity.element) {
            hit_props.damage += original_damage;
        }

        // tile bonus
        let tile = simulation.field.tile_at_mut(tile_pos).unwrap();
        let tile_state = &simulation.tile_states[tile.state_index()];
        let bonus_damage_callback = tile_state.calculate_bonus_damage_callback.clone();

        hit_props.damage += bonus_damage_callback.call(
            game_io,
            simulation,
            vms,
            (hit_props.clone(), original_damage),
        );

        let entities = &mut simulation.entities;
        let Ok((entity, living)) = entities.query_one_mut::<(&Entity, &mut Living)>(entity_id.into()) else {
            return;
        };

        // apply damage
        living.set_health(living.health - hit_props.damage);

        if hit_props.flags & HitFlag::IMPACT != 0 {
            // used for flashing white
            living.hit = true
        }

        // apply statuses
        living.status_director.apply_hit_flags(hit_props.flags);

        // store callbacks
        let hit_callbacks = living.hit_callbacks.clone();

        // handle drag
        if hit_props.drags() && entity.movement.is_none() {
            let can_move_to_callback = entity.can_move_to_callback.clone();
            let delta: IVec2 = hit_props.drag.direction.i32_vector().into();

            let mut dest = IVec2::new(entity.x, entity.y);
            let mut duration = 0;

            for _ in 0..hit_props.drag.count {
                dest += delta;

                let tile_exists = simulation.field.tile_at_mut(dest.into()).is_some();

                if !tile_exists || !can_move_to_callback.call(game_io, simulation, vms, dest.into())
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

        for callback in hit_callbacks {
            callback.call(game_io, simulation, vms, hit_props.clone());
        }
    }
}
