#include "app/renderer.hpp"
#include "app/image.hpp"

Renderer::Renderer(Window& window, FontManager& fontManager)
    : fontManager_(&fontManager), window_(window) {
    // TODO  create renderer
    if (!renderer_) {
        Assert(renderer_ != nullptr, "renderer create failed");
    }
}

Renderer::~Renderer() {
}

Renderer::Renderer(Renderer&& renderer) {
    swap(*this, renderer);
}

Renderer& Renderer::operator=(Renderer&& o) {
    if (&o != this) {
        swap(*this, o);
        o.renderer_ = nullptr;
    }
    return *this;
}

void Renderer::Present() {
    SDL_GL_SwapWindow(window_.Raw());
}

void Renderer::Clear(const Color& color) {
    GL_CALL(glClearColor(color.r, color.g, color.b, color.a));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT));
}

void Renderer::DrawLine(const math::Vector3& p1, const math::Vector3& p2) {
    // TODO not finish
}