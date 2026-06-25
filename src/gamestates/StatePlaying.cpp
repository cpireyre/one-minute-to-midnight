#include "StatePlaying.h"
#include "StateWinScreen.h"
#include "StatePaused.h"
#include "StateStack.h"
#include "ResourceManager.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
    uint32_t roll(uint32_t n) { return rand() % n; }

    // Shortest distance from point c to the axis-aligned box `bounds`.
    // Ref: https://www.jeffreythompson.org/collision-detection/circle-rect.php
    float pointToBoxDistance(sf::Vector2f c, const sf::FloatRect& bounds)
    {
        const sf::Vector2f min = bounds.position;
        const sf::Vector2f max = bounds.position + bounds.size;
        const float nearestX = std::clamp(c.x, min.x, max.x);
        const float nearestY = std::clamp(c.y, min.y, max.y);
        return std::hypot(c.x - nearestX, c.y - nearestY);
    }

    void scrollTextLeft(sf::Text* text, float dt)
    {
        const sf::Vector2f pos = text->getPosition();
        text->setPosition({pos.x - 400 * dt, pos.y});
    }

    constexpr float playerEnemyHitbox = 20.0f;
    constexpr float playerPowerupHitbox = 25.0f;
    constexpr float stompBounceVelocity = 600.0f;
    constexpr float boostDuration = 5.0f;

    // Sky texture index + ground colour applied as each in-game hour is reached.
    struct TimeOfDay { int hour; int skyTexture; uint32_t groundColor; };
    constexpr std::array<TimeOfDay, 4> dayNightSchedule = {{
        {16, 1, 0x795D4EFF}, // sunset
        {18, 2, 0x453D3FFF}, // dusk
        {20, 3, 0x2E343AFF}, // evening
        {22, 4, 0x141C1AFF}, // night
    }};
}

StatePlaying::StatePlaying(StateStack& stateStack)
    : m_stateStack(stateStack)
{
}

bool StatePlaying::init()
{
    // Skies from midday through to midnight, swapped in by the day/night schedule.
    const char* skyFiles[5] = {"1.jpg", "4.jpg", "5.jpg", "2.jpg", "3.jpg"};
    for (int i = 0; i < 5; ++i)
    {
        m_skyTextures[i] = ResourceManager::getOrLoadTexture(skyFiles[i]);
        if (m_skyTextures[i] == nullptr)
            return false;
    }
    m_pSky = std::make_unique<sf::Sprite>(*m_skyTextures[0]);

    m_ground.setSize({1024.0f, 1024.0f - ZERO_Y});
    m_ground.setPosition({0.0f, ZERO_Y});
    m_ground.setFillColor(sf::Color(0x5C794EFF));

    m_pPlayer = std::make_unique<Player>();
    if (!m_pPlayer->init())
        return false;
    m_pPlayer->setPosition(sf::Vector2f(ZERO_X, ZERO_Y));

    const sf::Font* pFont = ResourceManager::getOrLoadFont("Lavigne.ttf");
    if (pFont == nullptr)
        return false;

    m_clockText = std::make_unique<sf::Text>(*pFont);
    m_helpText = std::make_unique<sf::Text>(*pFont);
    m_controlText = std::make_unique<sf::Text>(*pFont);
    m_powerupText = std::make_unique<sf::Text>(*pFont);

    m_clockText->setString("12:00");
    m_clockText->setCharacterSize(90);
    m_clockText->setStyle(sf::Text::Bold);
    sf::FloatRect localBounds = m_clockText->getLocalBounds();
    m_clockText->setOrigin({localBounds.size.x / 120.0f - 10, localBounds.size.y / 120.0f - 10});

    m_helpText->setString("Survive until midnight!");
    m_helpText->setCharacterSize(50);
    m_helpText->setStyle(sf::Text::Bold);
    localBounds = m_helpText->getLocalBounds();
    m_helpText->setOrigin({localBounds.size.x / 2.f - 3000, localBounds.size.y / 1.0f - 200});

    m_controlText->setString("Jump on the red guys with space           Bounce off their heads with space");
    m_controlText->setCharacterSize(50);
    m_controlText->setStyle(sf::Text::Bold);
    localBounds = m_controlText->getLocalBounds();
    m_controlText->setOrigin({localBounds.size.x / 2.f - 1000, localBounds.size.y / 1.0f - 250});

    m_powerupText->setString("Collect yellow things to become invulnerable and go brr");
    m_powerupText->setCharacterSize(50);
    m_powerupText->setStyle(sf::Text::Bold);
    localBounds = m_powerupText->getLocalBounds();
    m_powerupText->setOrigin({localBounds.size.x / 2.f - 5000, localBounds.size.y / 1.0f - 250});

    return true;
}

void StatePlaying::update(float dt)
{
    // Advance the in-game clock (runs ~10x real time, faster while boosted).
    m_elapsedSeconds += dt * (10 + 2 * m_boostTimer);
    const int totalMinutes = static_cast<int>(m_elapsedSeconds);
    const int hour = 12 + totalMinutes / 60;
    const int minute = totalMinutes % 60;

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", hour, minute);
    m_clockText->setString(buffer);

    if (hour >= 24)
    {
        m_stateStack.push<StateWinScreen>();
        return;
    }

    // Roll the sky and ground through dusk into night as the hours pass.
    while (m_timeOfDayIndex < static_cast<int>(dayNightSchedule.size())
           && hour >= dayNightSchedule[m_timeOfDayIndex].hour)
    {
        const TimeOfDay& tod = dayNightSchedule[m_timeOfDayIndex];
        m_pSky->setTexture(*m_skyTextures[tod.skyTexture]);
        m_ground.setFillColor(sf::Color(tod.groundColor));
        ++m_timeOfDayIndex;
    }

    // Scroll the on-screen tutorial off to the left once the intro is over.
    if (m_elapsedSeconds > 10)
    {
        scrollTextLeft(m_helpText.get(), dt);
        scrollTextLeft(m_controlText.get(), dt);
        scrollTextLeft(m_powerupText.get(), dt);
    }

    // Spawn enemies on a randomised cadence (tighter while boosted).
    m_timeUntilEnemySpawn -= dt;
    if (m_timeUntilEnemySpawn < 0.0f)
    {
        constexpr float intervals[4] = {0.f, -.5f, 1.f, .25f};
        m_timeUntilEnemySpawn = enemySpawnInterval + intervals[roll(4)];
        if (m_boostActive)
            m_timeUntilEnemySpawn /= 2;

        auto pEnemy = std::make_unique<Enemy>();
        pEnemy->setPosition(sf::Vector2f(1000, ZERO_Y));
        if (pEnemy->init())
            m_enemies.push_back(std::move(pEnemy));
    }

    m_timeUntilPowerupSpawn -= dt;
    if (m_timeUntilPowerupSpawn < 0.0f)
    {
        m_timeUntilPowerupSpawn = powerupSpawnInterval;
        auto pPowerup = std::make_unique<Powerup>();
        pPowerup->setPosition(sf::Vector2f(1000, ZERO_Y));
        if (pPowerup->init())
            m_powerups.push_back(std::move(pPowerup));
    }

    // Pause on a fresh Escape press, ignoring the key being held down.
    const bool isPauseKeyPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
    m_hasPauseKeyBeenReleased |= !isPauseKeyPressed;
    if (m_hasPauseKeyBeenReleased && isPauseKeyPressed)
    {
        m_hasPauseKeyBeenReleased = false;
        m_stateStack.push<StatePaused>();
    }

    m_pPlayer->update(dt);
    for (const auto& pPowerup : m_powerups)
        pPowerup->update(dt, m_boostActive);
    for (const auto& pEnemy : m_enemies)
        pEnemy->update(dt, m_boostActive);

    m_boostTimer -= dt;
    if (m_boostTimer <= 0)
    {
        m_boostActive = false;
        m_boostTimer = 0;
    }

    const sf::Vector2f playerPos = m_pPlayer->getPosition();

    // Stomping an enemy from above pops it and bounces the player (higher if Space
    // is held); any other contact is fatal unless a boost is shielding the player.
    bool playerDied = false;
    for (const auto& pEnemy : m_enemies)
    {
        const sf::FloatRect bounds = pEnemy->getGlobalBounds();
        if (pointToBoxDistance(playerPos, bounds) > playerEnemyHitbox)
            continue;

        if (playerPos.y < bounds.position.y)
        {
            pEnemy->setActive(false);
            const bool chargedJump = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
            m_pPlayer->bounce(chargedJump ? Player::jumpVelocity : stompBounceVelocity);
        }
        else if (m_boostTimer <= 0)
        {
            playerDied = true;
            break;
        }
    }

    // Collecting a powerup grants a temporary speed boost and invulnerability.
    for (const auto& pPowerup : m_powerups)
    {
        if (!pPowerup->isActive())
            continue;
        if (pointToBoxDistance(playerPos, pPowerup->getGlobalBounds()) <= playerPowerupHitbox)
        {
            pPowerup->setActive(false);
            m_boostActive = true;
            m_boostTimer = boostDuration;
        }
    }

    if (playerDied)
        m_stateStack.popDeferred();
}

void StatePlaying::render(sf::RenderTarget& target) const
{
    target.draw(*m_pSky);
    target.draw(m_ground);
    target.draw(*m_clockText);
    target.draw(*m_helpText);
    target.draw(*m_controlText);
    target.draw(*m_powerupText);
    for (const auto& pPowerup : m_powerups)
        pPowerup->render(target);
    for (const auto& pEnemy : m_enemies)
        pEnemy->render(target);
    m_pPlayer->render(target);
}
