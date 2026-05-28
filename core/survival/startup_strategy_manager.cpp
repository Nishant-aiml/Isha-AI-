#include "startup_strategy_manager.hpp"

namespace isha {

std::string StartupStrategyManager::typeToString(StartupType type) {
    switch (type) {
        case StartupType::COLD: return "COLD";
        case StartupType::WARM: return "WARM";
        case StartupType::DEGRADED: return "DEGRADED";
        case StartupType::SAFE_MODE: return "SAFE_MODE";
        default: return "UNKNOWN";
    }
}

StartupBudgets StartupStrategyManager::getBudget(StartupType type) {
    StartupBudgets budgets;
    switch (type) {
        case StartupType::COLD:
            budgets.max_startup_latency_ms = 4000;
            budgets.max_tokenizer_init_ms = 800;
            budgets.max_mmap_init_ms = 1000;
            budgets.max_first_token_latency_ms = 2500;
            budgets.enforce_strict_isolation = false;
            break;
        case StartupType::WARM:
            budgets.max_startup_latency_ms = 1000;
            budgets.max_tokenizer_init_ms = 100;
            budgets.max_mmap_init_ms = 200;
            budgets.max_first_token_latency_ms = 800;
            budgets.enforce_strict_isolation = false;
            break;
        case StartupType::DEGRADED:
            budgets.max_startup_latency_ms = 6000;
            budgets.max_tokenizer_init_ms = 1500;
            budgets.max_mmap_init_ms = 2000;
            budgets.max_first_token_latency_ms = 4000;
            budgets.enforce_strict_isolation = true;
            break;
        case StartupType::SAFE_MODE:
            budgets.max_startup_latency_ms = 3000;
            budgets.max_tokenizer_init_ms = 500;
            budgets.max_mmap_init_ms = 500;
            budgets.max_first_token_latency_ms = 1500;
            budgets.enforce_strict_isolation = true;
            break;
    }
    return budgets;
}

StartupType StartupStrategyManager::determineStartupType(bool first_run, bool previous_crash, bool slow_disk) {
    if (previous_crash) {
        return StartupType::SAFE_MODE;
    }
    if (slow_disk) {
        return StartupType::DEGRADED;
    }
    if (first_run) {
        return StartupType::COLD;
    }
    return StartupType::WARM;
}

bool StartupStrategyManager::validateStartup(StartupType type, const StartupMetrics& metrics) {
    if (metrics.anr_risk_detected) return false;
    
    StartupBudgets budgets = getBudget(type);
    
    if (metrics.startup_latency_ms > budgets.max_startup_latency_ms) return false;
    if (metrics.tokenizer_init_ms > budgets.max_tokenizer_init_ms) return false;
    if (metrics.mmap_init_ms > budgets.max_mmap_init_ms) return false;
    if (metrics.first_token_latency_ms > budgets.max_first_token_latency_ms) return false;
    
    return true;
}

} // namespace isha
