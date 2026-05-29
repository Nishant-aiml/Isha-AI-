#include "survival/apk_update_migration_validation.hpp"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Starting apk_update_migration_validation_benchmark..." << std::endl;

    // Test a normal upgrade path
    auto status = isha::ApkUpdateMigrationValidation::validateAndMigrate("1.0", "2.0", ".");
    assert(status.migration_success == true);
    assert(status.tokenizer_realigned == true);
    assert(status.rollback_triggered == false);

    // Test a downgrade recovery path
    auto downgrade_status = isha::ApkUpdateMigrationValidation::validateAndMigrate("2.0", "1.0", ".");
    assert(downgrade_status.migration_success == false);
    assert(downgrade_status.rollback_triggered == true);

    std::cout << "apk_update_migration_validation_benchmark PASSED!" << std::endl;
    return 0;
}
