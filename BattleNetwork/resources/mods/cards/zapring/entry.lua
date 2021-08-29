nonce = function() end

DAMAGE = 20
TEXTURE = nil
BUSTER_TEXTURE = nil
AUDIO = nil

function package_init(package) 
    package:declare_package_id("com.claris.card.zapring")
    package:set_icon_texture(Engine.load_texture(_modpath.."icon.png"))
    package:set_preview_texture(Engine.load_texture(_modpath.."preview.png"))

    local props = package:get_card_props()
    props.shortname = "Zap Ring"
    props.code = '*'
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

        self.tile      = actor:get_current_tile()
		self.zapbuster = nil
        self.zapring   = nil
        
		local ref = self
		
        local step1 = Battle.Step.new()

        step1.update_func = function(self, dt) 
            if ref.zapbuster == nil then
                ref.zapbuster = create_zap_buster()
                actor:get_field():spawn(ref.beastman, ref.tile:x(), ref.tile:y())
            end
			if ref.zapring == nil then
				ref.zapring = create_zap()
				actor:get_field():spawn(ref.zapring, ref.tile:x(), ref.tile:y())
			end
            self:complete_step()
        end

    return action
end

function create_zap_buster() 
    local fx = Battle.Artifact.new()
    fx:set_texture(BUSTER_TEXTURE, true)
    fx:get_animation():load(_modpath.."buster_zapring.animation")
    fx:get_animation():set_state("DEFAULT")
    fx:get_animation():on_complete(function() 
        fx:remove()
    end)
    return fx
end

function create_zap(animation_state, direction, user)
    local spell = Battle.Spell.new(user:get_team())
    spell:set_texture(TEXTURE, true)
    spell:highlight_tile(Highlight.Solid)
	
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
            self:slide(dest, frames(7), frames(0), ActionOrder.Voluntary, 
                function()
                    ref.slide_started = true 
                end
            )
        end
    end

    spell.attack_func = function(self, other) 
        -- nothing
    end

    spell.delete_func = function(self) 
    end

    spell.can_move_to_func = function(tile)
        return true
    end

	Engine.play_audio(AUDIO, AudioPriority.High)

    return spell
end