function package_init(package) 
    package:declare_package_id("com.claris.navi.lan")
    package:set_speed(2.0)
	package:set_attack(3)
	package:set_charged_attack(30)
    package:set_special_description("It's the boys! Lan & Megaman!")
	package:set_icon_texture(Engine.load_texture(_modpath.."mega_face.png"))
	package:set_preview_texture(Engine.load_texture(_modpath.."preview.png"))
    package:set_overworld_animation_path(_modpath.."lan_ow.animation")
    package:set_overworld_texture_path(_modpath.."lan_ow.png")
    package:set_mugshot_animation_path(_modpath.."mug.animation")
	package:set_mugshot_texture_path(_modpath.."mug.png")
	package:set_emotions_texture_path(_modpath.."emotions.png")
end

function player_init(player)
    player:set_name("Hikaris")
    player:set_health(1000)
    player:set_element(Element.None)
    player:set_height(48.0)
	player:set_charge_position(1,-14)
    player:set_animation(_modpath.."megaman.animation")
    player:set_texture(Engine.load_texture(_modpath.."navi_megaman_atlas.og.png"), true)
    player:set_fully_charged_color(Color.new(255,0,0,255))
    player.update_func = function(self, dt)
        -- nothing in particular
    end
end

function create_charged_attack(player)
    print("execute charged attack")
    return Battle.Buster.new(player, true, 30)
end

function create_normal_attack(player)
    print("buster attack")
    return Battle.Buster.new(player, false, 3)
end

function create_special_attack(player)
    print("execute special")
    return Battle.Bomb.new(player, 20)
end