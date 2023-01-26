use crate::battle::*;
use crate::bindable::*;
use crate::render::*;
use framework::prelude::*;
use generational_arena::Arena;

#[derive(Clone, Copy)]
pub struct FullEntityPosition {
    pub tile_position: IVec2,
    pub offset: Vec2,
    pub tile_offset: Vec2,
}

#[derive(Clone)]
pub struct Entity {
    pub updated: bool,
    pub pending_spawn: bool,
    pub spawned: bool,
    pub on_field: bool,
    pub id: EntityId,
    pub name: String,
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
    pub animator_index: generational_arena::Index,
    pub sprite_tree: Tree<SpriteNode>,
    pub shadow_index: TreeIndex,
    pub offset: Vec2,      // does not flip with teams, only perspective
    pub tile_offset: Vec2, // resets every frame, does not flip with teams, only perspective
    pub hit_context: HitContext,
    pub time_frozen_count: usize,
    pub ignore_hole_tiles: bool,
    pub ignore_tile_effects: bool,
    pub move_action: Option<MoveAction>,
    pub move_anim_state: Option<String>,
    pub last_movement: FrameTime, // stores the simulation.battle_time for the last move_action update, excluding endlag
    pub card_action_index: Option<generational_arena::Index>,
    pub local_components: Vec<generational_arena::Index>,
    pub can_move_to_callback: BattleCallback<(i32, i32), bool>,
    pub update_callback: BattleCallback,
    pub collision_callback: BattleCallback<EntityId>,
    pub attack_callback: BattleCallback<EntityId>,
    pub spawn_callback: BattleCallback,
    pub battle_start_callback: BattleCallback,
    pub battle_end_callback: BattleCallback,
    pub delete_callback: BattleCallback,
    pub delete_callbacks: Vec<BattleCallback>,
}

impl Entity {
    pub fn new(game_io: &GameIO, id: EntityId, animator_index: generational_arena::Index) -> Self {
        let mut sprite_tree = Tree::new(SpriteNode::new(game_io, SpriteColorMode::Add));

        let mut shadow_node = SpriteNode::new(game_io, SpriteColorMode::Add);
        shadow_node.set_visible(false);
        shadow_node.set_layer(i32::MAX);
        let shadow_index = sprite_tree.insert_root_child(shadow_node);

        Self {
            updated: false,
            pending_spawn: false,
            spawned: false,
            on_field: false,
            name: String::new(),
            id,
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
            sprite_tree,
            shadow_index,
            offset: Vec2::ZERO,
            tile_offset: Vec2::ZERO,
            hit_context: HitContext {
                aggressor: id,
                ..Default::default()
            },
            time_frozen_count: 0,
            ignore_hole_tiles: false,
            ignore_tile_effects: false,
            move_action: None,
            move_anim_state: None,
            last_movement: 0,
            card_action_index: None,
            local_components: Vec::new(),
            can_move_to_callback: BattleCallback::stub(false),
            update_callback: BattleCallback::stub(()),
            collision_callback: BattleCallback::stub(()),
            attack_callback: BattleCallback::stub(()),
            spawn_callback: BattleCallback::stub(()),
            battle_start_callback: BattleCallback::stub(()),
            battle_end_callback: BattleCallback::stub(()), // todo:
            delete_callback: BattleCallback::new(move |_, simulation, _, _| {
                // default behavior, just erase
                let entity = (simulation.entities)
                    .query_one_mut::<&mut Entity>(id.into())
                    .unwrap();

                entity.erased = true;
            }),
            delete_callbacks: Vec::new(),
        }
    }

    pub fn full_position(&self) -> FullEntityPosition {
        FullEntityPosition {
            tile_position: IVec2::new(self.x, self.y),
            offset: self.offset,
            tile_offset: self.tile_offset,
        }
    }

    pub fn copy_full_position(&mut self, full_position: FullEntityPosition) {
        self.x = full_position.tile_position.x;
        self.y = full_position.tile_position.y;
        self.offset = full_position.offset;
        self.tile_offset = full_position.tile_offset;
    }

    pub fn set_shadow(&mut self, game_io: &GameIO, path: String) {
        let sprite_node = &mut self.sprite_tree[self.shadow_index];
        sprite_node.set_texture(game_io, path);

        let shadow_size = sprite_node.size();
        sprite_node.set_origin(shadow_size * 0.5);
    }

    pub fn current_can_move_to_callback(
        &self,
        card_actions: &Arena<CardAction>,
    ) -> BattleCallback<(i32, i32), bool> {
        if let Some(index) = self.card_action_index {
            // does the card action have a special can_move_to_callback?
            if let Some(callback) = card_actions[index].can_move_to_callback.clone() {
                return callback;
            }
        }

        self.can_move_to_callback.clone()
    }

    pub fn sort_key(&self) -> (i32, i32, i32) {
        (self.y, self.x, -self.sprite_tree.root().layer())
    }

    pub fn flipped(&self) -> bool {
        self.facing == Direction::Left
    }

    pub fn corrected_offset(&self, perspective_flipped: bool) -> Vec2 {
        let mut entity_offset = self.tile_offset + self.offset;

        if perspective_flipped {
            entity_offset.x *= -1.0;
        }

        entity_offset
    }
}
