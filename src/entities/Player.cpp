#include "Player.h"
#include "ResourceManager.h"
#include "Constants.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <cmath>

Player::Player()
{
}

bool Player::init()
{
    const sf::Texture* pTexture = ResourceManager::getOrLoadTexture("player.png");
    if (pTexture == nullptr)
        return false;

    m_pSprite = std::make_unique<sf::Sprite>(*pTexture);
    if (!m_pSprite)
        return false;

    m_rotation = sf::degrees(0);
    sf::FloatRect localBounds = m_pSprite->getLocalBounds();
    m_pSprite->setOrigin({localBounds.size.x / 2.0f, localBounds.size.y / 2.0f});
    m_pSprite->setPosition(m_position);
    m_pSprite->setScale(sf::Vector2f(3.0f, 3.0f));
    m_collisionRadius = 12;

    return true;
}

// Up is negative y; the player rests on the ground at ZERO_Y.
void Player::update(float dt)
{
    if (m_position.y >= ZERO_Y && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space))
        bounce(jumpVelocity);
    m_position.y -= m_velocity.y * dt;
    m_velocity.y -= m_acceleration.y * dt;
    if (m_position.y > ZERO_Y) {
        m_position.y = ZERO_Y;
        m_velocity.y = 0;
    }
}

void Player::bounce(float velocityY)
{
    m_acceleration.y = gravity;
    m_velocity.y = velocityY;
}

void Player::render(sf::RenderTarget& target) const
{
    m_pSprite->setRotation(m_rotation);
    m_pSprite->setPosition(m_position);
    target.draw(*m_pSprite);
}
