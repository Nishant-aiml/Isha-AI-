#ifndef ISHA_AI_SESSION_MANAGER_HPP
#define ISHA_AI_SESSION_MANAGER_HPP

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "../config/runtime_config.hpp"

namespace isha {

struct Session {
    std::string session_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    bool is_active;
    std::vector<std::pair<std::string, std::string>> conversation_turns; // (query, response)
};

class SessionManager {
public:
    SessionManager(const RuntimeConfig& config);
    ~SessionManager();

    std::string createSession();
    bool addTurn(const std::string& session_id, const std::string& query, const std::string& response);
    Session* getSession(const std::string& session_id);
    void expireSession(const std::string& session_id);
    void cleanExpiredSessions();

    const std::map<std::string, Session>& getActiveSessions() const { return sessions_; }

private:
    RuntimeConfig config_;
    std::map<std::string, Session> sessions_;
};

} // namespace isha

#endif // ISHA_AI_SESSION_MANAGER_HPP
