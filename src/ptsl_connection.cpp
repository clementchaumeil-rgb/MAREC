#include "ptsl_connection.h"

#include <iostream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include "PTSLC_CPP/CppPTSLClient.h"

using namespace PTSLC_CPP;

namespace marec {

PtslConnection::PtslConnection() = default;
PtslConnection::~PtslConnection() = default;

void PtslConnection::connect(const std::string& companyName, const std::string& appName)
{
    ClientConfig config;
    config.address = "localhost:31416";
    config.serverMode = Mode::Mode_ProTools;
    config.skipHostLaunch = SkipHostLaunch::SHLaunch_Yes;

    m_client = std::make_unique<CppPTSLClient>(config);

    m_client->OnResponseReceived = [](std::shared_ptr<CppPTSLResponse> /*response*/) {
        // Responses are handled synchronously via .get() on the future
    };

    // Register connection
    nlohmann::json reqBody;
    reqBody["company_name"] = companyName;
    reqBody["application_name"] = appName;

    CppPTSLRequest request(CommandId::CId_RegisterConnection, reqBody.dump());
    auto response = m_client->SendRequest(request).get();

    if (response.GetStatus() != CommandStatusType::TStatus_Completed) {
        auto errors = response.GetResponseErrorList();
        std::string errMsg = "RegisterConnection failed";
        if (!errors.errors.empty()) {
            errMsg += ": " + errors.errors[0]->errorMessage;
        }
        throw std::runtime_error(errMsg);
    }

    auto bodyJson = nlohmann::json::parse(response.GetResponseBodyJson());
    m_sessionId = bodyJson.value("session_id", "");

    if (m_sessionId.empty()) {
        throw std::runtime_error("RegisterConnection returned empty session_id");
    }

    m_client->SetSessionId(m_sessionId);
    std::cout << "Connected to Pro Tools (session: " << m_sessionId.substr(0, 8) << "...)\n";
}

CppPTSLClient& PtslConnection::client()
{
    if (!m_client) {
        throw std::runtime_error("Not connected to Pro Tools");
    }
    return *m_client;
}

const std::string& PtslConnection::sessionId() const
{
    return m_sessionId;
}

} // namespace marec
