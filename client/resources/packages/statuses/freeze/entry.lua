local GAME_FOLDER = Resources.game_folder()
local TEXTURE = Resources.load_texture(GAME_FOLDER .. "resources/scenes/battle/statuses.png")
local SFX = Resources.load_audio(GAME_FOLDER .. "resources/sfx/freeze.ogg")
local FREEZE_COLOR = Color.new(178, 204, 229, 255)

-- shared animator
local animator = Animation.new()
animator:load(GAME_FOLDER .. "resources/scenes/battle/statuses.animation")

-- mashing helper
local inputs = {}

for _, value in pairs(Input.Pressed) do
  inputs[#inputs + 1] = value
end

local function is_mashing(entity)
  for _, value in ipairs(inputs) do
    if entity:input_has(value) then
      return true
    end
  end

  return false
end

function status_init(status)
  Resources.play_audio(SFX)

  local entity = status:owner()
  local entity_sprite = entity:sprite()
  local freeze_sprite = entity:create_node()
  freeze_sprite:set_texture(TEXTURE)
  freeze_sprite:set_offset(0, -entity:height() / 2)
  freeze_sprite:set_layer(-1)

  -- resolve state
  local animator_state

  if entity:height() <= 48.0 then
    animator_state = "FREEZE_SMALL"
  elseif entity:height() <= 75.0 then
    animator_state = "FREEZE_MEDIUM"
  else
    animator_state = "FREEZE_LARGE"
  end

  animator:set_state(animator_state)
  animator:apply(freeze_sprite)

  -- this component updates the status's animation and handles mashing
  local component = entity:create_component(Lifetimes.Battle)
  local time = 0

  component.on_update_func = function()
    freeze_sprite:set_offset(0, -entity:height() / 2)

    animator:set_state(animator_state)
    animator:set_playback(Playback.Loop)
    animator:sync_time(time)
    animator:apply(freeze_sprite)
    time = time + 1

    entity_sprite:set_color_mode(ColorMode.Additive)
    entity_sprite:set_color(FREEZE_COLOR)

    if is_mashing(entity) then
      local remaining_time = status:remaining_time()
      status:set_remaining_time(remaining_time - 1)
    end
  end

  -- defense rule to add Break weakness and refresh from Aqua attacks
  local defense_rule = DefenseRule.new(DefensePriority.Last, DefenseOrder.CollisionOnly)

  defense_rule.filter_statuses_func = function(hit_props)
    if not TurnGauge.frozen() and hit_props.element == Element.Aqua or hit_props.secondary_element == Element.Aqua then
      -- refresh when hit with an Aqua attack out of time freeze
      status:set_remaining_time(150)
    end

    if hit_props.element == Element.Break or hit_props.secondary_element == Element.Break then
      hit_props.damage = hit_props.damage * 2
      status:set_remaining_time(0)
    end

    return hit_props
  end

  entity:add_defense_rule(defense_rule)

  -- clean up
  status.on_delete_func = function()
    entity:remove_defense_rule(defense_rule)
    entity_sprite:remove_node(freeze_sprite)
    component:eject()
  end
end
