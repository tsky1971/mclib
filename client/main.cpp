#include "Logger.h"

#include <mclib/common/Common.h>
#include <mclib/core/Client.h>
#include <mclib/util/Forge.h>
#include <mclib/util/Hash.h>
#include <mclib/util/Utility.h>
#include <mclib/util/VersionFetcher.h>

#include <iostream>

#ifdef _DEBUG
#pragma comment(lib, "../build64/Debug/mclib.lib")
#else
#pragma comment(lib, "../build64/Release/mclib.lib")
#endif

namespace {

const std::string server("172.16.81.229");
const u16 port = 25565;
const std::string username("testplayer");
const std::string password("");
const bool useProfileToken = false;

} // ns

int run(mc::protocol::Version versions, mc::util::ForgeHandler& forge);

int main(void) {
    mc::util::VersionFetcher versionFetcher(server, port);

    std::cout << "Fetching version" << std::endl;

    auto version = versionFetcher.GetVersion();

    mc::block::BlockRegistry::GetInstance()->RegisterVanillaBlocks(version);

    std::cout << "Connecting with version " << mc::protocol::to_string(version) << std::endl;
    return run(version, versionFetcher.GetForge());
}

int run(mc::protocol::Version version, mc::util::ForgeHandler& forge) {
    mc::protocol::packets::PacketDispatcher dispatcher;
    mc::core::Client client(&dispatcher, version);

    forge.SetConnection(client.GetConnection());

    client.GetPlayerController()->SetHandleFall(true);
    client.GetConnection()->GetSettings()
        .SetMainHand(mc::MainHand::Right)
        .SetViewDistance(16);

    example::Logger logger(&client, &dispatcher);

    try {
        std::cout << "Logging in." << std::endl;

        mc::core::AuthToken token;

        if (useProfileToken && mc::util::GetProfileToken(username, &token)) {
            std::cout << "Using profile token." << std::endl;
            client.Login(server, port, username, token, mc::core::UpdateMethod::Block);
        } else {
            client.Login(server, port, username, password, mc::core::UpdateMethod::Block);
        }
    } catch (std::exception& e) {
        std::wcout << e.what() << std::endl;
        return 1;
    }

    return 0;
}
