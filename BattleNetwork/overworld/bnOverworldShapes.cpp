#include "bnOverworldShapes.h"

namespace Overworld {
  Point::Point(float x, float y) {
    this->x = x;
    this->y = y;
    width = 0;
    height = 0;
  }

  bool Point::Intersects(float x, float y) {
    // points have no size, no possible intersection
    return false;
  }

  Rect::Rect(float x, float y, float width, float height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
  }

  bool Rect::Intersects(float x, float y) {
    return
      x >= this->x &&
      y >= this->y &&
      x <= this->x + width &&
      y <= this->y + height;
  }

  Ellipse::Ellipse(float x, float y, float width, float height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
  }

  bool Ellipse::Intersects(float x, float y) {
    auto distanceX = x - this->x;
    // add half height as tiled centers ellipse at the top
    auto distanceY = y - (this->y + this->height / 2.0f);

    auto radiusX = this->width / 2.0f;
    auto radiusY = this->height / 2.0f;

    // convert distances to relative positions on a unit circle
    auto unitX = distanceX / radiusX;
    auto unitY = distanceY / radiusY;

    // unit circle has a radius of 1
    // so we can just test for <= 1 to see if this point is within the circle
    return unitX * unitX + unitY * unitY <= 1;
  }

  Polygon::Polygon(float x, float y) {
    this->x = x;
    this->y = y;
    this->width = 0;
    this->height = 0;
  }

  void Polygon::AddPoint(float x, float y) {
    points.emplace_back(std::make_tuple(x, y));

    if (x < smallestX) {
      smallestX = x;
    }

    if (x > largestX) {
      largestX = x;
    }

    if (y < smallestY) {
      smallestY = y;
    }

    if (y > largestY) {
      largestY = y;
    }

    width = largestX - smallestX;
    height = largestY - smallestY;
  }

  // shoot a ray to the right and count edge intersections
  // even = outside, odd = inside
  bool Polygon::Intersects(float x, float y) {
    uint intersections = 0;

    std::tuple<float, float> lastPoint;
    bool initializedFirstPoint = false;

    for (auto& point : points) {
      if (!initializedFirstPoint)
      {
        lastPoint = point;
        initializedFirstPoint = true;
        continue;
      }

      auto Ax = std::get<0>(lastPoint) + this->x;
      auto Ay = std::get<1>(lastPoint) + this->y;
      auto Bx = std::get<0>(point) + this->x;
      auto By = std::get<1>(point) + this->y;

      auto run = Bx - Ax;

      if (run == 0) {
        // column line

        // make sure y is between these points
        auto yIsWithin = (y < Ay&& y > By) || (y > Ay && y < By);

        if (x < Ax && yIsWithin) {
          // column is to the right and y is within
          intersections += 1;
        }
      }
      else {
        // y = slope * x + yIntercept
        auto rise = By - Ay;
        auto slope = rise / run;
        auto yIntercept = Ay - slope * Ax;

        // find an intersection at y
        // algebra: x = (y - yIntercept) / slope
        auto intersectionX = (y - yIntercept) / slope;

        // make sure x is between these points, y is implied through above calculation
        auto xIsWithin = (intersectionX < Ax&& intersectionX > Bx) || (intersectionX > Ax && intersectionX < Bx);

        if (xIsWithin) {
          intersections += 1;
        }
      }


      lastPoint = point;
    }

    // odd = inside
    return intersections % 2 == 1;
  }

  std::optional<std::unique_ptr<Shape>> Shape::From(const XMLElement& objectElement) {
    if (objectElement.name != "object") {
      return {};
    }

    auto x = objectElement.GetAttributeFloat("x");
    auto y = objectElement.GetAttributeFloat("y");
    auto width = objectElement.GetAttributeFloat("width");
    auto height = objectElement.GetAttributeFloat("height");
    auto childName = objectElement.children.size() > 0 ? objectElement.children[0].name : "";

    if (childName == "polygon") {
      auto& polygonElement = objectElement.children[0];
      auto pointsString = polygonElement.GetAttribute("points");

      auto polygon = std::make_unique<Overworld::Polygon>(x, y);

      auto sliceStart = 0;
      float x;

      for (auto i = 0; i <= pointsString.size(); i++) {
        switch (pointsString[i]) {
        case ',': {
          try {
            x = stof(pointsString.substr(sliceStart, i));
          }
          catch (std::exception& e) {
            x = 0;
          }

          sliceStart = i + 1;
          break;
        }
        case '\0':
        case ' ': {
          float y;

          try {
            y = stof(pointsString.substr(sliceStart, i));
          }
          catch (std::exception& e) {
            y = 0;
          }

          polygon->AddPoint(x, y);
          break;
        }
        default:
          break;
        }
      }

      return polygon;
    }
    else if (width == 0.0f && height == 0.0f) {
      return std::make_unique<Overworld::Point>(x, y);
    }
    else if (childName == "ellipse") {
      return std::make_unique<Overworld::Ellipse>(x, y, width, height);
    }
    else {
      return std::make_unique<Overworld::Rect>(x, y, width, height);
    };

    return {};
  }
}
