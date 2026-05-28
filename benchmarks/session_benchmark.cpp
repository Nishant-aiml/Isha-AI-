#include "session/session_manager.hpp"
#include "logging/logger.hpp"
#include <iostream>
#include <cassert>

int main() {
    isha::Logger::getInstance().setMinLevel(isha::LogLevel::DEBUG);
    ISHA_LOG_INFO("Benchmark", "=== STARTING SESSION BENCHMARK ===");

    isha::RuntimeConfig config;
    config.session_expiration_seconds = 1; // 1 second for fast test
    isha::SessionManager manager(config);

    // Create session
    std::string s_id = manager.createSession();
    assert(!s_id.empty());
    assert(manager.getActiveSessions().size() == 1);

    // Add conversation turns
    bool turn_ok = manager.addTurn(s_id, "Hello", "Hi there!");
    assert(turn_ok);

    isha::Session* session = manager.getSession(s_id);
    assert(session != nullptr);
    assert(session->conversation_turns.size() == 1);
    assert(session->conversation_turns[0].first == "Hello");

    // Manually expire
    manager.expireSession(s_id);
    assert(!session->is_active);

    // Cleanup expired
    manager.cleanExpiredSessions();
    assert(manager.getActiveSessions().empty());

    ISHA_LOG_INFO("Benchmark", "=== SESSION BENCHMARK COMPLETED SUCCESSFULLY ===");
    return 0;
}
