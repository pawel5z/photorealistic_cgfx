#include "NoBuffDrawable.hpp"

class Axes : NoBuffDrawable {
public:
    Axes(std::string vertPath, std::string fragPath);
    void draw(const Camera &camera) const override;
};
