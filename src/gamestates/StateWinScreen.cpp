#include "StateWinScreen.h"
#include "StateStack.h"
#include "ResourceManager.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

StateWinScreen::StateWinScreen(StateStack& stateStack)
    : m_stateStack(stateStack)
{
    
}

bool StateWinScreen::init()
{
    const sf::Font* pFont = ResourceManager::getOrLoadFont("Lavigne.ttf");
    if (pFont == nullptr)
        return false;

    m_pText = std::make_unique<sf::Text>(*pFont);
    if (!m_pText)
        return false;
    m_clockText = std::make_unique<sf::Text>(*pFont);
    if (!m_clockText)
        return false;

    m_clockText->setString("24:00");
    m_clockText->setCharacterSize(200);
    m_clockText->setStyle(sf::Text::Bold);
    sf::FloatRect localBounds = m_clockText->getLocalBounds();
    m_clockText->setOrigin({localBounds.size.x / 60.0f, localBounds.size.y / 60.0f});

    m_pText->setString("You defeated the darkness!");
    m_pText->setStyle(sf::Text::Bold);
    localBounds = m_pText->getLocalBounds();
    m_pText->setOrigin({localBounds.size.x / 2.0f, localBounds.size.y / 2.0f});

    return true;
}

void StateWinScreen::update(float dt)
{
    (void)dt;
    // Terminal state: the player has won, so there is nothing left to update.
}

void StateWinScreen::render(sf::RenderTarget& target) const
{
    m_pText->setPosition({target.getSize().x * 0.5f, target.getSize().y * 0.5f});
    target.draw(*m_pText);
    target.draw(*m_clockText);
}
