nonce = function() end

DAMAGE = 20
TEXTURE = nil
BUSTER_TEXTURE = nil
AUDIO = nil

function package_init(package) 
    package:declare_package_id("com.claris.card.zapring")
    package:set_icon_texture(Engine.load_texture(_modpath.."icon.png"))
    package:set_preview_texture(Engine.load_texture(_modpath.."preview.png"))
	package:set_codes({'*'})

    local props = package:get_card_props()
    props.shortname = "Zap Ring"
    props.damage = DAMAGE
    props.time_freeze = false
    props.element = Element.Elec
    props.description = "Ring stuns enmy ahead!"

    -- assign the global resources
    TEXTURE = Engine.load_texture(_modpath.."spell_zapring.png")
	BUSTER_TEXTURE = Engine.load_texture(_modpath.."buster_zapring.png")
	AUDIO = Engine.load_audio(_modpath.."fwish.ogg")
end

--[[
    1. megaman loads buster
    2. zapring flies out
--]]
function card_create_action(actor, props)
    print("in create_card_action()!")
    local action = Battle.CardAction.new(actor, "PLAYER_SHOOTING")

    action.execute_func = function(self, user)
        print("in custom card action execute_func()!")

        self.tile = actor:get_current_tile()
		self.zapbuster = nil
        self.zapring = nil
		self.anim = nil
		
		if self.zapbuster == nil then
			self.zapbuster = create_zap_buster()
			self.anim = Engine.Animation.new(_modpath.."buster_zapring.animation")
			self.anim:set_state("DEFAULT")
			self:add_attachment(user, "BUSTER", self.zapbuster):use_animation(self.anim)
		end
		if self.zapring == nil then
			self.zapring = create_zap("DEFAULT", actor)
			actor:get_field():spawn(self.zapring, self.tile:x(), self.tile:y())
		end
	end
    return action
end

function create_zap_buster()
	local proxySprite = Engine.SpriteNode.new()
	proxySprite:set_texture(BUSTER_TEXTURE, true)
	proxySprite:set_layer(-1)
	return proxySprite
end

function create_zap(animation_state, user)
    local spell = Battle.Spell.new(user:get_team())
    spell:set_texture(TEXTURE, true)
    spell:highlight_tile(Highlight.Solid)
	spell:set_height(16.0)
	local direction = user:get_facing()
    spell.slide_started = false
	
    spell:set_hit_props(
        make_hit_props(
            DAMAGE, 
            Hit.Impact | Hit.Stun | Hit.Flinch, 
            Element.Elec,
            user:get_id(),
            no_drag()
        )
    )
	
    local anim = spell:get_animation()
    anim:load(_modpath.."spell_zapring.animation")
    anim:set_state(animation_state)

    spell.update_func = function(self, dt) 
        self:get_current_tile():attack_entities(self)

        if self:is_sliding() == false then 
            if self:get_current_tile():is_edge() and self.slide_started then 
                self:delete()
            end 

            local dest = self:get_tile(direction, 1)
            local ref = self
            self:slide(dest, frames(4), frames(0), ActionOrder.Voluntary, 
                function()
                    ref.slide_started = true 
                end
            )
        end
    end

    spell.attack_func = function(self, other) 
        -- nothing
    end
	
	spell.collision_func = function(self, other)
		self:remove()
	end
	
    spell.delete_func = function(self) 
    end

    spell.can_move_to_func = function(tile)
        return true
    end

	Engine.play_audio(AUDIO, AudioPriority.High)

    return spell
end