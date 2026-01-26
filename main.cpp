// LUMA Engine - Animation System Tests
#include <iostream>
#include "engine/animation/animation_test.h"

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  LUMA Engine - Animation System Tests" << std::endl;
    std::cout << "======================================" << std::endl;
    
    bool success = luma::test::runAnimationTests();
    
    return success ? 0 : 1;
}
