// LUMA Studio - Test Runner
// Run with: ./luma_tests [options]

#include "integration_test.h"
#include "unit_tests.h"
#include "mesh_edit_tests.h"
#include "selection_tests.h"
#include "shader_tests.h"
#include "test_photo_to_avatar.h"

int main(int argc, char* argv[]) {
    bool showManual = false;
    bool runUnit = true;
    bool runIntegration = true;
    bool runAvatar = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--manual" || arg == "-m") {
            showManual = true;
        }
        if (arg == "--unit" || arg == "-u") {
            runUnit = true;
            runIntegration = false;
        }
        if (arg == "--integration" || arg == "-i") {
            runUnit = false;
            runIntegration = true;
        }
        if (arg == "--avatar" || arg == "-v") {
            runAvatar = true;
            runUnit = false;
            runIntegration = false;
        }
        if (arg == "--all" || arg == "-a") {
            runUnit = true;
            runIntegration = true;
            runAvatar = true;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout << "LUMA Studio Test Suite\n";
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --unit, -u        Run unit tests only\n";
            std::cout << "  --integration, -i Run integration tests only\n";
            std::cout << "  --avatar, -v      Run photo-to-avatar pipeline tests\n";
            std::cout << "  --all, -a         Run all tests (default)\n";
            std::cout << "  --manual, -m      Show manual test checklist\n";
            std::cout << "  --help, -h        Show this help\n";
            return 0;
        }
    }
    
    bool allPassed = true;
    
    // Run unit tests
    if (runUnit) {
        luma::test::UnitTestRunner runner;
        luma::test::registerAllTests(runner);
        
        // Register additional test suites
        luma::test::MeshEditTests::registerMeshEditTests(runner);
        luma::test::SelectionTests::registerSelectionTests(runner);
        luma::test::ShaderTests::registerShaderTests(runner);
        
        runner.run();
        allPassed = allPassed && runner.allPassed();
    }
    
    // Run integration tests
    if (runIntegration) {
        bool integrationPassed = luma::test::runAllIntegrationTests();
        allPassed = allPassed && integrationPassed;
    }
    
    // Run photo-to-avatar pipeline tests
    if (runAvatar) {
        bool avatarPassed = luma::tests::runPhotoToAvatarTests();
        allPassed = allPassed && avatarPassed;
    }
    
    // Show manual checklist if requested
    if (showManual) {
        luma::test::printManualTestChecklist();
    } else if (!runUnit && !runIntegration && !runAvatar) {
        std::cout << "\nRun with --manual to see manual test checklist.\n";
    }
    
    std::cout << "\n========================================\n";
    std::cout << "FINAL RESULT: " << (allPassed ? "ALL PASSED" : "SOME FAILED") << "\n";
    std::cout << "========================================\n";
    
    return allPassed ? 0 : 1;
}
