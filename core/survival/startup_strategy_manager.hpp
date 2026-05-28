#ifndef ISHA_AI_STARTUP_STRATEGY_MANAGER_HPP
#define ISHA_AI_STARTUP_STRATEGY_MANAGER_HPP

#include <string>

namespace isha {

enum class StartupType {
    COLD,
    WARM,
    DEGRADED,
    SAFE_MODE
};

struct StartupBudgets {
    unsigned int max_startup_latency_ms;
    unsigned int max_tokenizer_init_ms;
    unsigned int max_mmap_init_ms;
    unsigned int max_first_token_latency_ms;
    bool enforce_strict_isolation;
};

struct StartupMetrics {
    double startup_latency_ms;
    double tokenizer_init_ms;
    double mmap_init_ms;
    double model_validation_ms;
    double first_token_latency_ms;
    bool anr_risk_detected;
};

class StartupStrategyManager {
public:
    static std::string typeToString(StartupType type);

    static StartupBudgets getBudget(StartupType type);

    // Determines appropriate startup type based on system flag states
    static StartupType determineStartupType(bool first_run, bool previous_crash, bool slow_disk);

    // Validates if startup metrics satisfy budgets
    static bool validateStartup(StartupType type, const StartupMetrics& metrics);
};

} // namespace isha

#endif // ISHA_AI_STARTUP_STRATEGY_MANAGER_HPP
