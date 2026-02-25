#pragma once

#include <memory>
#include <string>

namespace PTSLC_CPP {
class CppPTSLClient;
}

namespace marec {

class PtslConnection {
public:
    PtslConnection();
    ~PtslConnection();

    // Connect to Pro Tools and register the connection.
    // Throws std::runtime_error on failure.
    void connect(const std::string& companyName, const std::string& appName);

    // Access to the underlying client for sending requests.
    PTSLC_CPP::CppPTSLClient& client();

    // Session ID obtained from RegisterConnection.
    const std::string& sessionId() const;

private:
    std::unique_ptr<PTSLC_CPP::CppPTSLClient> m_client;
    std::string m_sessionId;
};

} // namespace marec
