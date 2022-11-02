use crate::battle::*;
use crate::bindable::*;
use crate::resources::*;
use framework::prelude::*;

#[derive(Clone)]
pub struct Living {
    pub hit: bool, // used for flashing white
    pub hitbox_enabled: bool,
    pub counterable: bool,
    pub health: i32,
    pub max_health: i32,
    pub ignore_common_aggressor: bool,
    pub aggressor: Option<EntityID>,
    pub intangibility: Intangibility,
    pub defense_rules: Vec<DefenseRule>,
    pub flinch_anim_state: Option<String>,
    // status director
    // status/hit queue?
    // status callbacks
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

    pub fn process_hit(
        game_io: &GameIO<Globals>,
        simulation: &mut BattleSimulation,
        vms: &[RollbackVM],
        entity_id: EntityID,
        mut hit_props: HitProperties,
    ) {
        let (entity, living) = (simulation.entities)
            .query_one_mut::<(&Entity, &mut Living)>(entity_id.into())
            .unwrap();

        let time_is_frozen = entity.time_is_frozen;
        let tile_pos = (entity.x, entity.y);

        let tile = simulation.field.tile_at_mut(tile_pos).unwrap();
        let defense_rules = living.defense_rules.clone();

        // resolving tile effects
        if tile.state() == TileState::Holy {
            hit_props.damage += 1;
            hit_props.damage /= 2;
        }

        // filter statuses through defense rules
        DefenseJudge::filter_statuses(game_io, simulation, vms, &mut hit_props, &defense_rules);

        if time_is_frozen {
            hit_props.flags |= HitFlag::SHAKE;
            hit_props.flags |= HitFlag::NO_COUNTER;
        }

        let (entity, living) = simulation
            .entities
            .query_one_mut::<(&Entity, &mut Living)>(entity_id.into())
            .unwrap();

        let original_damage = hit_props.damage;

        // super effective bonus
        if hit_props.is_super_effective(entity.element) {
            hit_props.damage += original_damage;
        }

        // tile bonus
        let tile = simulation.field.tile_at_mut(tile_pos).unwrap();

        if tile.apply_bonus_damage(&hit_props) {
            hit_props.damage += original_damage;
        }

        living.set_health(living.health - hit_props.damage);

        if hit_props.flags & HitFlag::IMPACT != 0 {
            // used for flashing white
            living.hit = true
        }

        // todo: pass hit_props to status director
    }
}
