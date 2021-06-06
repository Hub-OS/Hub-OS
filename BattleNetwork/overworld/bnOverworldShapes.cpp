#include "bnOverworldShapes.h"
#include <math.h>
#include <Swoosh/Ease.h>
#include "../bnLogger.h"
// using pointers to make mutation clear
static inline void rotateAround(float centerX, float centerY, float rotation, float* x, float* y) {
  if (rotation != 0.0f) {
    float relativeX = *x - centerX;
    float relativeY = *y - centerY;

    float distance = std::sqrt(relativeX * relativeX + relativeY * relativeY);
    float relativeRadians = std::atan2(relativeY, relativeX);
    float rotationRadians = rotation / 180.0f * static_cast<float>(swoosh::ease::pi);
    float radians = relativeRadians + rotationRadians;

    // set x + y to a position rotated around centerX + centerY
    *x = std::cos(radians) * distance + centerX;
    *y = std::sin(radians) * distance + centerY;
  }
}


namespace Overworld {
  Point::Point(float x, float y) {
    this->x = x;
    this->y = y;
    width = 0;
    height = 0;
  }

  bool Point::Intersects(float x, float y) const {
    // points have no size, no possible intersection
    return false;
  }

  Rect::Rect(float x, float y, float width, float height, float rotation) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->rotation = rotation;
  }

  bool Rect::Intersects(float x, float y) const {
    rotateAround(this->x, this->y, -rotation, &x, &y);

    return
      x >= this->x &&
      y >= this->y &&
      x <= this->x + width &&
      y <= this->y + height;
  }

  Ellipse::Ellipse(float x, float y, float width, float height, float rotation) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->rotation = rotation;
  }

  bool Ellipse::Intersects(float x, float y) const {
    rotateAround(this->x, this->y, -rotation, &x, &y);

    // add half size as tiled centers ellipse at the top left
    auto distanceX = x - (this->x + this->width / 2.0f);
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

  Polygon::Polygon(float x, float y, float rotation) {
    this->x = x;
    this->y = y;
    this->width = 0;
    this->height = 0;
    this->rotation = rotation;
  }

  void Polygon::AddPoint(float x, float y) {
    points.emplace_back(std::make_tuple(x, y));

    if (points.size() == 1) {
      smallestX = x;
      smallestY = y;
      largestX = x;
      largestY = y;
      return;
    }

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
  bool Polygon::Intersects(float x, float y) const {
    if (points.size() == 0) {
      return false;
    }

    rotateAround(this->x, this->y, -rotation, &x, &y);

    unsigned int intersections = 0;

    std::tuple<float, float> lastPoint = points[points.size() - 1];

    for (auto& point : points) {
      auto Ax = std::get<0>(lastPoint) + this->x;
      auto Ay = std::get<1>(lastPoint) + this->y;
      auto Bx = std::get<0>(point) + this->x;
      auto By = std::get<1>(point) + this->y;

      auto run = Bx - Ax;

      if (run == 0) {
        // column line

        // make sure y is between these points
        auto yIsWithin = (y >= Ay && y <= By) || (y >= By && y <= Ay);

        if (x < Ax && yIsWithin) {
          // column is to the right and y is within
          intersections += 1;
        }
      }
      else if ((y >= Ay && y < By) || (y >= By && y < Ay)) { // make sure y is within the line excluding the top point (avoid colliding with vertex twice)
        // y = slope * x + yIntercept
        auto rise = By - Ay;
        auto slope = rise / run;
        auto yIntercept = Ay - slope * Ax;

        // find an intersection at y
        // algebra: x = (y - yIntercept) / slope
        auto intersectionX = (y - yIntercept) / slope;

        // make sure x is between these points
        auto xIsWithin = (intersectionX >= Ax && intersectionX <= Bx) || (intersectionX >= Bx && intersectionX <= Ax);

        // make sure intersectionX is to the right
        if (xIsWithin && x <= intersectionX) {
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
    auto rotation = objectElement.GetAttributeFloat("rotation");
    auto childName = objectElement.children.size() > 0 ? objectElement.children[0].name : "";

    if (childName == "polygon") {
      auto& polygonElement = objectElement.children[0];
      auto pointsString = polygonElement.GetAttribute("points");

      auto polygon = std::make_unique<Overworld::Polygon>(x, y, rotation);

      auto sliceStart = 0;
      float x;

      for (auto i = 0; i <= pointsString.size(); i++) {
        switch (pointsString[i]) {
        case ',': {
          try {
            x = stof(pointsString.substr(sliceStart, i));
          }
          catch (std::exception&) {
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
          catch (std::exception&) {
            y = 0;
          }

          polygon->AddPoint(x, y);
          sliceStart = i + 1;
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
      return std::make_unique<Overworld::Ellipse>(x, y, width, height, rotation);
    }
    else {
      return std::make_unique<Overworld::Rect>(x, y, width, height, rotation);
    };

    return {};
  }
}
