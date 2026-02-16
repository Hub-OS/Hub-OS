use crate::battle::*;
use crate::bindable::*;
use crate::render::*;
use crate::structures::SlotMap;
use framework::prelude::*;

#[derive(Clone, Copy)]
pub struct FullEntityPosition {
    pub tile_position: IVec2,
    pub offset: Vec2,
    pub movement_offset: Vec2,
}

#[derive(Clone)]
pub struct Entity {
    pub updated: bool,
    pub pending_spawn: bool,
    pub spawned: bool,
    pub on_field: bool,
    pub team: Team,
    pub element: Element,
    pub deleted: bool,
    pub erased: bool,
    pub facing: Direction,
    pub share_tile: bool,
    pub auto_reserves_tiles: bool, // should be set and never change after spawning
    pub x: i32,
    pub y: i32,
    pub height: f32,
    pub elevation: f32,
    pub animator_index: GenerationalIndex,
    pub sprite_tree_index: GenerationalIndex,
    pub offset: Vec2,          // does not flip with teams, only perspective
    pub movement_offset: Vec2, // resets every frame, does not flip with teams, only perspective
    pub time_frozen: bool,
    pub ignore_hole_tiles: bool,
    pub ignore_negative_tile_effects: bool,
}

impl Entity {
    pub fn create(game_io: &GameIO, simulation: &mut BattleSimulation) -> EntityId {
        let id: EntityId = Self::reserve(simulation);

        // create sprite tree
        let sprite_tree = Tree::new(SpriteNode::new(game_io, SpriteColorMode::Add));
        let sprite_tree_index = simulation.sprite_trees.insert(sprite_tree);

        // create animator
        let mut animator = BattleAnimator::new();
        animator.set_target(sprite_tree_index, GenerationalIndex::tree_root());
        animator.disable();

        let animator_index = simulation.animators.insert(animator);

        let entity = Self {
            updated: false,
            pending_spawn: false,
            spawned: false,
            on_field: false,
            team: Team::Unset,
            element: Element::None,
            deleted: false,
            erased: false,
            facing: Direction::None,
            share_tile: true,
            auto_reserves_tiles: false,
            x: 0,
            y: 0,
            height: 0.0,
            elevation: 0.0,
            animator_index,
            sprite_tree_index,
            offset: Vec2::ZERO,
            movement_offset: Vec2::ZERO,
            time_frozen: false,
            ignore_hole_tiles: false,
            ignore_negative_tile_effects: false,
        };

        let delete_callback = BattleCallback::new(move |game_io, resources, simulation, _| {
            // default behavior, just erase
            Entity::mark_erased(game_io, resources, simulation, id);
        });

        let components = (entity, DeleteCallback(delete_callback));
        simulation.entities.spawn_at(id.into(), components);

        id
    }

    fn reserve(simulation: &mut BattleSimulation) -> EntityId {
        let id = simulation.entities.reserve_entity();

        let index = id.id() as usize;
        let generation = (id.to_bits().get() >> 32) as u32;

        if index < simulation.generation_tracking.len() {
            simulation.generation_tracking[index] = generation;
        } else {
            simulation.generation_tracking.push(generation);
        }

        id.into()
    }

    pub fn can_move_to(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        position: (i32, i32),
    ) -> bool {
        let entities = &mut simulation.entities;
        let Ok((can_move_to_callback, action_queue)) =
            entities.query_one_mut::<(&CanMoveToCallback, Option<&ActionQueue>)>(entity_id.into())
        else {
            return false;
        };

        let entity_callback = can_move_to_callback.0.clone();
        let Some(index) = action_queue.and_then(|q| q.active) else {
            return entity_callback.call(game_io, resources, simulation, position);
        };

        let action = &simulation.actions[index];

        if !action.executed {
            return entity_callback.call(game_io, resources, simulation, position);
        }

        let Some(action_callback) = action.can_move_to_callback.clone() else {
            return entity_callback.call(game_io, resources, simulation, position);
        };

        action_callback.call(game_io, resources, simulation, position)
    }

    pub fn full_position(&self) -> FullEntityPosition {
        FullEntityPosition {
            tile_position: IVec2::new(self.x, self.y),
            offset: self.offset,
            movement_offset: self.movement_offset,
        }
    }

    pub fn copy_full_position(&mut self, full_position: FullEntityPosition) {
        self.x = full_position.tile_position.x;
        self.y = full_position.tile_position.y;
        self.offset = full_position.offset;
        self.movement_offset = full_position.movement_offset;
    }

    pub fn sort_key(&self, sprite_trees: &SlotMap<Tree<SpriteNode>>) -> (i32, i32, i32) {
        let Some(sprite_tree) = sprite_trees.get(self.sprite_tree_index) else {
            return (self.y, 0, self.movement_offset.y as _);
        };

        (
            self.y,
            -sprite_tree.root().layer(),
            self.movement_offset.y as _,
        )
    }

    pub fn flipped(&self) -> bool {
        self.facing == Direction::Left
    }

    pub fn corrected_offset(&self, perspective_flipped: bool) -> Vec2 {
        let mut entity_offset = self.movement_offset + self.offset;

        if perspective_flipped {
            entity_offset.x *= -1.0;
        }

        entity_offset
    }

    pub fn screen_position(&self, field: &Field, perspective_flipped: bool) -> Vec2 {
        let offset = self.corrected_offset(perspective_flipped);
        let tile_center = field.calc_tile_center((self.x, self.y), perspective_flipped);
        tile_center + offset
    }

    pub fn delete(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        id: EntityId,
    ) {
        let Ok((entity, action_queue, delete_callback, delete_callbacks)) =
            simulation.entities.query_one_mut::<(
                &mut Entity,
                Option<&mut ActionQueue>,
                Option<&DeleteCallback>,
                Option<&mut DeleteCallbacks>,
            )>(id.into())
        else {
            return;
        };

        if entity.deleted {
            return;
        }

        simulation.ownership_tracking.untrack(id);

        // gather action indices for deletion
        let mut action_indices: Vec<_> = (simulation.actions)
            .iter()
            .filter(|(_, action)| action.entity == id && action.processed)
            .map(|(index, _)| index)
            .collect();

        if let Some(action_queue) = action_queue {
            action_indices.extend(action_queue.pending.drain(..));
        }

        // mark deleted
        entity.deleted = true;

        // gather delete callbacks
        let listener_callbacks = delete_callbacks
            .map(|c| std::mem::take(&mut c.0))
            .unwrap_or_default();
        let delete_callback = delete_callback.cloned();

        // delete player augments
        if let Ok(player) = simulation.entities.query_one_mut::<&mut Player>(id.into()) {
            let augment_iter = player.augments.values();
            let augment_callbacks =
                augment_iter.flat_map(|augment| augment.delete_callback.clone());

            simulation.pending_callbacks.extend(augment_callbacks);
            simulation.call_pending_callbacks(game_io, resources);
        }

        // delete card actions
        Action::delete_multi(game_io, resources, simulation, true, action_indices);

        // call delete callbacks after
        simulation.pending_callbacks.extend(listener_callbacks);

        if let Some(callback) = delete_callback {
            simulation.pending_callbacks.push(callback.0);
        }

        simulation.call_pending_callbacks(game_io, resources);

        // clear reservations
        simulation.field.drop_entity(id);
    }

    pub fn mark_erased(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        id: EntityId,
    ) {
        // clear the delete callback
        let _ = simulation.entities.remove_one::<DeleteCallback>(id.into());

        let Ok(entity) = simulation.entities.query_one_mut::<&mut Entity>(id.into()) else {
            return;
        };

        if entity.erased {
            return;
        }

        // mark as erased
        entity.erased = true;

        simulation.ownership_tracking.untrack(id);

        // delete
        Entity::delete(game_io, resources, simulation, id);
    }
}
