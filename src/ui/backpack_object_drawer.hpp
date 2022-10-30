#pragma once

#include "components/backpack.hpp"
#include "grid_panel.hpp"
#include "others/config.hpp"
#include "others/data.hpp"

class BackpackObjectDrawer {
public:
    void operator()(component::GridPanel*, const engine::Rect& rect, int x, int y);

private:
    void drawObject(component::Backpack*, component::GridPanel*, const engine::Image&, const engine::Vec2& pos);
    void drawObjectNum(engine::Font* font, int number, const engine::Vec2& pos, const engine::Color&);
};