#include "bnMissile.h"
#include "bnRingExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

Missile::Missile(Team _team,Battle::Tile* target, float _duration) : duration(_duration), Spell(_team) {
    Missile::target = target;
    SetLayer(1);

    goingUp = true;
    setTexture(Textures().GetTexture(TextureType::MOB_METALMAN_ATLAS));

    anim = CreateComponent<AnimationComponent>(weak_from_this());
    anim->SetPath("resources/mobs/metalman/metalman.animation");
    anim->Load();
    anim->SetAnimation("MISSILE_UP", Animator::Mode::Loop);

    setScale(0.f, 0.f);

    progress = 0.0f;

    // Which direction to come down from
    if (GetTeam() == Team::blue) {
        start = sf::Vector2f(480, -480.0f);
    }
    else if (GetTeam() == Team::red) {
        start = sf::Vector2f(0, -480.0f);
    }
    else {
        Delete();
    }

    Audio().Play(AudioType::TOSS_ITEM_LITE);

    auto props = Hit::DefaultProperties;
    props.damage = 100;
    props.flags |= Hit::impact;
    SetHitboxProperties(props);

    anim->OnUpdate(0);
    Entity::drawOffset = { 0, -480.0f };
}

Missile::~Missile() {
}

void Missile::OnUpdate(double _elapsed) {
    setScale(2.f, 2.f);

    if(!goingUp) {
        if(progress > 1.0f) {
            float beta = swoosh::ease::linear(progress-1.00, duration, 1.0);

            float posX = (1.0f - beta) * start.x;
            float posY = (1.0f - beta) * start.y;

            Entity::drawOffset = { posX, posY };

            // When at the end of the arc
            if (beta >= 1.0f) {
                // update tile to target tile
                tile->AffectEntities(*this);

                if(tile->GetState() != TileState::empty && tile->GetState() != TileState::broken) {
                    field->AddEntity(std::make_shared<RingExplosion>(), *GetTile());
                }

                Delete();
            }
        }

        progress += _elapsed;

        HighlightTile(Battle::Tile::Highlight::flash);

    } else {
        HighlightTile(Battle::Tile::Highlight::none);

        float beta = swoosh::ease::linear(progress, duration, 1.0);

        float posX = 16.0f;
        float posY = (beta * (-480.0f)) + ((1.0f - beta) * (-128));

        Entity::drawOffset = { posX, posY };

        // When at the end of the arc
        if (progress >= duration * 5) {
            // update to come down
            goingUp = false;
            progress = 0;
            anim->SetAnimation("MISSILE_DOWN", Animator::Mode::Loop);

            Entity::drawOffset = { 0, -480.f };

            if(GetTile() != target) {
                tile->RemoveEntityByID(GetID());
                AdoptTile(target);
            }
        }

        progress += _elapsed;
    }
}

void Missile::OnDelete()
{
  Remove();
}

void Missile::Attack(std::shared_ptr<Character> _entity) {
    _entity->Hit(GetHitboxProperties());
}
