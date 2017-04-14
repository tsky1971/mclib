#include <mclib/core/Client.h>
#include <mclib/util/Utility.h>

#include <iostream>

// These were changed in MSVC 2015. Redefine them so the old lib files link correctly.
// legacy_stdio_definitions.lib is supposed to define these but it doesn't seem to work.
#if (defined _MSC_VER) && _MSC_VER >= 1900
extern "C" { FILE __iob_func[3] = { *stdin,*stdout,*stderr }; }
int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;
#endif

namespace mc {
namespace core {

Client::Client(protocol::packets::PacketDispatcher* dispatcher, protocol::Version version)
    : m_Dispatcher(dispatcher),
    m_Connection(m_Dispatcher, version),
    m_EntityManager(m_Dispatcher),
    m_PlayerManager(m_Dispatcher, &m_EntityManager),
    m_World(m_Dispatcher),
    m_PlayerController(&m_Connection, m_World, m_PlayerManager),
    m_Connected(false)
{
    m_Connection.RegisterListener(this);
}

Client::~Client() {
    m_Connection.Disconnect();
    m_Connected = false;
    if (m_UpdateThread.joinable())
        m_UpdateThread.join();
    m_Connection.UnregisterListener(this);
}

void Client::OnSocketStateChange(network::Socket::Status newState) {
    m_Connected = (newState == network::Socket::Status::Connected);
}

void Client::UpdateThread() {
    s64 lastUpdate = 0;

    while (m_Connected) {
        try {
            m_Connection.CreatePacket();
        } catch (std::exception& e) {
            std::wcout << e.what() << std::endl;
        }

        m_PlayerController.Update();

        entity::EntityPtr playerEntity = m_EntityManager.GetPlayerEntity();
        if (playerEntity) {
            // Keep entity manager and player controller in sync
            playerEntity->SetPosition(m_PlayerController.GetPosition());
        }

        s64 time = util::GetTime();
        if (time >= lastUpdate + (1000 / 20)) {
            NotifyListeners(&ClientListener::OnTick);
            lastUpdate = time;
        }
    }
}

void Client::Login(const std::string& host, unsigned short port,
    const std::string& user, const std::string& password, bool block)
{
    if (m_UpdateThread.joinable()) {
        m_Connected = false;
        m_UpdateThread.join();
    }

    if (!m_Connection.Connect(host, port))
        throw std::runtime_error("Could not connect to server");

    m_Connection.Login(user, password);

    if (!block) {
        m_UpdateThread = std::thread(&Client::UpdateThread, this);
    } else {
        UpdateThread();
    }
}

void Client::Ping(const std::string& host, unsigned short port) {
    if (m_UpdateThread.joinable()) {
        m_Connected = false;
        m_UpdateThread.join();
    }

    if (!m_Connection.Connect(host, port))
        throw std::runtime_error("Could not connect to server");

    m_UpdateThread = std::thread(&Client::UpdateThread, this);

    m_Connection.Ping();
}

} // ns core
} // ns mc
