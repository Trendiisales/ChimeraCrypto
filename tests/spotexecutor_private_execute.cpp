// spotexecutor_private_execute.cpp — Phase-1 NEGATIVE compile test.
//
// This TU deliberately calls SpotExecutor::execute() from non-gateway (strategy)
// code. After the Phase-1 fix execute() is PRIVATE, so this MUST FAIL to compile.
// run_phase1_tests.sh asserts the compile fails; a successful compile = the
// chokepoint regressed (strategies can bypass the ExecutionGateway again).
#include "live/SpotExecutor.hpp"

int main() {
    chimera::SpotExecutor ex;
    // Direct raw execute() from strategy code — must be rejected by the compiler.
    ex.execute("BTCUSDT", true, 1.0, 100.0);
    return 0;
}
