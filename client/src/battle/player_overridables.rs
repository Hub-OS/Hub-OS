use super::{
    BattleAnimator, BattleCallback, BattleSimulation, CardSelectButton, Entity, Player,
    SharedBattleResources,
};
use crate::bindable::{CardProperties, EntityId, GenerationalIndex};
use crate::render::{FrameTime, SpriteNode};
use crate::structures::{SlotMap, Tree};
use framework::prelude::GameIO;
use packets::structures::Direction;

// compressing up to 4 optional flags in one byte
#[derive(Default, Clone)]
pub struct PlayerOverridableFlags(u8);

impl PlayerOverridableFlags {
    fn set_bits(&mut self, index: u8, value: Option<bool>) {
        let offset = index * 2;

        self.0 &= !(0b11 << offset);

        let encoded_value = if let Some(value) = value {
            (1 << 1) + value as u8
        } else {
            0
        };

        self.0 |= encoded_value << offset;
    }

    fn get_value(&self, index: u8) -> Option<bool> {
        let offset = index * 2;
        let value = self.0 >> offset;

        if value & 0b10 == 0 {
            return None;
        }

        Some((value & 1) == 1)
    }

    pub fn movement_on_input(&self) -> Option<bool> {
        self.get_value(0)
    }

    pub fn set_movement_on_input(&mut self, value: Option<bool>) {
        self.set_bits(0, value);
    }

    pub fn charges_with_shoot(&self) -> Option<bool> {
        self.get_value(1)
    }

    pub fn set_charges_with_shoot(&mut self, value: Option<bool>) {
        self.set_bits(1, value);
    }

    pub fn special_on_input(&self) -> Option<bool> {
        self.get_value(2)
    }

    pub fn set_special_on_input(&mut self, value: Option<bool>) {
        self.set_bits(2, value);
    }
}

#[derive(Default, Clone)]
pub struct PlayerOverridables {
    pub buttons: Vec<Option<CardSelectButton>>,
    pub calculate_charge_time: Option<BattleCallback<u8, FrameTime>>,
    pub flags: PlayerOverridableFlags,
    pub normal_attack: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub charged_attack: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub special_attack: Option<BattleCallback<(), Option<GenerationalIndex>>>,
    pub calculate_card_charge_time: Option<BattleCallback<CardProperties, Option<FrameTime>>>,
    pub charged_card: Option<BattleCallback<CardProperties, Option<GenerationalIndex>>>,
    pub movement: Option<BattleCallback<Direction>>,
}

impl PlayerOverridables {
    pub fn default_calculate_charge_time(level: u8) -> FrameTime {
        match level {
            0 | 1 => 100,
            2 => 90,
            3 => 80,
            4 => 70,
            _ => 60,
        }
    }

    pub fn default_movement(
        game_io: &GameIO,
        resources: &SharedBattleResources,
        simulation: &mut BattleSimulation,
        entity_id: EntityId,
        direction: Direction,
    ) {
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

        if Entity::can_move_to(game_io, resources, simulation, entity_id, dest) {
            let scripts = &resources.vm_manager.scripts;
            scripts.queue_default_player_movement.call(
                game_io,
                resources,
                simulation,
                (entity_id, dest),
            );
        }
    }

    pub fn flat_map_mut_for<'a, T: 'a>(
        player: &'a mut Player,
        map: impl Fn(&'a mut Self) -> Option<T> + Clone + 'static,
    ) -> impl Iterator<Item = T> + 'a {
        ReborrowChainMut::new(
            player,
            [
                // priority augments
                |player, i| {
                    player
                        .augments
                        .values_mut()
                        .filter(|augment| augment.priority)
                        .map(|augment| &mut augment.overridables)
                        .nth(i)
                },
                // form
                |player, i| {
                    if i > 0 {
                        return None;
                    }

                    player
                        .active_form
                        .and_then(|index| player.forms.get_mut(index))
                        .map(|form| &mut form.overridables)
                },
                // augments
                |player, i| {
                    player
                        .augments
                        .values_mut()
                        .filter(|augment| !augment.priority)
                        .map(|augment| &mut augment.overridables)
                        .nth(i)
                },
                // base
                |player, i| {
                    if i == 0 {
                        Some(&mut player.overridables)
                    } else {
                        None
                    }
                },
            ],
        )
        .into_iter()
        .flat_map(map)
    }

    pub fn flat_map_for<'a, T: 'a>(
        player: &'a Player,
        map: impl Fn(&'a Self) -> Option<T> + Clone + 'static,
    ) -> impl Iterator<Item = T> + 'a {
        ReborrowChain::new(
            player,
            [
                // priority augments
                |player, i| {
                    player
                        .augments
                        .values()
                        .filter(|augment| augment.priority)
                        .map(|augment| &augment.overridables)
                        .nth(i)
                },
                // form
                |player, i| {
                    if i > 0 {
                        return None;
                    }

                    player
                        .active_form
                        .and_then(|index| player.forms.get(index))
                        .map(|form| &form.overridables)
                },
                // augments
                |player, i| {
                    player
                        .augments
                        .values()
                        .filter(|augment| !augment.priority)
                        .map(|augment| &augment.overridables)
                        .nth(i)
                },
                // base
                |player, i| {
                    if i == 0 {
                        Some(&player.overridables)
                    } else {
                        None
                    }
                },
            ],
        )
        .into_iter()
        .flat_map(map)
    }

    pub fn card_button_slots_for(player: &Player) -> Option<&[Option<CardSelectButton>]> {
        PlayerOverridables::flat_map_for(player, |overridables| {
            if overridables.buttons.len() <= 1 {
                return None;
            }

            let buttons = &overridables.buttons[1..];

            if buttons.iter().all(Option::is_none) {
                return None;
            }

            Some(buttons)
        })
        .next()
    }

    pub fn card_button_slots_mut_for(
        player: &mut Player,
    ) -> Option<&mut [Option<CardSelectButton>]> {
        PlayerOverridables::flat_map_mut_for(player, move |overridables| {
            if overridables.buttons.len() <= 1 {
                return None;
            }

            let buttons = &mut overridables.buttons[1..];

            if buttons.iter().all(Option::is_none) {
                return None;
            }

            Some(buttons)
        })
        .next()
    }

    pub fn special_button_for(player: &Player) -> Option<&CardSelectButton> {
        PlayerOverridables::flat_map_for(player, |overridables| {
            overridables
                .buttons
                .get(CardSelectButton::SPECIAL_SLOT)?
                .as_ref()
        })
        .next()
    }

    pub fn special_button_mut_for(player: &mut Player) -> Option<&mut CardSelectButton> {
        PlayerOverridables::flat_map_mut_for(player, |overridables| {
            overridables
                .buttons
                .get_mut(CardSelectButton::SPECIAL_SLOT)?
                .as_mut()
        })
        .next()
    }

    pub fn delete_self(
        self,
        sprite_trees: &mut SlotMap<Tree<SpriteNode>>,
        animators: &mut SlotMap<BattleAnimator>,
    ) {
        for button in self.buttons.into_iter().flatten() {
            button.delete_self(sprite_trees, animators);
        }
    }
}

macro_rules! create_reborrow_chain {
    ($name:ident, $ptr_mutability:tt $(, $mutability:tt)?) => {
        struct $name<T, State, const LEN: usize> {
            i: usize,
            j: usize,
            reborrowers: [fn(&mut State, usize) -> Option<&$($mutability)* T>; LEN],
            state: State,
            marker: std::marker::PhantomData<T>,
        }

        impl<T, State, const LEN: usize> $name<T, State, LEN> {
            fn new(
                state: State,
                reborrowers: [fn(&mut State, usize) -> Option<&$($mutability)* T>; LEN],
            ) -> Self {
                Self {
                    i: 0,
                    j: 0,
                    state,
                    reborrowers,
                    marker: Default::default(),
                }
            }

            fn fetch_current<'a>(
                state: &'a mut State,
                reborrowers: &[fn(&mut State, usize) -> Option<&$($mutability)* T>],
                i: usize,
                j: usize
            ) -> Option<&'a $($mutability)* T> {
                let reborrow = reborrowers.get(i)?;
                reborrow(state, j)
            }

            fn into_iter<'a>(mut self) -> impl Iterator<Item = &'a $($mutability)* T> + 'a
            where
                State: 'a,
                T: 'a
            {
                std::iter::from_fn(move || {
                    while Self::fetch_current(&mut self.state, &self.reborrowers, self.i, self.j).is_none() {
                        if self.i >= self.reborrowers.len() {
                            return None;
                        }

                        self.i += 1;
                        self.j = 0;
                    }

                    let current = Self::fetch_current(&mut self.state, &self.reborrowers, self.i, self.j)?;
                    self.j += 1;

                    let ptr = current as *$ptr_mutability T;

                    // should be as safe as normal mutable iterators
                    // waiting for official lending iterators
                    Some(unsafe { &$($mutability)* *ptr })
                })
            }
        }
    };
}

create_reborrow_chain!(ReborrowChainMut, mut, mut);
create_reborrow_chain!(ReborrowChain, const);
