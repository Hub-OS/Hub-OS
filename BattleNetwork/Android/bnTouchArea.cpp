//
// Created by Bountyhunter on 4/18/2019.
//
#include "bnTouchArea.h"
#include "../bnLogger.h"
#include "../bnEngine.h"

std::vector<int> TouchArea::m_touches; // We want to be aware of what touches are already owned
std::vector<TouchArea*> TouchArea::m_instances;

TouchArea::TouchArea(const sf::IntRect&& source) {
    m_area = std::move(source);
    m_touchIndex = 0;
    m_state = TouchArea::State::RELEASED;
    m_allowExtendedRelease = false;

    // Set callbacks to empty functors
    m_onReleaseCallback = [](sf::Vector2i){};
    m_onDragCallback = [](sf::Vector2i){};
    m_onTouchCallback = [](){};
    m_onDefaultCallback = [](){};

    reset();
}

TouchArea::TouchArea(const sf::IntRect& source) {
    m_area = sf::IntRect(source); // copy
    m_touchIndex = 0;
    m_state = TouchArea::State::RELEASED;
    m_allowExtendedRelease = false;

    // Set callbacks to empty functors
    m_onReleaseCallback = [](sf::Vector2i){};
    m_onDragCallback = [](sf::Vector2i){};
    m_onTouchCallback = [](){};
    m_onDefaultCallback = [](){};

    reset();
}

TouchArea::~TouchArea() {
    releaseTouch();
}

void TouchArea::reset() {
    m_touchStart = m_touchEnd = sf::Vector2i();
}

void TouchArea::releaseTouch() {
    //TouchArea::m_touches.erase(std::remove(TouchArea::m_touches.begin(), TouchArea::m_touches.end(), m_touchIndex), TouchArea::m_touches.end());
    //m_touchIndex = m_touches.size() > 0 ? *m_touches.rbegin() : 0;
    m_touches.clear();
    m_touchIndex = 0;
}

void TouchArea::privPoll() {
    if(m_state == TouchArea::State::RELEASED) {
        m_onDefaultCallback();
        m_state = TouchArea::State::DEFAULT;
    }

    if(m_state == TouchArea::State::DEFAULT) {
        unsigned int nextFinger = unsigned int(m_touches.size());
        if(sf::Touch::isDown(nextFinger)) {
            // Check to see if in rectangle
            sf::Vector2i touchPosition = sf::Touch::getPosition(nextFinger, *ENGINE.GetWindow());
            sf::Vector2f coords = ENGINE.GetWindow()->mapPixelToCoords(touchPosition, ENGINE.GetDefaultView());
            sf::Vector2i iCoords = sf::Vector2i((int)coords.x, (int)coords.y);
            touchPosition = iCoords;

            if(m_area.contains(touchPosition)) {
                m_touchIndex = nextFinger;
                m_touches.push_back(m_touchIndex);
                m_state = TouchArea::State::TOUCHED;
                m_touchStart = touchPosition;
                m_onTouchCallback();
            }
        }
    } else if(m_state == TouchArea::State::TOUCHED || m_state == TouchArea::DRAGGING) {
        if(sf::Touch::isDown(m_touchIndex)) {
            // Check to see if we are still within the rectangle
            sf::Vector2i touchPosition = sf::Touch::getPosition(m_touchIndex, *ENGINE.GetWindow());
            sf::Vector2f coords = ENGINE.GetWindow()->mapPixelToCoords(touchPosition, ENGINE.GetDefaultView());
            sf::Vector2i iCoords = sf::Vector2i((int)coords.x, (int)coords.y);
            touchPosition = iCoords;

            // If we are outside rectangle and have extended enabled, continue tracking
            if (m_area.contains(touchPosition) || m_allowExtendedRelease) {
                sf::Vector2i delta = (m_touchEnd - m_touchStart);

                if(m_state == TouchArea::State::TOUCHED) {
                    if (delta.x != 0 || delta.y != 0) {
                        // Set state to dragging and fire first callback
                        m_touchEnd = touchPosition;
                        m_onDragCallback(m_touchEnd - m_touchStart);
                        m_state = TouchArea::State::DRAGGING;
                    }
                } else {
                    // Update end pos
                    m_touchEnd = touchPosition;
                    m_onDragCallback(m_touchEnd - m_touchStart);
                }
            } else {
                // We are outside rectangle and have not released, toggle default callback
                releaseTouch();
                reset();
                m_onDefaultCallback();
                m_state = TouchArea::State::DEFAULT;
            }
        } else {
            // If we are outside rectangle and have extended enabled, fire release
            if (m_area.contains(m_touchEnd) || m_allowExtendedRelease) {
                // The touch has been released, toggle release callback based on last known position
                m_onReleaseCallback(m_touchEnd - m_touchStart);
                reset();
                m_state = TouchArea::State::RELEASED;
            } else {
                // We are outside rectangle and have released, toggle default callback
                reset();
                m_onDefaultCallback();
                m_state = TouchArea::State::DEFAULT;
            }
            releaseTouch();
        }
    }
}

void TouchArea::enableExtendedRelease(bool enable) {
    m_allowExtendedRelease = enable;
}

const TouchArea::State TouchArea::getState() const {
    return m_state;
}

const int TouchArea::getTouchIndex() const {
    return m_touchIndex;
}

void TouchArea::onRelease(std::function<void(sf::Vector2i)> callback) {
    m_onReleaseCallback = callback;
}

void TouchArea::onDrag(std::function<void(sf::Vector2i)> callback) {
    m_onDragCallback = callback;
}


void TouchArea::onTouch(std::function<void()> callback) {
    m_onTouchCallback = callback;
}

void TouchArea::onDefault(std::function<void()> callback) {
    m_onDefaultCallback = callback;
}

TouchArea& TouchArea::create(const sf::IntRect&& source) {
    TouchArea* instance = new TouchArea(std::move(source));
    TouchArea::m_instances.push_back(instance);
    return *instance;
}

TouchArea& TouchArea::create(const sf::IntRect& source) {
    TouchArea* instance = new TouchArea(source);
    TouchArea::m_instances.push_back(instance);
    return *instance;
}

void TouchArea::poll() {
    auto pos = sf::Touch::getPosition(0);

    if(sf::Touch::isDown(0))
        Logger::Log("touch position was " + std::to_string(pos.x) + ", " + std::to_string(pos.y));

    for(auto t : TouchArea::m_instances) {
        t->privPoll();
    }
}

void TouchArea::free() {
    for(int i = 0; i < TouchArea::m_instances.size(); i++) {
        delete TouchArea::m_instances[i];
    }

    TouchArea::m_instances.clear();
}