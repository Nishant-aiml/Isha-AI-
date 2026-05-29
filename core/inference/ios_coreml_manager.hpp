#ifndef ISHA_AI_IOS_COREML_MANAGER_HPP
#define ISHA_AI_IOS_COREML_MANAGER_HPP

#include <string>
#include <vector>
#include "../model/model_manager.hpp"

namespace isha {

class IosCoremlManager {
public:
    explicit IosCoremlManager(ModelManager& model_mgr);
    ~IosCoremlManager() = default;

    // Triggers memory warning: unloads active model based on T3 -> T2 -> T1 order
    void handleMemoryWarning();

    // Handles background transition: stop inference, checkpoint, unload model, keep T0 metadata only
    void handleBackgroundEntry();

    // Handles background expiration: teardown everything immediately
    void handleBackgroundExpiration();

    bool isAneAccelerated() const { return ane_accelerated_; }

private:
    ModelManager& model_mgr_;
    bool ane_accelerated_ = true;
};

} // namespace isha

#endif // ISHA_AI_IOS_COREML_MANAGER_HPP
