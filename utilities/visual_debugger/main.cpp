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
}

bool IsClientValid(const std::unique_ptr<net::Socket>& client) {
    return client && client->Valid();
}

void UpdateSystem(ecs::Commands& cmd, ecs::Querier, ecs::Resources resources, ecs::Events&) {
    auto& netContext = resources.Get<NetContext>();
    auto& exitTrigger = resources.Get<ExitTrigger>();
    auto& cmdContext = resources.Get<CmdContext>();
    auto& nkcontext = resources.Get<nk_context*>();

    if (nk_begin(
            nkcontext, "Console", nk_rect(0, 0, 200, 400),
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

    netContext.state = NetState::Connected;

    char buf[4096] = {0};
    int len = netContext.clientSock->Recv(buf, sizeof(buf));
    int code = net::GetLastErrorCode();
    if (code == WSAEWOULDBLOCK || code == WSAETIMEDOUT) {
        return;
    }
    if (len <= 0) {
        netContext.clientSock->Close();
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

    LOGI("recived: ", len, " - ", buf);
    switch (cmdType) {
        case debugger::CmdType::Line: {
            auto cmd = debugger::LineCmd::Deserialize(ptr);
            cmdContext.cmds.push_back(cmd);
            LOGI("recived LineCmd, a: ", cmd.a, "\n\tb:", cmd.b,
                 "\n\tcolor:", cmd.color);
        } break;
        case debugger::CmdType::Point: {
            auto cmd = debugger::PointCmd::Deserialize(ptr);
            cmdContext.cmds.push_back(cmd);
            LOGI("recived PointCmd, pt: ", cmd.point, "\t\ncolor: ", cmd.color);
        } break;
        case debugger::CmdType::Plane: {
            auto cmd = debugger::PlaneCmd::Deserialize(ptr);
            cmdContext.cmds.push_back(cmd);
            LOGI("recived PlaneCmd, pts:");
            for (auto& pt : cmd.vertices) {
                LOGI("\n\t", pt);
            }
            LOGI("\n\tColor: ", cmd.color);
        } break;
        default:
            LOGI("recived unknown cmd: ", static_cast<int>(cmdType));
    }
}

constexpr int PointSize = 4 / 2;

struct Visitor final {
    Visitor(Renderer& renderer) : renderer_(renderer) {}

    void operator()(const debugger::PointCmd& cmd) {
        renderer_.SetDrawColor({cmd.color.r, cmd.color.g, cmd.color.b});
        renderer_.FillRect(math::Rect{cmd.point.x - PointSize,
                                      cmd.point.y - PointSize, PointSize * 2.0f,
                                      PointSize * 2.0f});
    }

    void operator()(const debugger::LineCmd& cmd) {
        renderer_.SetDrawColor({cmd.color.r, cmd.color.g, cmd.color.b});
        renderer_.DrawLine({cmd.a.x, cmd.a.y}, {cmd.b.x, cmd.b.y});
    }

    void operator()(const debugger::PlaneCmd& cmd) {
        renderer_.SetDrawColor({cmd.color.r, cmd.color.g, cmd.color.b});
        for (int i = 0; i < cmd.vertices.size(); i++) {
            auto& pt = cmd.vertices[i];
            auto& nextPt = cmd.vertices[(i + 1) % cmd.vertices.size()];

            renderer_.DrawLine({pt.x, pt.y}, {nextPt.x, nextPt.y});
        }
    }

    Renderer& renderer_;
};

void RenderSystem(ecs::Commands& cmd, ecs::Querier, ecs::Resources resources,
                  ecs::Events&) {
    auto& renderer = resources.Get<Renderer>();
    auto& cmdContext = resources.Get<CmdContext>();

    for (auto& cmd : cmdContext.cmds) {
        std::visit(Visitor{renderer}, cmd);
    }
}

void NuklearRenderSystem(ecs::Commands& cmd, ecs::Querier,
                         ecs::Resources resources, ecs::Events&) {
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}

class VisualDebugger : public App {
public:
    VisualDebugger() {
        auto& world = GetWorld();
        world.AddPlugins<DefaultPlugins>()
            .AddStartupSystem(NuklearStartupSystem)
            .AddStartupSystem(StartupSystem)
            .AddSystem(UpdateSystem)
            .AddSystem(NuklearRenderSystem)
            .AddSystem(RenderSystem)
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