local bn_assets = require("BattleNetwork.Assets")

local animation_path = "canodumb.animation"
local cannon_sfx = bn_assets.load_audio("cannon.ogg")

-- cursor.lua
local Cursor = {}

local function spawn_barrel_smoke(canodumb)
  local field = canodumb:field()
  local tile = canodumb:current_tile()

  local artifact = Artifact.new()
  artifact:set_facing(canodumb:facing())
  artifact:sprite():set_layer(-1)
  artifact:set_texture(canodumb:texture())

  if canodumb:facing() == Direction.Left then
    artifact:set_offset(-20, -33)
  else
    artifact:set_offset(20, -33)
  end

  local anim = artifact:animation()
  anim:load(animation_path)
  anim:set_state("SMOKE")
  anim:on_complete(function()
    artifact:erase()
  end)

  field:spawn(artifact, tile:x(), tile:y())
end

local function attack_tile(canodumb, attack_spell, tile)
  if not tile then
    attack_spell:erase()
    return
  end

  -- set the cursor up for damage
  attack_spell:set_hit_props(HitProps.new(
    canodumb._attack,
    Hit.Flinch | Hit.Impact | Hit.Flash,
    Element.None,
    canodumb:context(),
    Drag.None
  ))

  tile:attack_entities(attack_spell)
end

local function attack(canodumb, cursor)
  Resources.play_audio(cannon_sfx)

  spawn_barrel_smoke(canodumb)

  local cursor_anim = cursor.spell:animation()
  cursor_anim:set_state("CURSOR_SMOKE")
  cursor_anim:set_playback(Playback.Once)
  cursor_anim:on_complete(function()
    cursor:erase()
  end)

  local canodumb_anim = canodumb:animation()
  canodumb_anim:set_state(canodumb._shoot_state)
  canodumb_anim:on_complete(function()
    canodumb_anim:set_state(canodumb._idle_state)
  end)

  -- create a new attack spell to deal damage
  -- we can't deal damage to a target if we've hit them and they haven't moved
  local attack_spell = Spell.new(canodumb:team())

  local tile = canodumb:get_tile()
  local direction = canodumb:facing()

  attack_spell.on_update_func = function()
    tile = tile:get_tile(direction, 1)

    attack_tile(canodumb, attack_spell, tile)
  end

  attack_spell.on_collision_func = function()
    -- hit something, dont hit anything more
    attack_spell:erase()
  end

  -- attach the spell so attacks can register
  canodumb:field():spawn(attack_spell, 0, 0)
end

local function begin_attack(canodumb, cursor)
  -- stop the cursor from scanning for players
  cursor.spell.on_update_func = nil
  cursor.spell.on_collision_func = nil

  local cursor_anim = cursor.spell:animation()
  cursor_anim:set_state(canodumb._cursor_shoot_state)
  cursor_anim:set_playback(Playback.Once)
  cursor_anim:on_complete(function()
    canodumb._should_attack = true
  end)
end

local function spawn_cursor(cursor, canodumb, tile)
  local spell = Spell.new(canodumb:team())
  spell:set_texture(canodumb:texture())
  spell:sprite():set_layer(-1)
  spell:set_hit_props(HitProps.new(
    0,
    Hit.None,
    Element.None,
    canodumb:context()
  ))

  local anim = spell:animation()
  anim:load(animation_path)
  anim:set_state(canodumb._cursor_state)
  anim:set_playback(Playback.Once)

  local field = canodumb:field()
  field:spawn(spell, tile)

  spell.on_update_func = function()
    cursor.remaining_frames = cursor.remaining_frames - 1

    -- test if we need to move
    if cursor.remaining_frames > 0 then
      return
    end

    tile = tile:get_tile(canodumb:facing(), 1)

    if tile then
      cursor.remaining_frames = canodumb._frames_per_cursor_movement + 1
      spell:teleport(tile)
    else
      cursor:erase()
    end
  end

  spell.on_collision_func = function()
    begin_attack(canodumb, cursor)
  end

  spell.can_move_to_func = function(tile)
    return true
  end

  return spell
end

function Cursor:new(canodumb, tile)
  local cursor = {
    canodumb = canodumb,
    spell = nil,
    remaining_frames = canodumb._frames_per_cursor_movement
  }

  setmetatable(cursor, self)
  self.__index = self

  cursor.spell = spawn_cursor(cursor, canodumb, tile)

  return cursor
end

function Cursor:erase()
  self.spell:erase()
  self.canodumb._cursor = nil
end

-- entry.lua

local target_update, idle_update

target_update = function(canodumb, dt)
  if canodumb._cursor then
    local spell = canodumb._cursor.spell

    spell:current_tile():attack_entities(spell)

    if canodumb._should_attack then
      canodumb._should_attack = false
      attack(canodumb, canodumb._cursor)
    end
  else
    canodumb.on_update_func = idle_update
  end
end

idle_update = function(canodumb, dt)
  local y = canodumb:get_tile():y()
  local team = canodumb:team()

  local field = canodumb:field()
  local targets = field:find_characters(function(c)
    -- same row, different team
    return c:team() ~= team and c:get_tile():y() == y
  end)

  if #targets == 0 then return end -- no target

  -- found a target, spawn a cursor and change state
  local cursor_tile = canodumb:get_tile(canodumb:facing(), 1)
  canodumb._cursor = Cursor:new(canodumb, cursor_tile)
  canodumb.on_update_func = target_update
end

local delete_func = function(canodumb)
  if canodumb._cursor then
    canodumb._cursor:erase()
  end
  canodumb:default_character_delete()
end

function character_init(canodumb)
  -- private variables
  canodumb._frames_per_cursor_movement = 15
  canodumb._cursor = nil
  canodumb._idle_state = "IDLE_1"
  canodumb._shoot_state = "SHOOT_1"
  canodumb._cursor_state = "CURSOR"
  canodumb._cursor_shoot_state = "CURSOR_SHOOT"
  canodumb._attack = 10
  canodumb._should_attack = false

  -- meta
  canodumb:set_name("Canodumb")
  canodumb:set_height(55)

  local rank = canodumb:rank()

  if rank == Rank.V1 then
    canodumb:set_health(60)
  elseif rank == Rank.V2 then
    canodumb._idle_state = "IDLE_2"
    canodumb._shoot_state = "SHOOT_2"
    canodumb._attack = 50
    canodumb:set_health(90)
  elseif rank == Rank.V3 then
    canodumb._idle_state = "IDLE_3"
    canodumb._shoot_state = "SHOOT_3"
    canodumb._attack = 100
    canodumb:set_health(130)
  else
    canodumb._idle_state = "IDLE_SP"
    canodumb._shoot_state = "SHOOT_SP"
    canodumb._attack = 200
    canodumb:set_health(180)
  end

  canodumb:set_texture(Resources.load_texture("canodumb.png"))

  local anim = canodumb:animation()
  anim:load(animation_path)
  anim:set_state(canodumb._idle_state)
  anim:set_playback(Playback.Once)

  -- setup defense rules
  canodumb:add_defense_rule(DefenseVirusBody.new())

  -- setup event hanlders
  canodumb.on_update_func = idle_update
  canodumb.on_delete_func = delete_func
end
