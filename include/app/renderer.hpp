#pragma once

#include "app/font.hpp"
#include "app/fwd.hpp"
#include "app/window.hpp"
#include "core/math.hpp"
#include "core/pch.hpp"
#include "app/transform.hpp"
#include "app/sprite.hpp"
#include "glad/glad.h"
#include "app/glhelper.hpp"

class Image;
class ImageManager;

class Renderer final {
public:
    friend class Image;
    friend class DefaultPlugins;

    Renderer(Window& window, FontManager& fontManager);
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&);
    ~Renderer();

    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&);

    void DrawLine(const math::Vector3&, const math::Vector3&);

    void Present();
    void Clear(const Color&);

    SDL_Renderer* Raw() const { return renderer_; }

private:
    SDL_Renderer* renderer_ = nullptr;
    FontManager* fontManager_ = nullptr;
    ImageManager* imageManager_ = nullptr;
    Window& window_;

    // use Copy-And-Swap-Idiom:
    // https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom
    friend void swap(Renderer& lhs, Renderer& rhs) noexcept {
        std::swap(lhs.renderer_, rhs.renderer_);
        std::swap(lhs.fontManager_, rhs.fontManager_);
        std::swap(lhs.imageManager_, rhs.imageManager_);
    }

    void drawTexture(SDL_Texture* texture, int rawW, int rawH,
                     const math::Rect& region, const math::Vector2& size,
                     const Transform& transform, const math::Vector2& anchor,
                     Flip flip);
};