#ifndef SIGNAL_HH
#define SIGNAL_HH

#include <csignal>
#include <atomic>

namespace Utopia {

/// The flag indicating whether a signal was received
std::atomic<bool> got_signal;

/// Default signal handler functions, only setting the `got_signal` global flag
void default_signal_handler(int signum) {
    got_signal.store(true);
}

/// Attaches a signal handler for the given signal via sigaction
template<typename signum_t, typename Handler>
void attach_signal_handler(signum_t signum, Handler handler) {
    // Initialize the signal flag to make sure it is false
    got_signal.store(false);

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
