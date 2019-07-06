//
// Created by Bountyhunter on 4/18/2019.
//

#ifndef ANDROID_TOUCHAREA_H
#define ANDROID_TOUCHAREA_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>
#include "bnEngine.h"

class TouchArea {
public:
    enum State : int {
        DEFAULT,
        TOUCHED,
        DRAGGING,
        RELEASED
    };

private:
    State m_state;
    sf::IntRect m_area;
    sf::Vector2i m_touchStart;
    sf::Vector2i m_touchEnd;
    bool m_allowExtendedRelease;
    unsigned int m_touchIndex; // We want ownership of touches. 1 touch per 1 area.

    static std::vector<int> m_touches; // We want to be aware of what touches are already owned

    static std::vector<TouchArea*> m_instances;

    std::function<void(sf::Vector2i)> m_onReleaseCallback;
    std::function<void()> m_onTouchCallback;
    std::function<void(sf::Vector2i)> m_onDragCallback;
    std::function<void()> m_onDefaultCallback;

    explicit TouchArea(const sf::IntRect&& source);
    explicit TouchArea(const sf::IntRect&  source);

    void reset();
    void releaseTouch();
    void privPoll();

public:
    TouchArea() = delete;
    TouchArea(const TouchArea& rhs) = delete;
    ~TouchArea();

    void enableExtendedRelease(bool enable=true);
    void onRelease(std::function<void(sf::Vector2i)> callback);
    void onTouch(std::function<void()> callback);
    void onDrag(std::function<void(sf::Vector2i)> callback);
    void onDefault(std::function<void()> callback);

    static TouchArea& create(const sf::IntRect&  source);
    static TouchArea& create(const sf::IntRect&& source);
    static void free();
    static void poll();

    const State getState() const;
    const int getTouchIndex() const;
};

#endif //ANDROID_TOUCHAREA_H
