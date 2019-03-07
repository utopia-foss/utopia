#include <cassert>
#include <iostream>

#include <dune/utopia/core/signal.hh>

using namespace Utopia;

/// A custom signal handling method to test in the main
void custom_signal_handler(int) {
    return;
}


int main() {
    try {
        // Test the default signal handler handles the global flags correctly
        stop_now.store(false);
        default_signal_handler(123);  // not an actual signal, irrelevant here
        assert(stop_now.load());
        assert(received_signum.load() == 123);

        // Actually attach the default signal handler, then raise
        stop_now.store(false);
        attach_signal_handler(SIGINT);
        raise(SIGINT);
        assert(stop_now.load());
        assert(received_signum.load() == SIGINT);

        // Attach a custom signal handler function
        attach_signal_handler(SIGTERM, custom_signal_handler);
        raise(SIGTERM);
        // If this point is reached, the signal was handled and did not lead to
        // an exit... can now return with 0.
        return 0;
    }
    catch (...) {
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
