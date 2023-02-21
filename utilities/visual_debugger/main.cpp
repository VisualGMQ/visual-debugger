#include "app/app.hpp"
#include "net.hpp"

struct NetContext {
    std::unique_ptr<net::Net> net = nullptr;
    net::Socket* serverSock = nullptr;
    std::unique_ptr<net::Socket> clienetSock = nullptr;
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
}

void UpdateSystem(ecs::Commands& cmd, ecs::Queryer, ecs::Resources resources, ecs::Events&) {
    auto& netContext = resources.Get<NetContext>();
    auto& exitTrigger = resources.Get<ExitTrigger>();

    if (!netContext.clienetSock->Valid()) {
        return;
    }

    char buf[1024] = {0};
    int len = netContext.clienetSock->Recv(buf, sizeof(buf));
    int code = net::GetLastErrorCode();
    if (code == WSAEWOULDBLOCK || code == WSAETIMEDOUT || code == WSAENETDOWN || code == WSAECONNRESET) {
        if (len == WSAENETDOWN || len == WSAECONNRESET) {
            netContext.clienetSock->Close();
            netContext.serverSock->Close();
            netContext.net.reset();
            exitTrigger.Exit();
        }
        return;
    } else {
        LOGE("net error: ", net::GetLastError());
    }

    LOGI("recived: ", len, " - ", buf);
}

void RenderSystem(ecs::Commands& cmd, ecs::Queryer, ecs::Resources resources, ecs::Events&) {
    static float x = 0;
    
    auto& renderer = resources.Get<Renderer>();
    renderer.SetDrawColor(Color{0, 255, 0});
    renderer.DrawCircle({x, 100}, 50);
    
    x += 5;
    if (x > 1024) {
        x = 0;
    }
}

class VisualDebugger : public App {
public:
    VisualDebugger() {
        auto& world = GetWorld();
        world.AddPlugins<DefaultPlugins>()
            .AddStartupSystem(StartupSystem)
            .AddSystem(UpdateSystem)
            .AddSystem(RenderSystem)
            .AddSystem(ExitTrigger::DetectExitSystem);
    }
};

RUN_APP(VisualDebugger)