function load_texture(path)
    return Engine.ResourceHandle.new().textures:load_file(path)
end

function roster_init(info) 
    print("modpath: ".._modpath)
    info:declare_package_id("com.example.player.Eraseman")
    info:set_special_description("Scripted net navi!")
    info:set_speed(2.0)
    info:set_attack(2)
    info:set_charged_attack(20)
    info:set_icon_texture(load_texture(_modpath.."eraseman_face.png"))
    info:set_preview_texture(load_texture(_modpath.."preview.png"))
    info:set_overworld_animation_path(_modpath.."erase_OW.animation")
    info:set_overworld_texture_path(_modpath.."erase_OW.png")
    info:set_mugshot_texture_path(_modpath.."mug.png")
    info:set_mugshot_animation_path(_modpath.."mug.animation")
end

function battle_init(player)
    player:set_name("Eraseman")
    player:set_health(1100)
    player:set_element(Element.Cursor)
    player:set_height(100.0)
    print("here 1")
    player:set_animation(_modpath.."eraseman.animation")
    print("here 2")
    player:set_texture(load_texture(_modpath.."navi_eraseman_atlas.png"), true)
    print("here 3")
    player:set_fully_charged_color(Color.new(255,0,0,255))
    print("here 4")
end

function execute_special_attack(player)
    print("execute special")
    return nil
end

function execute_buster_attack(player)
    print("buster attack")
    return Battle.Buster.new(player, false, 10)
end

function execute_charged_attack(player)
    print("charged attack")
    return special_card_action(player)
end

function spawn_attack(user, x, y)
    local spell = Battle.Spell.new(user:get_team())

    spell:set_hit_props(
        make_hit_props(
            50, 
            Hit.Impact | Hit.Drag | Hit.Flinch, 
            Element.Cursor, 
            user:get_id(), 
            drag(Direction.Right, 1)
        )
    )

    spell.update_func = function(self, dt) 
        self:get_current_tile():attack_entities(self)
        self:delete()
    end

    spell.attack_func = function(self, other) 
        -- on hit does nothing
    end

    spell:highlight_tile(Highlight.Flash)

    user:get_field():spawn(spell, x, y)
end

function special_card_action(user) 
    local action = Battle.CardAction.new(user, "PLAYER_SPECIAL")
    action:set_lockout(make_action_lockout(LockType.Animation, 0, Lockout.Weapons))

    action.execute_func = function(self, actor)
        local do_attack = function()
            local t1 = user:get_tile(Direction.Right, 1)
            local t2 = user:get_tile(Direction.Right, 2)

            spawn_attack(actor, t1:x(), t1:y())
            spawn_attack(actor, t2:x(), t2:y())
            spawn_attack(actor, t2:x(), t2:y()-1)
            spawn_attack(actor, t2:y(), t2:y()+1)
        end

        self:add_anim_action(3, do_attack)
    end

    return action
end