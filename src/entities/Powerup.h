#pragma once

#include "Entity.h"
#include <memory>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace sf { class Sprite; }

class Powerup final : public Entity
{
public:
    static constexpr float collisionRadius = 24.0f;

    Powerup() = default;
    virtual ~Powerup() = default;
    
    bool init() override;
    void update(float dt) override;
    void update(float dt, bool boostEntitySpeed);
    void render(sf::RenderTarget& target) const override;
};

