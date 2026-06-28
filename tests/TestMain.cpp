#include <juce_core/juce_core.h>

int main()
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult(i)->failures;

    if (failures > 0)
        std::cerr << failures << " test(s) FAILED\n";
    else
        std::cout << "All tests PASSED\n";

    return failures > 0 ? 1 : 0;
}
