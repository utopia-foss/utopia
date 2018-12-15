#ifndef SIGNAL_HH
#define SIGNAL_HH

#include <csignal>
#include <atomic>

namespace Utopia {

/// The flag indicating whether to stop whatever is being done right now
std::atomic<bool> stop_now;

/// Default signal handler functions, only setting the `stop_now` global flag
void default_signal_handler([[maybe_unused]] int signum) {
    stop_now.store(true);
}

/// Attaches a signal handler for the given signal via sigaction
template<typename signum_t, typename Handler>
void attach_signal_handler(signum_t signum, Handler handler) {
    // Initialize the signal flag to make sure it is false
    stop_now.store(false);

    // Use POSIX-style sigaction definition rather than deprecated signal
    struct sigaction sa;

    sa.sa_handler = handler;
    sa.sa_flags = 0;
    sigaddset(&sa.sa_mask, signum);

    sigaction(signum, &sa, NULL);
}

/// Attaches the default signal handler for the given signal
template<typename signum_t>
void attach_signal_handler(signum_t signum) {
    attach_signal_handler(signum, &default_signal_handler);
}

} // namespace Utopia

#endif // SIGNAL_HH
