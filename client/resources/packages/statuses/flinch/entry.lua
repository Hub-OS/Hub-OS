local GAME_FOLDER = Resources.game_folder()
local SFX = Resources.load_audio(GAME_FOLDER .. "resources/sfx/hurt.ogg")

---@param status Status
function status_init(status)
  local entity = status:owner()
  local animation = entity:animation()

  if not animation:has_state("CHARACTER_HIT") then
    return
  end

  if not status:reapplied() then
    entity:cancel_movement()
    entity:cancel_actions()
  end

  -- set the animation immediately
  animation:set_state("CHARACTER_HIT")

  -- use no animation on the action to carry the previous animation
  local action = Action.new(entity)
  action:set_lockout(ActionLockout.new_sequence())

  local i = 0

  local wait_step = action:create_step()
  wait_step.on_update_func = function()
    i = i + 1

    -- we want to flinch for 22 frames, confirmed by counting frames in battle
    if i == 21 then
      wait_step:complete_step()
    end
  end

  entity:queue_action(action)

  Resources.play_audio(SFX)
end
