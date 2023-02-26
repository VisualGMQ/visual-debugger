#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"
#include "app/app.hpp"
#include "net.hpp"
#include <variant>
#include "draw_commands.hpp"

enum class NetState {
    ClientClosing,
    Accepting,
    Connected,
};

struct NetContext {
    std::unique_ptr<net::Net> net = nullptr;
    net::Socket* serverSock = nullptr;
    std::unique_ptr<net::Socket> clientSock = nullptr;
    std::string clientAddr = "";
    NetState state = NetState::ClientClosing;
};

using Cmd = std::variant<debugger::LineCmd, debugger::PlaneCmd, debugger::PointCmd>;

struct CmdContext {
    std::vector<Cmd> cmds;
};

void NuklearStartupSystem(ecs::Commands& cmd, ecs::Resources resources) {
    auto& window = resources.Get<Window>();
    auto& renderer = resources.Get<Renderer>();
    auto ctx = nk_sdl_init(window.Raw(), renderer.Raw());

    /* Load Fonts: if none of these are loaded a default font will be used  */
    {
        float font_scale = 1;
        struct nk_font_atlas *atlas;
        struct nk_font_config config = nk_font_config(0);
        struct nk_font *font;

        /* set up the font atlas and add desired font; note that font sizes are
         * multiplied by font_scale to produce better results at higher DPIs */
        nk_sdl_font_stash_begin(&atlas);
        font = nk_font_atlas_add_default(atlas, 13 * font_scale , &config);
        nk_sdl_font_stash_end();

        /* this hack makes the font appear to be scaled down to the desired
         * size and is only necessary when font_scale > 1 */
        font->handle.height /= font_scale;
        /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
        nk_style_set_font(ctx, &font->handle);
    }

    cmd.SetResource<nk_context*>(std::move(ctx));
    cmd.SetResource<Transform>({});
}

std::unique_ptr<net::Socket> AcceptClient(net::Socket* socket, sockaddr& addr) {
    int iResult;

    std::unique_ptr<net::Socket> sock;
    // do {
        auto acceptResult = socket->Accept(addr);
        sock = std::move(acceptResult.value);
        iResult = acceptResult.result;
    // } while(iResult == WSAEWOULDBLOCK);
    if (iResult == WSAEWOULDBLOCK) {
        return nullptr;
    }

    return std::move(sock);
}


void StartupSystem(ecs::Commands& cmd, ecs::Resources resources) {
    auto& window = resources.Get<Window>();
    auto& renderer = resources.Get<Renderer>();

    auto netInst = net::Init();
    auto result = net::AddrInfoBuilder::CreateTCP("localhost", 12757).Build();
    auto socket = netInst->CreateSocket(result.value);
    socket->Bind();
    LOGI("listening...");
    socket->Listen();

    socket->EnableNoBlock();

    cmd.SetResource<NetContext>(NetContext{std::move(netInst), socket, std::make_unique<net::Socket>()});
    cmd.SetResource<CmdContext>(CmdContext{});

    std::ofstream file("./debugger-log.txt");
    cmd.SetResource<std::ofstream>(std::move(file));
}

bool IsClientValid(const std::unique_ptr<net::Socket>& client) {
    return client && client->Valid();
}

void UpdateSystem(ecs::Commands& cmd, ecs::Querier, ecs::Resources resources, ecs::Events&) {
    auto& netContext = resources.Get<NetContext>();
    auto& exitTrigger = resources.Get<ExitTrigger>();
    auto& cmdContext = resources.Get<CmdContext>();
    auto& nkcontext = resources.Get<nk_context*>();
    auto& transform = resources.Get<Transform>();
    auto& mouse = resources.Get<Mouse>();
    auto& outputFile = resources.Get<std::ofstream>();

    if (nk_begin(
            nkcontext, "Console", nk_rect(0, 0, 400, 200),
            NK_WINDOW_SCALABLE | NK_WINDOW_MOVABLE | NK_WINDOW_MINIMIZABLE)) {
        nk_layout_row_dynamic(nkcontext, 30, 1);

        std::string str = "Client State: ";
        if (IsClientValid(netContext.clientSock)) {
            str += "Connected";
        } else {
            str += "Closing";
        }
        nk_label(nkcontext, str.c_str(), NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_CENTERED);

        str = "Start Accept";
        if (IsClientValid(netContext.clientSock)) {
            str = "Close Connection";
        }
        if (netContext.state == NetState::Accepting) {
            str = "Accepting...";
        }
        if (nk_button_label(nkcontext, str.c_str())) {
            if (IsClientValid(netContext.clientSock)) {
                netContext.clientSock->Close();
                netContext.state = NetState::ClientClosing;
            } else {
                netContext.state = NetState::Accepting;
            }
        }
        if (IsClientValid(netContext.clientSock)) {
            nk_label(nkcontext,
                     ("client address: " + netContext.clientAddr).c_str(),
                     NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_CENTERED);
        }
        nk_label(nkcontext,
                 ("coordination: " + std::to_string(transform.position.x) +
                  ", " + std::to_string(transform.position.y))
                     .c_str(),
                 NK_TEXT_ALIGN_LEFT | NK_TEXT_ALIGN_CENTERED);
        if (nk_button_label(nkcontext, "Clear")) {
            cmdContext.cmds.clear();
        }
    }
    nk_end(nkcontext);

    if (netContext.state == NetState::Accepting) {
        sockaddr addr;
        netContext.clientSock = AcceptClient(netContext.serverSock, addr);
        netContext.clientAddr = inet_ntoa(*(in_addr*)&addr);
    }

    if (!IsClientValid(netContext.clientSock)) {
        return;
    }

    // process datas:

    netContext.state = NetState::Connected;

    char buf[4096] = {0};
    int len = netContext.clientSock->Recv(buf, sizeof(buf));
    int code = net::GetLastErrorCode();
    if (code == WSAEWOULDBLOCK || code == WSAETIMEDOUT) {
        return;
    }
    if (len <= 0) {
        if (netContext.clientSock->Close() != 0) {
            LOGE("close failed: ", net::GetLastError());
        }
        netContext.state = NetState::ClientClosing;

        if (len < 0) {
            LOGE("net error: ", net::Error2Str(code), ", ", code);
        } else {
            LOGE("client closed");
        }
        return;
    }

    debugger::Uint8Ptr ptr = (debugger::Uint8Ptr)buf;

    debugger::CmdType cmdType;
    debugger::GetBuf(ptr, &cmdType, sizeof(cmdType));

    switch (cmdType) {
        case debugger::CmdType::Line: {
            auto cmd = debugger::LineCmd::Deserialize(ptr);
            if (cmdContext.cmds.empty()) {
                transform.position.Set(-cmd.a.x, -cmd.a.y);
            }
            cmdContext.cmds.push_back(cmd);
            LOGI("recived LineCmd, a: ", cmd.a, "\n\tb:", cmd.b,
                "\n\tbulge:", cmd.bulge,
                 "\n\tcolor:", cmd.color);
            outputFile << "line" << std::endl;
            cmd.a.Output2File(outputFile);
            cmd.b.Output2File(outputFile);
            outputFile << " " << cmd.bulge << std::endl;
        } break;
        case debugger::CmdType::Point: {
            auto cmd = debugger::PointCmd::Deserialize(ptr);
            if (cmdContext.cmds.empty()) {
                transform.position.Set(-cmd.point.x, -cmd.point.y);
            }
            cmdContext.cmds.push_back(cmd);
            LOGI("recived PointCmd, pt: ", cmd.point, "\t\ncolor: ", cmd.color);
            outputFile << "point" << std::endl;
            cmd.point.Output2File(outputFile);
            outputFile << std::endl;
        } break;
        case debugger::CmdType::Plane: {
            auto cmd = debugger::PlaneCmd::Deserialize(ptr);
            if (cmd.vertices.empty() || cmd.bulges.empty()) {
                LOGE("invalid plane, vertices or bulges are 0");
                return;
            }
            if (cmdContext.cmds.empty()) {
                transform.position.Set(-cmd.vertices[0].x, -cmd.vertices[0].y);
            }
            cmdContext.cmds.push_back(cmd);
            LOGI("recived PlaneCmd, pts:");
            outputFile << "plane" << std::endl << cmd.vertices.size() << std::endl;
            for (auto& pt : cmd.vertices) {
                LOGI("\n\t", pt);
                pt.Output2File(outputFile);
            }
            outputFile << std::endl;
            outputFile << std::endl << cmd.bulges.size() << std::endl;
            for (auto& bulge : cmd.bulges) {
                LOGI("\n\t", bulge);
                outputFile << bulge << " ";
            }
            outputFile << std::endl;

            LOGI("\n\tColor: ", cmd.color);
        } break;
        default:
            LOGI("recived unknown cmd: ", static_cast<int>(cmdType));
    }
}

constexpr int PointSize = 4 / 2;

struct Visitor final {
    Visitor(Renderer& renderer, const Transform& transform, const math::Vector2& size)
        : renderer_(renderer), transform_(transform), size_(size) {}

    void operator()(const debugger::PointCmd& cmd) {
        renderer_.SetDrawColor({cmd.color.r, cmd.color.g, cmd.color.b});
        auto center = (math::Vector2(cmd.point.x, cmd.point.y) + transform_.position) * transform_.scale + size_ / 2.0f;
        renderer_.FillRect(
            math::Rect{center.x - PointSize,
                       center.y - PointSize,
                       PointSize * 2.0f, PointSize * 2.0f});
    }

    void operator()(const debugger::LineCmd& cmd) {
        renderer_.SetDrawColor({cmd.color.r, cmd.color.g, cmd.color.b});
        renderer_.DrawLine(
            (math::Vector2{cmd.a.x, cmd.a.y} + transform_.position) *
                transform_.scale + size_ / 2.0f,
            (math::Vector2{cmd.b.x, cmd.b.y} + transform_.position) *
                transform_.scale + size_ / 2.0f);
    }

    void operator()(const debugger::PlaneCmd& cmd) {
        renderer_.SetDrawColor({cmd.color.r, cmd.color.g, cmd.color.b});
        for (int i = 0; i < cmd.vertices.size(); i++) {
            auto& pt = cmd.vertices[i];
            auto& nextPt = cmd.vertices[(i + 1) % cmd.vertices.size()];

            renderer_.DrawLine(
                (math::Vector2{pt.x, pt.y} + transform_.position) *
                    transform_.scale + size_ / 2.0f,
                (math::Vector2{nextPt.x, nextPt.y} + transform_.position) *
                    transform_.scale + size_ / 2.0f);
        }
    }

    Renderer& renderer_;
    const Transform& transform_;
    const math::Vector2& size_;
};

void RenderSystem(ecs::Commands& cmd, ecs::Querier, ecs::Resources resources,
                  ecs::Events&) {
    auto& renderer = resources.Get<Renderer>();
    auto& cmdContext = resources.Get<CmdContext>();
    auto& transform = resources.Get<Transform>();
    auto& window = resources.Get<Window>();

    for (auto& cmd : cmdContext.cmds) {
        std::visit(Visitor{renderer, transform, window.GetSize()}, cmd);
    }
}

void NuklearRenderSystem(ecs::Commands& cmd, ecs::Querier,
                         ecs::Resources resources, ecs::Events&) {
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}

void EventHandleSystem(ecs::Commands& cmd, ecs::Querier, ecs::Resources resources,
                  ecs::Events& events) {
    auto& transform = resources.Get<Transform>();
    auto& mouse = resources.Get<Mouse>();
    auto& ctx = resources.Get<nk_context*>();

    if (nk_item_is_any_active(ctx)) { return; }

    auto wheelReader = events.Reader<SDL_MouseWheelEvent>();
    if (wheelReader.Has()) {
        auto& wheel = wheelReader.Read();
        if (wheel.preciseY > 0) {
            transform.scale *= 1.01;
        } else if (wheel.preciseY < 0) {
            transform.scale *= 0.99;
        }
    }

    auto motionReader = events.Reader<SDL_MouseMotionEvent>();
    if (motionReader.Has()) {
        auto& motion = motionReader.Read();
        if (mouse.LeftBtn().IsPressing()) {
            transform.position.x += motion.xrel * 1.0 / transform.scale.x;
            transform.position.y += motion.yrel * 1.0 / transform.scale.y;
        }
    }
}

class VisualDebugger : public App {
public:
    VisualDebugger() {
        auto& world = GetWorld();
        world.AddPlugins<DefaultPlugins>()
            .AddStartupSystem(NuklearStartupSystem)
            .AddStartupSystem(StartupSystem)
            .AddSystem(UpdateSystem)
            .AddSystem(EventHandleSystem)
            .AddSystem(RenderSystem)
            .AddSystem(NuklearRenderSystem)
            .AddSystem(ExitTrigger::DetectExitSystem)
            .SetResource<ExtraEventHandler>(ExtraEventHandler(
                [&](const SDL_Event& event) {
                    nk_sdl_handle_event(const_cast<SDL_Event*>(&event));
                },
                [](ecs::Resources resources) {
                    nk_input_begin(resources.Get<nk_context*>());
                },
                [](ecs::Resources resources) {
                    nk_input_end(resources.Get<nk_context*>());
                }));
    }
};

RUN_APP(VisualDebugger)