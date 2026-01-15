local AuxProp = {}
AuxProp.__index = AuxProp

function AuxProp.new()
  local table = {
    requirements = {},
    callbacks = {}
  }
  setmetatable(table, AuxProp)
  return table
end

local req_list = {
  "require_chance",
  "require_interval",
  "require_hit_element",
  "require_hit_element_is_weakness",
  "require_hit_flags",
  "require_hit_flags_absent",
  "require_hit_damage",
  "require_projected_hit_damage",
  "require_total_damage",
  "require_element",
  "require_emotion",
  "require_negative_tile_interaction",
  "require_context_start",
  "require_action",
  "require_charge_time",
  "require_card_charge_time",
  "require_charged_card",
  "require_card_primary_element",
  "require_card_not_primary_element",
  "require_card_element",
  "require_card_not_element",
  "require_card_damage",
  "require_card_recover",
  "require_card_hit_flags",
  "require_card_hit_flags_absent",
  "require_card_code",
  "require_card_class",
  "require_card_not_class",
  "require_card_time_freeze",
  "require_card_tag",
  "require_card_not_tag",
  "require_statuses",
  "require_statuses_absent",
  "require_projected_health_threshold",
  "require_projected_health",
  "require_health_threshold",
  "require_health",
}

for _, name in ipairs(req_list) do
  AuxProp[name] = function(self, ...)
    self.requirements[#self.requirements + 1] = { name, ... }
    return self
  end
end

local eff_list = {
  "update_context",
  "increase_card_multiplier",
  "increase_card_damage",
  "intercept_action",
  "interrupt_action",
  "increase_pre_hit_damage",
  "decrease_pre_hit_damage",
  "declare_immunity",
  "apply_status",
  "remove_status",
  "increase_hit_damage",
  "decrease_hit_damage",
  "decrease_total_damage",
  "drain_health",
  "recover_health",
}

for _, name in ipairs(eff_list) do
  AuxProp[name] = function(self, ...)
    if self.effect then
      error("effect already defined")
    end
    self.effect = { name, ... }
    return self
  end
end

function AuxProp:once()
  self.delete_on_activate = true
  return self
end

function AuxProp:immediate()
  self.delete_next_run = true
  return self
end

function AuxProp:with_callback(callback)
  self.callbacks[#self.callbacks + 1] = callback
  return self
end

return AuxProp
