#ifndef SIGNAL_HH
#define SIGNAL_HH

#include <csignal>
#include <atomic>

namespace Utopia {

/// The flag indicating whether to stop whatever is being done right now
/** @detail This needs to be an atomic flag in order to be thread-safe. While
  *         the check of this flag is about three times slower than checking a
  *         boolean's value (quick-bench.com/IRtD4sQfp_xUwGPa2YrATD4vEyA), this
  *         difference is minute to other computations done between two checks.
  */
std::atomic<bool> stop_now;

/// Default signal handler functions, only setting the `stop_now` global flag
void default_signal_handler(int) {
    stop_now.store(true);
}

/// Attaches a signal handler for the given signal via sigaction
/** @detail This function sets 
  *
  * @param  signum  The signal number to attach a custom handler to
  * @param  handler Pointer to the function that should be invoked when the
  *                 specified signal is received.
  */
template<typename Handler>
void attach_signal_handler(int signum, Handler handler) {
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
void attach_signal_handler(int signum) {
    attach_signal_handler(signum, &default_signal_handler);
}

} // namespace Utopia

#endif // SIGNAL_HH
