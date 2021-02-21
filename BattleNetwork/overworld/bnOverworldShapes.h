#pragma once

#include <vector>
#include <memory>
#include <optional>
#include <tuple>
#include "bnXML.h"

namespace Overworld {
  class Shape {
  public:
    static std::optional<std::unique_ptr<Shape>> From(const XMLElement& objectElement);

    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    float GetRotation() const { return rotation; }

    // point is within the shape or on an edge
    virtual bool Intersects(float x, float y) const = 0;

    virtual ~Shape() = default;

  protected:
    Shape() = default;
    float x, y, width, height, rotation;
  };

  class Point : public Shape {
  public:
    Point(float x, float y);
    bool Intersects(float x, float y) const override;
  };

  class Rect : public Shape {
  public:
    Rect(float x, float y, float width, float height, float rotation = 0.0f);
    bool Intersects(float x, float y) const override;
  };

  class Ellipse : public Shape {
  public:
    Ellipse(float x, float y, float width, float height, float rotation = 0.0f);
    bool Intersects(float x, float y) const override;
  };

  class Polygon : public Shape {
  public:
    Polygon(float x, float y, float rotation = 0.0f);

    void AddPoint(float x, float y);

    bool Intersects(float x, float y) const override;

  private:
    std::vector<std::tuple<float, float>> points;
    float smallestX, smallestY, largestX, largestY;
  };
}
