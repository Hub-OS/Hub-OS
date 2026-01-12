local table, card_properties, button_slot = ...

local button = table:create_card_button(1, button_slot)
---@type Entity
local player = button:owner()

button:use_card_preview(card_properties)
button:use_fixed_card_cursor(true)

-- create frame
button:set_texture(Resources.game_folder() .. "resources/scenes/battle/card_select.png")
local button_animation = button:animation()
button_animation:load(Resources.game_folder() .. "resources/scenes/battle/card_select.animation")
button_animation:set_state("ICON_FRAME")

-- create icon
player:stage_card(card_properties)
local icon_texture = player:staged_item_texture(#player:staged_items())
player:pop_staged_item()

local button_sprite = button:sprite()
local icon_node = button_sprite:create_node()
icon_node:set_texture(icon_texture)

local text_style = TextStyle.new("CODE")
local code_node = button_sprite:create_text_node(text_style, card_properties.code)
code_node:set_color(Color.new(255, 255, 0))
code_node:set_offset(4, 16)

-- set description
button:use_card_description(card_properties)

-- updating ui for dynamic damage
local open_component = player:create_component(Lifetime.CardSelectOpen)
open_component.on_update_func = function()
  if button:deleted() then
    open_component:eject()
    return
  end

  if not card_properties.dynamic_damage then
    return
  end

  local update_component = player:create_component(Lifetime.Scene)
  local last_damage = card_properties.damage
  update_component.on_update_func = function()
    if button:deleted() then
      update_component:eject()
      return
    end

    card_properties.damage = CardProperties.resolve_damage(card_properties, player)

    if card_properties.damage ~= last_damage then
      last_damage = card_properties.damage
      button:use_card_preview(card_properties)
    end
  end

  local close_component = player:create_component(Lifetime.CardSelectClose)
  close_component.on_update_func = function()
    update_component:eject()
  end
end

-- functionality
local used = false

local close_component = player:create_component(Lifetime.CardSelectClose)

close_component.on_update_func = function()
  if used or button:deleted() then
    button:delete()
    close_component:eject()
    open_component:eject()
  end
end

local function card_allowed()
  -- check selection count
  local items = player:staged_items()
  local total_selection = 0

  for _, item in ipairs(items) do
    if item.category == "card" or item.category == "deck_card" or item.category == "icon" then
      total_selection = total_selection + 1
    end
  end

  if total_selection >= 5 then
    return false
  end

  if card_properties.code == "" then
    -- codeless, no need to check against restrictions
    return true
  end

  -- check against restriction
  local restriction = player:card_select_restriction()

  if not restriction.code and not restriction.package_id then
    -- no restriction
    return true
  end

  if restriction.code then
    return restriction.code == card_properties.code or card_properties.code == "*"
  end

  return restriction.package_id == card_properties.package_id
end

button.use_func = function()
  if used or not card_allowed() then
    return false
  end

  icon_node:set_visible(false)
  used = true

  player:stage_card(card_properties, function()
    used = false
    icon_node:set_visible(true)
  end)

  return true
end

button.on_selection_change_func = function()
  if card_allowed() then
    icon_node:set_shader_effect(SpriteShaderEffect.None)
  else
    icon_node:set_shader_effect(SpriteShaderEffect.Grayscale)
  end
end

return button
