#pragma once

#include "Entity.h"
#include <memory>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace sf { class Sprite; }

class Player final : public Entity
{
public:
    static constexpr float gravity = 4000.0f;
    static constexpr float jumpVelocity = 1300.0f;

    Player();
    virtual ~Player() = default;

    bool init() override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

    // Launch the player upward, e.g. when jumping or bouncing off an enemy.
    void bounce(float velocityY);
};
