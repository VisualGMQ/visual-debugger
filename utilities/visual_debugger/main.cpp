#include "app/app.hpp"
#include "net.hpp"
#include <variant>
#include "draw_commands.hpp"

struct NetContext {
    std::unique_ptr<net::Net> net = nullptr;
    net::Socket* serverSock = nullptr;
    std::unique_ptr<net::Socket> clienetSock = nullptr;
};

using Cmd = std::variant<debugger::LineCmd, debugger::PlaneCmd, debugger::PointCmd>;

struct CmdContext {
    std::vector<Cmd> cmds;
};

void StartupSystem(ecs::Commands& cmd, ecs::Resources resources) {
    auto& window = resources.Get<Window>();
    auto& renderer = resources.Get<Renderer>();

    auto netInst = net::Init();
    auto result = net::AddrInfoBuilder::CreateTCP("localhost", 8080).Build();
    auto socket = netInst->CreateSocket(result.value);
    socket->Bind();
    LOGI("listening...");
    socket->Listen();

    socket->EnableNoBlock();

    int iResult;

    std::unique_ptr<net::Socket> sock;

    do {
        auto acceptResult = socket->Accept();
        sock = std::move(acceptResult.value);
        iResult = acceptResult.result;
    } while(iResult == WSAEWOULDBLOCK);

    LOGI("accepted OK");

    cmd.SetResource<NetContext>(NetContext{std::move(netInst), socket, std::move(sock)});
    cmd.SetResource<CmdContext>(CmdContext{});
}

void UpdateSystem(ecs::Commands& cmd, ecs::Queryer, ecs::Resources resources, ecs::Events&) {
    auto& netContext = resources.Get<NetContext>();
    auto& exitTrigger = resources.Get<ExitTrigger>();
    auto& cmdContext = resources.Get<CmdContext>();

    if (!netContext.clienetSock->Valid()) {
        return;
    }

    char buf[4096] = {0};
    int len = netContext.clienetSock->Recv(buf, sizeof(buf));
    int code = net::GetLastErrorCode();
    if (code == WSAEWOULDBLOCK || code == WSAETIMEDOUT) {
        return;
    }
    if (len <= 0) {
        netContext.clienetSock->Close();
        netContext.serverSock->Close();
        netContext.net.reset();

        LOGE("net error: ", net::Error2Str(code));
        return;
    }

    debugger::Uint8Ptr ptr = (debugger::Uint8Ptr)buf;

    debugger::CmdType cmdType;
    debugger::GetBuf(ptr, &cmdType, sizeof(cmdType));

    LOGI("recived: ", len, " - ", buf);
    switch (cmdType) {
        case debugger::CmdType::Line:
            cmdContext.cmds.push_back(debugger::LineCmd::Deserialize(ptr));
            LOGI("recived LineCmd");
            break;
        case debugger::CmdType::Point:
            LOGI("recived PointCmd");
            cmdContext.cmds.push_back(debugger::PointCmd::Deserialize(ptr));
            break;
        case debugger::CmdType::Plane:
            LOGI("recived PlaneCmd");
            cmdContext.cmds.push_back(debugger::PlaneCmd::Deserialize(ptr));
            break;
        default:
            LOGI("recived unknown cmd: ", static_cast<int>(cmdType));
    }
}

void TestRenderSystem(ecs::Commands& cmd, ecs::Queryer, ecs::Resources resources, ecs::Events&) {
    static float x = 0;
    
    auto& renderer = resources.Get<Renderer>();
    renderer.SetDrawColor(Color{0, 255, 0});
    renderer.DrawCircle({x, 100}, 50);
    
    x += 5;
    if (x > 1024) {
        x = 0;
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

void RenderSystem(ecs::Commands& cmd, ecs::Queryer, ecs::Resources resources, ecs::Events&) {
    auto& renderer = resources.Get<Renderer>();
    auto& cmdContext = resources.Get<CmdContext>();

    for (auto& cmd : cmdContext.cmds) {
        std::visit(Visitor{renderer}, cmd);
    }
}

class VisualDebugger : public App {
public:
    VisualDebugger() {
        auto& world = GetWorld();
        world.AddPlugins<DefaultPlugins>()
            .AddStartupSystem(StartupSystem)
            .AddSystem(UpdateSystem)
            // .AddSystem(TestRenderSystem)
            .AddSystem(RenderSystem)
            .AddSystem(ExitTrigger::DetectExitSystem);
    }
};

RUN_APP(VisualDebugger)