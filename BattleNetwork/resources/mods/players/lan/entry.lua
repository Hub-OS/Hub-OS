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
    player:set_texture(Engine.load_texture(_modpath.."navi_megaman_atlas.png"), false)
    player:store_base_palette(Engine.load_texture(_modpath.."forms/base.palette.png"))
    player:set_palette(player:get_base_palette())

    player.update_func = function(self, dt)
        -- nothing in particular
    end

    player.normal_attack_func = create_normal_attack
    player.charged_attack_func = create_charged_attack
    -- player.special_attack_func = create_special_attack

    -- aqua form
    local spout = player:create_form()
    spout:set_mugshot_texture_path(_modpath.."forms/spout_entry.png")
    spout.on_activate_func = function(self, player)
        self.overlay = player:create_sync_node("head")

        local sprite = self.overlay:sprite()
        sprite:set_texture(Engine.load_texture(_modpath.."forms/spout_cross.png"), false)
        sprite:set_layer(-1)
        sprite:enable_parent_shader(false)

        self.overlay:get_animation():load(_modpath.."forms/spout_cross.animation")

        player:set_palette(Engine.load_texture(_modpath.."forms/aqua.palette.png"))
    end

    spout.on_deactivate_func = function(self, player)
        player:remove_sync_node(self.overlay)
        player:set_palette(player:get_base_palette())
    end

    spout.update_func = function(self, dt, player) end

    spout.charged_attack_func = function(player)
        return Battle.Bomb.new(player, 20)
    end

    -- dust form
    local dust = player:create_form()
    dust:set_mugshot_texture_path(_modpath.."forms/dust_entry.png")
    dust.on_activate_func = function(self, player)
        self.overlay = player:create_sync_node("head")

        local sprite = self.overlay:sprite()
        sprite:set_texture(Engine.load_texture(_modpath.."forms/dust_cross.png"), false)
        sprite:set_layer(-1)
        sprite:enable_parent_shader(false)

        self.overlay:get_animation():load(_modpath.."forms/dust_cross.animation")

        player:set_palette(Engine.load_texture(_modpath.."forms/dust.palette.png"))
    end

    dust.on_deactivate_func = function(self, player)
        player:remove_sync_node(self.overlay)
        player:set_palette(player:get_base_palette())
    end

    dust.update_func = function(self, dt, player) end

    dust.charged_attack_func = function(player)
        return Battle.Buster.new(player, true, 200)
    end

    -- slash form
    local slash = player:create_form()
    slash:set_mugshot_texture_path(_modpath.."forms/slash_entry.png")
    slash.on_activate_func = function(self, player)
        self.overlay = player:create_sync_node("head")

        local sprite = self.overlay:sprite()
        sprite:set_texture(Engine.load_texture(_modpath.."forms/slash_cross.png"), false)
        sprite:set_layer(-1)
        sprite:enable_parent_shader(false)

        self.overlay:get_animation():load(_modpath.."forms/slash_cross.animation")

        player:set_palette(Engine.load_texture(_modpath.."forms/slash.palette.png"))
    end

    slash.on_deactivate_func = function(self, player)
        player:remove_sync_node(self.overlay)
        player:set_palette(player:get_base_palette())
    end

    slash.update_func = function(self, dt, player) end

    slash.charged_attack_func = function(player)
        return Battle.Sword.new(player, 80)
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
