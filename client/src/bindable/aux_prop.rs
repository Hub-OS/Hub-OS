use super::{
    CardClass, CardProperties, Comparison, Element, EntityId, GenerationalIndex, HitFlags,
    HitProperties, MathExpr,
};
use crate::battle::{
    ActionQueue, ActionType, ActionTypes, AttackContext, BattleCallback, Character, EmotionWindow,
    Entity, Player, SharedBattleResources,
};
use crate::lua_api::{VM_INDEX_REGISTRY_KEY, create_action_table};
use crate::render::FrameTime;
use packets::structures::Emotion;
use std::ops::{Range, RangeInclusive};

#[derive(Clone, Debug)]
pub enum AuxVariable {
    MaxHealth,
    Health,
    Damage,
}

impl AuxVariable {
    pub fn create_resolver(
        health: i32,
        max_health: i32,
        damage: i32,
    ) -> impl Fn(&AuxVariable) -> f32 + Copy {
        move |var| match var {
            AuxVariable::MaxHealth => max_health as f32,
            AuxVariable::Health => health as f32,
            AuxVariable::Damage => damage as f32,
        }
    }
}

#[derive(Debug, PartialEq, Eq)]
pub struct ParseAuxVariableError;

impl std::str::FromStr for AuxVariable {
    type Err = ParseAuxVariableError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "MAX_HEALTH" => Ok(AuxVariable::MaxHealth),
            "HEALTH" => Ok(AuxVariable::Health),
            "DAMAGE" => Ok(AuxVariable::Damage),
            _ => Err(ParseAuxVariableError),
        }
    }
}

impl<'lua> rollback_mlua::FromLua<'lua> for AuxVariable {
    fn from_lua(
        lua_value: rollback_mlua::Value<'lua>,
        lua: &'lua rollback_mlua::Lua,
    ) -> rollback_mlua::Result<Self> {
        let name: rollback_mlua::String = lua.unpack(lua_value.clone())?;
        let name_str = name.to_str()?;

        match name_str {
            "MAX_HEALTH" => Ok(AuxVariable::MaxHealth),
            "HEALTH" => Ok(AuxVariable::Health),
            "DAMAGE" => Ok(AuxVariable::Damage),
            _ => Err(rollback_mlua::Error::RuntimeError(format!(
                "Invalid AuxProp variable: {name_str:?}"
            ))),
        }
    }
}

#[derive(Clone)]
pub enum AuxRequirement {
    Interval(FrameTime),
    HitElement(Element),
    HitElementIsWeakness,
    HitFlags(HitFlags),
    HitFlagsAbsent(HitFlags),
    HitDamage(Comparison, i32),
    ProjectedHitDamage(MathExpr<f32, AuxVariable>, Comparison, i32),
    TotalDamage(Comparison, i32),
    Element(Element),
    Emotion(Emotion),
    NegativeTileInteraction,
    ContextStart,
    Action(ActionTypes),
    ChargeTime(Comparison, FrameTime),
    CardChargeTime(Comparison, FrameTime),
    ChargedCard,
    CardPrimaryElement(Element),
    CardNotPrimaryElement(Element),
    CardElement(Element),
    CardNotElement(Element),
    CardDamage(Comparison, i32),
    CardRecover(Comparison, i32),
    CardHitFlags(HitFlags),
    CardHitFlagsAbsent(HitFlags),
    CardCode(String),
    CardClass(CardClass),
    CardNotClass(CardClass),
    CardTimeFreeze(bool),
    CardTag(String),
    CardNotTag(String),
    ProjectedHPThreshold(MathExpr<f32, AuxVariable>, Comparison, f32),
    ProjectedHP(MathExpr<f32, AuxVariable>, Comparison, i32),
    HPThreshold(Comparison, f32),
    HP(Comparison, i32),
}

impl AuxRequirement {
    // always activates when applicable, granted that no additional conditions are required or provided.
    const UNCONDITIONAL_PRIORITY: usize = 0;
    // apply every X real-time frames, otherwise treated as unconditional.
    const TIMER_PRIORITY: usize = 1;
    // activates when matching specific properties from incoming hitboxes.
    const HIT_PRIORITY: usize = 2;
    // properties of self
    const BODY_PRIORITY: usize = 3;
    // activates when current HP matches a specified formula that is allowed to be more open ended. Could allow for cases such as "HP is above 50% of max", "HP has a 4 in it", "HP is under 20% of max", "HP is less than 10". Cannot check the amount of incoming damage.
    const HP_EXPR_PRIOIRTY: usize = 4;
    // just like HP EXPR but do not activate if the specified threshold is reached
    const HP_GE_PRIORITY: usize = 5;
    // activates if HP would dip below a specified percentage threshold if the currently evaluated damage were to be applied. 0% HP is a valid threshold here, meaning HP would reach zero this turn. These checks will still be run even if no damage is being applied this frame. Can also compare with arbitrarily provided numbers, ie "would my HP reach 0% if I reduced it by 1"
    const HP_LE_PRIORITY: usize = 6;

    fn priority(&self) -> usize {
        match self {
            AuxRequirement::Interval(_) => Self::TIMER_PRIORITY, // TIMER
            AuxRequirement::HitElement(_)
            | AuxRequirement::HitElementIsWeakness
            | AuxRequirement::HitFlags(_)
            | AuxRequirement::HitFlagsAbsent(_)
            | AuxRequirement::HitDamage(_, _)
            | AuxRequirement::ProjectedHitDamage(_, _, _)
            | AuxRequirement::TotalDamage(_, _) => Self::HIT_PRIORITY, // HIT
            AuxRequirement::Element(_)
            | AuxRequirement::Emotion(_)
            | AuxRequirement::NegativeTileInteraction
            | AuxRequirement::ContextStart
            | AuxRequirement::Action(_)
            | AuxRequirement::ChargeTime(..)
            | AuxRequirement::CardChargeTime(..)
            | AuxRequirement::ChargedCard
            | AuxRequirement::CardPrimaryElement(_)
            | AuxRequirement::CardNotPrimaryElement(_)
            | AuxRequirement::CardElement(_)
            | AuxRequirement::CardNotElement(_)
            | AuxRequirement::CardDamage(_, _)
            | AuxRequirement::CardRecover(_, _)
            | AuxRequirement::CardHitFlags(_)
            | AuxRequirement::CardHitFlagsAbsent(_)
            | AuxRequirement::CardCode(_)
            | AuxRequirement::CardClass(_)
            | AuxRequirement::CardNotClass(_)
            | AuxRequirement::CardTimeFreeze(_)
            | AuxRequirement::CardTag(_)
            | AuxRequirement::CardNotTag(_) => Self::BODY_PRIORITY, // BODY
            AuxRequirement::ProjectedHPThreshold(_, _, _)
            | AuxRequirement::ProjectedHP(_, _, _) => Self::HP_EXPR_PRIOIRTY, // HP EXPR
            AuxRequirement::HPThreshold(cmp, _) | AuxRequirement::HP(cmp, _) => match cmp {
                Comparison::GT | Comparison::GE => Self::HP_GE_PRIORITY, // HP GE
                _ => Self::HP_LE_PRIORITY,                               // HP LE
            },
        }
    }

    fn from_lua<'lua>(
        resources: &SharedBattleResources,
        lua: &'lua rollback_mlua::Lua,
        lua_value: rollback_mlua::Value<'lua>,
    ) -> rollback_mlua::Result<Self> {
        let table: rollback_mlua::Table = lua.unpack(lua_value.clone())?;

        let name: rollback_mlua::String = table.get(1)?;
        let name_str = name.to_str()?;

        let requirement = match name_str {
            "require_interval" => AuxRequirement::Interval(table.get(2)?),
            "require_hit_element" => AuxRequirement::HitElement(table.get(2)?),
            "require_hit_element_is_weakness" => AuxRequirement::HitElementIsWeakness,
            "require_hit_flags" => AuxRequirement::HitFlags(table.get(2)?),
            "require_hit_flags_absent" => AuxRequirement::HitFlagsAbsent(table.get(2)?),
            "require_hit_damage" => AuxRequirement::HitDamage(table.get(2)?, table.get(3)?),
            "require_projected_hit_damage" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxRequirement::ProjectedHitDamage(expr, table.get(3)?, table.get(4)?)
            }
            "require_total_damage" => AuxRequirement::TotalDamage(table.get(2)?, table.get(3)?),
            "require_element" => AuxRequirement::Element(table.get(2)?),
            "require_emotion" => AuxRequirement::Emotion(table.get(2)?),
            "require_negative_tile_interaction" => AuxRequirement::NegativeTileInteraction,
            "require_action" => AuxRequirement::Action(
                table
                    .get::<_, Option<ActionTypes>>(2)?
                    .unwrap_or(ActionType::ALL),
            ),
            "require_context_start" => AuxRequirement::ContextStart,
            "require_charge_time" => AuxRequirement::ChargeTime(table.get(2)?, table.get(3)?),
            "require_card_charge_time" => {
                AuxRequirement::CardChargeTime(table.get(2)?, table.get(3)?)
            }
            "require_charged_card" => AuxRequirement::ChargedCard,
            "require_card_primary_element" => AuxRequirement::CardPrimaryElement(table.get(2)?),
            "require_card_not_primary_element" => {
                AuxRequirement::CardNotPrimaryElement(table.get(2)?)
            }
            "require_card_element" => AuxRequirement::CardElement(table.get(2)?),
            "require_card_not_element" => AuxRequirement::CardNotElement(table.get(2)?),
            "require_card_damage" => AuxRequirement::CardDamage(table.get(2)?, table.get(3)?),
            "require_card_recover" => AuxRequirement::CardRecover(table.get(2)?, table.get(3)?),
            "require_card_hit_flags" => AuxRequirement::CardHitFlags(table.get(2)?),
            "require_card_hit_flags_absent" => AuxRequirement::CardHitFlagsAbsent(table.get(2)?),
            "require_card_code" => AuxRequirement::CardCode(table.get(2)?),
            "require_card_class" => AuxRequirement::CardClass(table.get(2)?),
            "require_card_not_class" => AuxRequirement::CardNotClass(table.get(2)?),
            "require_card_time_freeze" => AuxRequirement::CardTimeFreeze(table.get(2)?),
            "require_card_tag" => AuxRequirement::CardTag(table.get(2)?),
            "require_card_not_tag" => AuxRequirement::CardNotTag(table.get(2)?),
            "require_projected_health_threshold" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxRequirement::ProjectedHPThreshold(expr, table.get(3)?, table.get(4)?)
            }
            "require_projected_health" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxRequirement::ProjectedHP(expr, table.get(3)?, table.get(4)?)
            }
            "require_health_threshold" => AuxRequirement::HPThreshold(table.get(2)?, table.get(3)?),
            "require_health" => AuxRequirement::HP(table.get(2)?, table.get(3)?),
            _ => {
                return Err(rollback_mlua::Error::FromLuaConversionError {
                    from: lua_value.type_name(),
                    to: "AuxRequirement",
                    message: None,
                });
            }
        };

        Ok(requirement)
    }
}

#[derive(Default, Clone)]
pub enum AuxEffect {
    UpdateContext(BattleCallback<EntityId>),
    IncreaseCardDamage(i32),
    IncreaseCardMultiplier(f32),
    InterceptAction(BattleCallback<GenerationalIndex, Option<GenerationalIndex>>),
    InterruptAction(BattleCallback<GenerationalIndex>),
    IncreasePreHitDamage(MathExpr<f32, AuxVariable>),
    DecreasePreHitDamage(MathExpr<f32, AuxVariable>),
    StatusImmunity(HitFlags),
    ApplyStatus(HitFlags, FrameTime),
    RemoveStatus(HitFlags),
    IncreaseHitDamage(MathExpr<f32, AuxVariable>),
    DecreaseHitDamage(MathExpr<f32, AuxVariable>),
    DecreaseDamageSum(MathExpr<f32, AuxVariable>),
    DrainHP(i32),
    RecoverHP(i32),
    #[default]
    None,
}

impl AuxEffect {
    const PRE_HIT_START: usize = 5; // IncreasePreHitDamage
    const HIT_PREP_START: usize = 6; // StatusImmunity
    const ON_HIT_START: usize = 10; // IncreaseHitDamage
    const POST_HIT_START: usize = 12; // DecreaseDamageSum
    const POST_HIT_END: usize = 15; // None

    const fn priority(&self) -> usize {
        match self {
            AuxEffect::UpdateContext(..) => 0,
            AuxEffect::IncreaseCardDamage(_) => 1,
            AuxEffect::IncreaseCardMultiplier(_) => 2,
            AuxEffect::InterceptAction(_) => 3,
            AuxEffect::InterruptAction(_) => 4,
            AuxEffect::IncreasePreHitDamage(_) => 5,
            AuxEffect::DecreasePreHitDamage(_) => 5,
            AuxEffect::StatusImmunity(_) => 7,
            AuxEffect::ApplyStatus(_, _) => 8,
            AuxEffect::RemoveStatus(_) => 9,
            AuxEffect::IncreaseHitDamage(_) => 10,
            AuxEffect::DecreaseHitDamage(_) => 11,
            AuxEffect::DecreaseDamageSum(_) => 12,
            AuxEffect::DrainHP(_) => 13,
            AuxEffect::RecoverHP(_) => 14,
            AuxEffect::None => 15,
        }
    }

    pub fn executes_on_card_use(&self) -> bool {
        matches!(
            self,
            AuxEffect::IncreaseCardMultiplier(_) | AuxEffect::IncreaseCardDamage(_)
        )
    }

    pub fn resolves_action(&self) -> bool {
        matches!(self, AuxEffect::InterceptAction(_))
    }

    pub fn resolves_action_context(&self) -> bool {
        matches!(self, AuxEffect::UpdateContext(..))
    }

    pub fn executes_on_current_action(&self) -> bool {
        matches!(self, AuxEffect::InterruptAction(_))
    }

    pub fn executes_pre_hit(&self) -> bool {
        const PRE_HIT_RANGE: Range<usize> = AuxEffect::PRE_HIT_START..AuxEffect::HIT_PREP_START;

        PRE_HIT_RANGE.contains(&self.priority())
    }

    pub fn executes_hit_prep(&self) -> bool {
        const HIT_PREP_RANGE: Range<usize> = AuxEffect::HIT_PREP_START..AuxEffect::ON_HIT_START;

        HIT_PREP_RANGE.contains(&self.priority())
    }

    pub fn executes_on_hit(&self) -> bool {
        const ON_HIT_RANGE: Range<usize> = AuxEffect::ON_HIT_START..AuxEffect::POST_HIT_START;

        ON_HIT_RANGE.contains(&self.priority())
    }

    pub fn executes_after_hit(&self) -> bool {
        const POST_HIT_RANGE: RangeInclusive<usize> =
            AuxEffect::POST_HIT_START..=AuxEffect::POST_HIT_END;

        POST_HIT_RANGE.contains(&self.priority())
    }

    pub fn hit_related(&self) -> bool {
        const HIT_RANGE: RangeInclusive<usize> = AuxEffect::PRE_HIT_START..=AuxEffect::POST_HIT_END;

        HIT_RANGE.contains(&self.priority())
    }

    fn from_lua<'lua>(
        resources: &SharedBattleResources,
        lua: &'lua rollback_mlua::Lua,
        lua_value: rollback_mlua::Value<'lua>,
    ) -> rollback_mlua::Result<Self> {
        if matches!(lua_value, rollback_mlua::Nil) {
            return Ok(AuxEffect::None);
        }

        let table: rollback_mlua::Table = lua.unpack(lua_value.clone())?;

        let name: rollback_mlua::String = table.get(1)?;
        let name_str = name.to_str()?;

        let effect = match name_str {
            "declare_immunity" => AuxEffect::StatusImmunity(table.get(2)?),
            "apply_status" => AuxEffect::ApplyStatus(table.get(2)?, table.get(3)?),
            "remove_status" => AuxEffect::RemoveStatus(table.get(2)?),
            "increase_hit_damage" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxEffect::IncreaseHitDamage(expr)
            }
            "decrease_hit_damage" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxEffect::DecreaseHitDamage(expr)
            }
            "decrease_total_damage" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxEffect::DecreaseDamageSum(expr)
            }
            "increase_card_damage" => AuxEffect::IncreaseCardDamage(table.get(2)?),
            "increase_card_multiplier" => AuxEffect::IncreaseCardMultiplier(table.get(2)?),
            "intercept_action" => {
                let callback = BattleCallback::new_transformed_lua_callback(
                    lua,
                    lua.named_registry_value(VM_INDEX_REGISTRY_KEY)?,
                    table.get(2)?,
                    |_, lua, index| lua.pack_multi(create_action_table(lua, index)?),
                )?;
                AuxEffect::InterceptAction(callback)
            }
            "interrupt_action" => {
                let callback = BattleCallback::new_transformed_lua_callback(
                    lua,
                    lua.named_registry_value(VM_INDEX_REGISTRY_KEY)?,
                    table.get(2)?,
                    |_, lua, index| lua.pack_multi(create_action_table(lua, index)?),
                )?;
                AuxEffect::InterruptAction(callback)
            }
            "increase_pre_hit_damage" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxEffect::IncreasePreHitDamage(expr)
            }
            "decrease_pre_hit_damage" => {
                let expr = resources.parse_math_expr(table.get(2)?)?;
                AuxEffect::DecreasePreHitDamage(expr)
            }
            "drain_health" => AuxEffect::DrainHP(table.get(2)?),
            "recover_health" => AuxEffect::RecoverHP(table.get(2)?),
            "update_context" => {
                let vm_index = lua.named_registry_value(VM_INDEX_REGISTRY_KEY)?;
                let callback =
                    BattleCallback::<EntityId, AttackContext>::new_transformed_lua_callback(
                        lua,
                        vm_index,
                        table.get(2)?,
                        |api_ctx, lua, entity_id| {
                            let mut api_ctx = api_ctx.borrow_mut();
                            let entities = &mut api_ctx.simulation.entities;
                            let Ok(attack_context) =
                                entities.query_one_mut::<&AttackContext>(entity_id.into())
                            else {
                                return lua.pack_multi(&AttackContext::new(entity_id));
                            };

                            lua.pack_multi(attack_context)
                        },
                    )?;

                let callback = BattleCallback::new(
                    move |game_io, resources, simulation, entity_id: EntityId| {
                        let context = callback.call(game_io, resources, simulation, entity_id);
                        let _ = simulation.entities.insert_one(entity_id.into(), context);
                    },
                );

                AuxEffect::UpdateContext(callback)
            }
            _ => {
                return Err(rollback_mlua::Error::RuntimeError(String::from(
                    "invalid or missing AuxProp effect",
                )));
            }
        };

        Ok(effect)
    }
}

#[derive(Default, Clone)]
struct RequirementState {
    passed: bool,
    tested: bool,
}

#[derive(Clone)]
pub struct AuxProp {
    requirements: Vec<(AuxRequirement, RequirementState)>,
    effect: AuxEffect,
    callbacks: Vec<BattleCallback>,
    deletes_on_activation: bool,
    deletes_next_run: bool,
    tested: bool,
    activated: bool,
}

impl AuxProp {
    pub fn new() -> Self {
        Self {
            requirements: Vec::new(),
            effect: AuxEffect::None,
            callbacks: Vec::new(),
            deletes_on_activation: false,
            deletes_next_run: false,
            tested: false,
            activated: false,
        }
    }

    pub fn new_element_bonus(element: Element) -> Self {
        Self::new()
            .with_requirement(AuxRequirement::HitElement(element))
            .with_effect(AuxEffect::IncreaseHitDamage(
                MathExpr::from_variable(AuxVariable::Damage).mul(2.0),
            ))
    }

    pub fn with_effect(mut self, effect: AuxEffect) -> Self {
        assert!(matches!(self.effect, AuxEffect::None));
        self.effect = effect;
        self
    }

    pub fn with_callback(mut self, callback: BattleCallback) -> Self {
        self.callbacks.push(callback);
        self
    }

    pub fn with_requirement(mut self, requirement: AuxRequirement) -> Self {
        self.requirements
            .push((requirement, RequirementState::default()));
        self
    }

    pub fn delete_next_run(mut self) -> Self {
        self.deletes_next_run = true;
        self
    }

    pub fn completed(&self) -> bool {
        (self.deletes_next_run && self.tested) || (self.deletes_on_activation && self.activated)
    }

    pub fn effect(&self) -> &AuxEffect {
        &self.effect
    }

    pub fn callbacks(&self) -> &[BattleCallback] {
        &self.callbacks
    }

    pub fn priority(&self) -> (usize, usize) {
        let effect_priority = self.effect.priority();
        let requirements_priority = self
            .requirements
            .iter()
            .map(|(requirement, _)| requirement.priority())
            .max()
            .unwrap_or(AuxRequirement::UNCONDITIONAL_PRIORITY);

        (effect_priority, requirements_priority)
    }

    pub fn mark_tested(&mut self) {
        self.tested = self.requirements.iter().all(|(_, state)| state.tested);
    }

    pub fn activated(&self) -> bool {
        self.activated
    }

    pub fn mark_activated(&mut self) {
        self.activated = true;
    }

    pub fn reset_tests(&mut self) {
        for (requirement, state) in &mut self.requirements {
            *state = RequirementState::default();

            // we should always consider these as tested
            state.tested = matches!(requirement, AuxRequirement::ContextStart);
        }

        self.activated = false;
        self.tested = false;
    }

    pub fn passed_all_tests(&self) -> bool {
        self.requirements.iter().all(|(_, state)| state.passed)
    }

    pub fn passed_non_time_tests(&self) -> bool {
        self.requirements
            .iter()
            .filter(|(requirement, _)| !matches!(requirement, AuxRequirement::Interval(_)))
            .all(|(_, state)| state.passed)
    }

    pub fn hit_passes_tests(
        &mut self,
        entity: &Entity,
        health: i32,
        max_health: i32,
        hit_props: &HitProperties,
    ) -> bool {
        let previously_passing = self.passed_all_tests();
        self.process_hit(entity, health, max_health, hit_props);
        let now_passing = self.passed_all_tests();

        // reset
        if previously_passing {
            for (_, state) in &mut self.requirements {
                state.passed = true;
            }
        }

        now_passing
    }

    pub fn mark_context_start(&mut self) {
        for (requirement, state) in &mut self.requirements {
            if matches!(requirement, AuxRequirement::ContextStart) {
                state.passed = true;
            }
        }
    }

    pub fn process_body(
        &mut self,
        emotion_window: Option<&EmotionWindow>,
        player: Option<&Player>,
        character: Option<&Character>,
        entity: &Entity,
        action_queue: Option<&ActionQueue>,
    ) {
        let emotion = emotion_window.map(|emotion_window| emotion_window.emotion());
        let card = character.and_then(|character| character.cards.last());

        for (requirement, state) in &mut self.requirements {
            let result = match requirement {
                AuxRequirement::Element(elem) => *elem == entity.element,
                AuxRequirement::Emotion(emot) => {
                    emotion.is_some_and(|emotion| emot == emotion) || *emot == Emotion::default()
                }
                AuxRequirement::NegativeTileInteraction => !entity.ignore_negative_tile_effects,
                AuxRequirement::ChargeTime(cmp, time) => player.is_some_and(|player| {
                    let max_charge_time = player.attack_charge.max_charge_time();
                    let charge_time = player.attack_charge.charging_time();

                    cmp.compare(charge_time.min(max_charge_time), *time)
                }),
                AuxRequirement::CardChargeTime(cmp, time) => player.is_some_and(|player| {
                    let max_charge_time = player.card_charge.max_charge_time();
                    let charge_time = player.card_charge.charging_time();

                    cmp.compare(charge_time.min(max_charge_time), *time)
                }),
                AuxRequirement::ChargedCard => player.is_some_and(|player| {
                    player.card_charge.fully_charged() || player.card_charged
                }),
                AuxRequirement::CardPrimaryElement(elem) => {
                    card.is_some_and(|card| card.element == *elem)
                }
                AuxRequirement::CardNotPrimaryElement(elem) => {
                    card.is_some_and(|card| card.element != *elem)
                }
                AuxRequirement::CardElement(elem) => card
                    .is_some_and(|card| card.element == *elem || card.secondary_element == *elem),
                AuxRequirement::CardNotElement(elem) => card
                    .is_some_and(|card| card.element != *elem && card.secondary_element != *elem),
                AuxRequirement::CardDamage(cmp, damage) => {
                    card.is_some_and(|card| cmp.compare(card.damage, *damage))
                }
                AuxRequirement::CardRecover(cmp, recover) => {
                    card.is_some_and(|card| cmp.compare(card.recover, *recover))
                }
                AuxRequirement::CardHitFlags(flags) => {
                    card.is_some_and(|card| card.hit_flags & *flags == *flags)
                }
                AuxRequirement::CardHitFlagsAbsent(flags) => {
                    card.is_some_and(|card| card.hit_flags & *flags == 0)
                }
                AuxRequirement::CardCode(code) => card.is_some_and(|card| card.code == *code),
                AuxRequirement::CardClass(class) => {
                    card.is_some_and(|card| card.card_class == *class)
                }
                AuxRequirement::CardNotClass(class) => {
                    card.is_some_and(|card| card.card_class != *class)
                }
                AuxRequirement::CardTimeFreeze(time_freeze) => {
                    card.is_some_and(|card| card.time_freeze == *time_freeze)
                }
                AuxRequirement::CardTag(tag) => card.is_some_and(|card| card.tags.contains(tag)),
                AuxRequirement::CardNotTag(tag) => {
                    card.is_some_and(|card| !card.tags.contains(tag))
                }
                AuxRequirement::Action(action_type_filter) => {
                    action_queue.is_some_and(|q| q.action_type & *action_type_filter != 0)
                }
                _ => continue,
            };

            state.tested = true;
            state.passed = result;
        }
    }

    pub fn process_card(&mut self, card: Option<&CardProperties>) {
        for (requirement, state) in &mut self.requirements {
            let result = match requirement {
                AuxRequirement::CardPrimaryElement(elem) => {
                    card.is_some_and(|card| card.element == *elem)
                }
                AuxRequirement::CardNotPrimaryElement(elem) => {
                    card.is_some_and(|card| card.element != *elem)
                }
                AuxRequirement::CardElement(elem) => card
                    .is_some_and(|card| card.element == *elem || card.secondary_element == *elem),
                AuxRequirement::CardNotElement(elem) => card
                    .is_some_and(|card| card.element != *elem && card.secondary_element != *elem),
                AuxRequirement::CardDamage(cmp, damage) => {
                    card.is_some_and(|card| cmp.compare(card.damage, *damage))
                }
                AuxRequirement::CardRecover(cmp, recover) => {
                    card.is_some_and(|card| cmp.compare(card.recover, *recover))
                }
                AuxRequirement::CardHitFlags(flags) => {
                    card.is_some_and(|card| card.hit_flags & *flags == *flags)
                }
                AuxRequirement::CardCode(code) => card.is_some_and(|card| card.code == *code),
                AuxRequirement::CardClass(class) => {
                    card.is_some_and(|card| card.card_class == *class)
                }
                AuxRequirement::CardNotClass(class) => {
                    card.is_some_and(|card| card.card_class != *class)
                }
                AuxRequirement::CardTimeFreeze(time_freeze) => {
                    card.is_some_and(|card| card.time_freeze == *time_freeze)
                }
                AuxRequirement::CardTag(tag) => card.is_some_and(|card| card.tags.contains(tag)),
                AuxRequirement::CardNotTag(tag) => {
                    card.is_some_and(|card| !card.tags.contains(tag))
                }
                _ => continue,
            };

            state.tested = true;
            state.passed = result;
        }
    }

    pub fn process_time(&mut self, time_frozen: bool, time: FrameTime) {
        for (requirement, state) in &mut self.requirements {
            let AuxRequirement::Interval(n) = requirement else {
                continue;
            };

            state.tested = true;
            state.passed = !time_frozen && time % *n == 0;
        }
    }

    pub fn process_hit(
        &mut self,
        entity: &Entity,
        health: i32,
        max_health: i32,
        hit_props: &HitProperties,
    ) {
        for (requirement, state) in &mut self.requirements {
            let result = match requirement {
                AuxRequirement::HitElement(element) => {
                    hit_props.element == *element || hit_props.secondary_element == *element
                }
                AuxRequirement::HitElementIsWeakness => {
                    entity.element.is_weak_to(hit_props.element)
                        || entity.element.is_weak_to(hit_props.secondary_element)
                }
                AuxRequirement::HitFlags(hit_flag) => hit_props.flags & *hit_flag == *hit_flag,
                AuxRequirement::HitFlagsAbsent(hit_flag) => hit_props.flags & *hit_flag == 0,
                AuxRequirement::HitDamage(cmp, damage) => cmp.compare(hit_props.damage, *damage),
                AuxRequirement::ProjectedHitDamage(expr, cmp, damage) => {
                    let value = expr.eval(AuxVariable::create_resolver(
                        health,
                        max_health,
                        hit_props.damage,
                    ));

                    cmp.compare(value, *damage as f32)
                }
                _ => continue,
            };

            state.tested = true;
            state.passed = result;
        }
    }

    pub fn process_health_calculations(&mut self, health: i32, max_health: i32, total_damage: i32) {
        let get_var = AuxVariable::create_resolver(health, max_health, total_damage);

        for (requirement, state) in &mut self.requirements {
            if state.passed {
                // skip if we've already processed, occurs for hit and prehit aux props
                continue;
            }

            let result = match requirement {
                AuxRequirement::TotalDamage(cmp, reference) => {
                    cmp.compare(total_damage, *reference)
                }
                AuxRequirement::ProjectedHPThreshold(expr, cmp, reference) => {
                    let value = expr.eval(get_var);

                    cmp.compare(value / max_health as f32, *reference)
                }
                AuxRequirement::ProjectedHP(expr, cmp, reference) => {
                    let value = expr.eval(get_var);

                    cmp.compare(value, *reference as f32)
                }
                AuxRequirement::HPThreshold(cmp, reference) => {
                    cmp.compare(health as f32 / max_health as f32, *reference)
                }
                AuxRequirement::HP(cmp, reference) => cmp.compare(health, *reference),
                _ => continue,
            };

            state.tested = true;
            state.passed = result;
        }
    }

    pub fn from_lua<'lua>(
        resources: &SharedBattleResources,
        lua: &'lua rollback_mlua::Lua,
        lua_value: rollback_mlua::Value<'lua>,
    ) -> rollback_mlua::Result<Self> {
        let table: rollback_mlua::Table = lua.unpack(lua_value)?;

        let effect = AuxEffect::from_lua(resources, lua, table.raw_get("effect")?)?;
        let callbacks = table.raw_get("callbacks").unwrap_or_default();
        let deletes_on_activation = table.raw_get("delete_on_activate").unwrap_or_default();
        let deletes_next_run = table.raw_get("delete_next_run").unwrap_or_default();

        let requirements_table: rollback_mlua::Table = table.raw_get("requirements")?;
        let requirements_len = requirements_table.raw_len();
        let mut requirements = Vec::with_capacity(requirements_len);

        for lua_value in requirements_table.sequence_values() {
            let requirement = AuxRequirement::from_lua(resources, lua, lua_value?)?;
            requirements.push((requirement, RequirementState::default()));
        }

        Ok(AuxProp {
            requirements,
            effect,
            callbacks,
            deletes_on_activation,
            deletes_next_run,
            tested: false,
            activated: false,
        })
    }
}
