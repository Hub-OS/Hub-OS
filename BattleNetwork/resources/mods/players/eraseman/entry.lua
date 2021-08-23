function package_init(package) 
    package:declare_package_id("com.example.player.Eraseman")
    package:set_special_description("Scripted net navi!")
    package:set_speed(2.0)
    package:set_attack(2)
    package:set_charged_attack(20)
    package:set_icon_texture(Engine.load_texture(_modpath.."eraseman_face.png"))
    package:set_preview_texture(Engine.load_texture(_modpath.."preview.png"))
    package:set_overworld_animation_path(_modpath.."erase_OW.animation")
    package:set_overworld_texture_path(_modpath.."erase_OW.png")
    package:set_mugshot_texture_path(_modpath.."mug.png")
    package:set_mugshot_animation_path(_modpath.."mug.animation")
end

function player_init(player)
    player:set_name("Eraseman")
    player:set_health(1100)
    player:set_element(Element.Cursor)
    player:set_height(55.0)
    player:set_animation(_modpath.."eraseman.animation")
    player:set_texture(Engine.load_texture(_modpath.."navi_eraseman_atlas.png"), true)
    player:set_fully_charged_color(Color.new(255,0,0,255))

    player.update_func = function(self, dt) 
        -- nothing in particular
    end
end

function create_normal_attack(player)
    print("buster attack")
    
    return Battle.Buster.new(player, false, 10)
end

function create_charged_attack(player)
    print("charged attack")

    local action = Battle.CardAction.new(player, "PLAYER_SPECIAL")
    action:set_lockout(make_animation_lockout())

    action.execute_func = function(self, player)
        local do_attack = function()
            local t1 = player:get_tile(player:get_facing(), 1)
            local t2 = player:get_tile(player:get_facing(), 2)

            spawn_attack(player, t1:x(), t1:y())
            spawn_attack(player, t2:x(), t2:y())
            spawn_attack(player, t2:x(), t2:y()-1)
            spawn_attack(player, t2:y(), t2:y()+1)
        end

        self:add_anim_action(3, do_attack)
    end

    local meta = action:copy_metadata()
    meta.time_freeze = true
    meta.shortname = "Decapitation"
    action:set_metadata(meta)

    return action
end

function create_special_attack(player)
    print("execute special")
    return nil
end

function spawn_attack(user, x, y)
    local spell = Battle.Spell.new(user:get_team())

    spell:set_hit_props(
        make_hit_props(
            50, 
            Hit.Impact | Hit.Drag | Hit.Flinch, 
            Element.Cursor, 
            user:get_id(), 
            drag(user:get_facing(), 1)
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