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

    float GetX() { return x; }
    float GetY() { return y; }
    float GetWidth() { return width; }
    float GetHeight() { return height; }

    // point is within the shape or on an edge
    virtual bool Intersects(float x, float y) = 0;

    virtual ~Shape() = default;

  protected:
    Shape() = default;
    float x, y, width, height;
  };

  class Point : public Shape {
  public:
    Point(float x, float y);
    bool Intersects(float x, float y) override;
  };

  class Rect : public Shape {
  public:
    Rect(float x, float y, float width, float height);
    bool Intersects(float x, float y) override;
  };

  class Ellipse : public Shape {
  public:
    Ellipse(float x, float y, float width, float height);
    bool Intersects(float x, float y) override;
  };

  class Polygon : public Shape {
  public:
    Polygon(float x, float y);

    void AddPoint(float x, float y);

    bool Intersects(float x, float y) override;

  private:
    std::vector<std::tuple<float, float>> points;
    float smallestX, smallestY, largestX, largestY;
  };
}
