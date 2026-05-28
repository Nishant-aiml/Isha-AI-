#include "session_manager.hpp"
#include "../runtime/event_bus.hpp"
#include "../logging/logger.hpp"
#include <random>
#include <sstream>

namespace isha {

static std::string generateRandomId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    const char *hex = "0123456789abcdef";
    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << hex[dis(gen)];
    }
    return ss.str();
}

SessionManager::SessionManager(const RuntimeConfig& config) : config_(config) {
    // Clean expired sessions when memory is tight
    EventBus::getInstance().subscribe(EventType::MEMORY_PRESSURE_CRITICAL, "SessionManager", [this](const Event&) {
        ISHA_LOG_INFO("SessionManager", "Memory warning received. Clearing session cache.");
        cleanExpiredSessions();
    });
}

SessionManager::~SessionManager() {
    EventBus::getInstance().unsubscribe(EventType::MEMORY_PRESSURE_CRITICAL, "SessionManager");
}

std::string SessionManager::createSession() {
    std::string id = generateRandomId();
    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::seconds(config_.session_expiration_seconds);

    Session session = { id, now, expires, true, {} };
    sessions_[id] = session;

    ISHA_LOG_INFO("SessionManager", "Created session: " + id);
    return id;
}

bool SessionManager::addTurn(const std::string& session_id, const std::string& query, const std::string& response) {
    auto it = sessions_.find(session_id);
    if (it == sessions_.end() || !it->second.is_active) {
        ISHA_LOG_WARN("SessionManager", "Cannot add turn: session " + session_id + " is invalid or inactive.");
        return false;
    }
    
    it->second.conversation_turns.push_back({query, response});
    ISHA_LOG_DEBUG("SessionManager", "Added turn to session: " + session_id);
    return true;
}

Session* SessionManager::getSession(const std::string& session_id) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        return &(it->second);
    }
    return nullptr;
}

void SessionManager::expireSession(const std::string& session_id) {
    auto it = sessions_.find(session_id);
    if (it != sessions_.end() && it->second.is_active) {
        it->second.is_active = false;
        ISHA_LOG_INFO("SessionManager", "Session expired: " + session_id);
        EventBus::getInstance().publish({EventType::SESSION_EXPIRED, "SessionManager", session_id});
    }
}

void SessionManager::cleanExpiredSessions() {
    auto now = std::chrono::system_clock::now();
    unsigned int cleaned_count = 0;
    
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (!it->second.is_active || it->second.expires_at <= now) {
            std::string id = it->first;
            it = sessions_.erase(it);
            cleaned_count++;
            EventBus::getInstance().publish({EventType::SESSION_CLEANED, "SessionManager", id});
        } else {
            ++it;
        }
    }

    if (cleaned_count > 0) {
        ISHA_LOG_INFO("SessionManager", "Purged " + std::to_string(cleaned_count) + " expired/inactive sessions.");
    }
}

} // namespace isha
