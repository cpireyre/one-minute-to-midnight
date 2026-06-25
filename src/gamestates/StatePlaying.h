#pragma once

#include "Constants.h"
#include "IState.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "entities/Powerup.h"
#include <memory>
#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

class StatePlaying : public IState
{
public:
    StatePlaying(StateStack& stateStack);
    ~StatePlaying() = default;

    bool init() override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) const override;

private:
    static constexpr float enemySpawnInterval = 1.0f;
    static constexpr float powerupSpawnInterval = 12.0f;

    StateStack& m_stateStack;
    std::unique_ptr<Player> m_pPlayer;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<std::unique_ptr<Powerup>> m_powerups;

    std::unique_ptr<sf::Sprite> m_pSky;
    const sf::Texture* m_skyTextures[5] = {};
    sf::RectangleShape m_ground;

    std::unique_ptr<sf::Text> m_clockText;
    std::unique_ptr<sf::Text> m_helpText;
    std::unique_ptr<sf::Text> m_controlText;
    std::unique_ptr<sf::Text> m_powerupText;

    float m_timeUntilEnemySpawn = enemySpawnInterval;
    float m_timeUntilPowerupSpawn = powerupSpawnInterval;
    float m_elapsedSeconds = 0.0f;
    int m_timeOfDayIndex = 0;

    bool m_boostActive = false;
    float m_boostTimer = 0.0f;
    bool m_hasPauseKeyBeenReleased = true;
};
