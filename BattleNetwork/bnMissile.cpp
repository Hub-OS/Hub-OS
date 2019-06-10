#include "bnMissile.h"
#include "bnExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include <cmath>
#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

Missile::Missile(Field* _field, Team _team, Battle::Tile* target, float _duration) : anim(this), duration(_duration), Spell() {
    this->target = target;
    SetLayer(1);
    field = _field;
    team = _team;
    direction = Direction::NONE;
    deleted = false;
    goingUp = true;
    auto texture = TEXTURES.GetTexture(TextureType::MOB_METALMAN_ATLAS);
    setTexture(*texture);

    anim.Setup("resources/mobs/metalman/metalman.animation");
    anim.Reload();
    anim.SetAnimation("MISSILE_UP", Animate::Mode::Loop);

    setScale(0.f, 0.f);

    progress = 0.0f;

    // Which direction to come down from
    if (GetTeam() == Team::BLUE) {
        start = sf::Vector2f(target->getPosition().x + 480, target->getPosition().y -480.0f);
    }
    else if (GetTeam() == Team::RED) {
        start = sf::Vector2f(target->getPosition().x + 480, target->getPosition().y -480.0f);
    }
    else {
        this->Delete();
    }

    AUDIO.Play(AudioType::TOSS_ITEM_LITE);

    auto props = Hit::DefaultProperties;
    props.damage = 100;
    props.flags |= Hit::impact;
    this->SetHitboxProperties(props);

    anim.Update(0);
}

Missile::~Missile() {
}

void Missile::Update(float _elapsed) {
    setScale(2.f, 2.f);

    if(this->IsDeleted()) return;

    anim.Update(_elapsed);

    if(!goingUp) {
        if(progress > 1.0f) {
            double beta = swoosh::ease::linear(progress-1.0f, duration, 1.0);

            double posX = (beta * tile->getPosition().x) + ((1.0f - beta) * start.x);
            double posY = (beta * tile->getPosition().y) + ((1.0f - beta) * start.y);

            setPosition((float) posX, (float) posY);

            // When at the end of the arc
            if (beta >= 1.0f) {
                // update tile to target tile
                tile->AffectEntities(this);

                if(tile->GetState() != TileState::EMPTY && tile->GetState() != TileState::BROKEN) {
                    this->field->AddEntity(*(new Explosion(this->field, this->GetTeam(), 1)),
                                           this->GetTile()->GetX(), this->GetTile()->GetY());
                }

                this->Delete();
            }
        }

        progress += _elapsed;

        this->EnableTileHighlight(int(progress*15)%2);
    } else {
        double beta = swoosh::ease::linear(progress, duration, 1.0);

        double posX = tile->getPosition().x + 16.0f;

        double posY = (beta * (-128.0f)) + ((1.0f - beta) * (tile->getPosition().y - 128.0f));

        setPosition((float) posX, (float) posY);

        // When at the end of the arc
        if (progress >= duration * 5) {
            // update to come down
            goingUp = false;
            progress = 0;
            anim.SetAnimation("MISSILE_DOWN", Animate::Mode::Loop);

            if(this->GetTile() != target) {
                auto pos = this->getPosition();
                tile->RemoveEntityByID(this->GetID());
                this->AdoptTile(target);
                this->setPosition(pos);
            }

            setPosition(start.x, start.y);
        }

        progress += _elapsed;
    }
}

bool Missile::Move(Direction _direction) {
    return true;
}

void Missile::Attack(Character* _entity) {
    _entity->Hit(GetHitboxProperties());
}
