local GAME_FOLDER = Resources.game_folder()
local SFX = Resources.load_audio(GAME_FOLDER .. "resources/sfx/hurt.ogg")

---@param status Status
function status_init(status)
  local entity = status:owner()
  local animation = entity:animation()

  if not animation:has_state("CHARACTER_HIT") then
    return
  end

  entity:cancel_actions()
  entity:cancel_movement()

  local action = Action.new(entity, "CHARACTER_HIT")
  action:set_lockout(ActionLockout.new_sequence())

  local i = 0

  local wait_step = action:create_step()
  wait_step.on_update_func = function()
    i = i + 1

    -- we want to flinch for 22 frames
    -- timing through the action's on_update_func shows this is accurate
    if i == 21 then
      wait_step:complete_step()
    end
  end

  entity:queue_action(action)

  Resources.play_audio(SFX)
end
