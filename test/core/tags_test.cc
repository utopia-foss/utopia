#include <cassert>
#include <iostream>

#include <utopia/core/tags.hh>

int main() {
    try {
        Utopia::DefaultTag t_true;
        assert(!t_true.is_tagged);
        t_true.is_tagged = true;
        assert(t_true.is_tagged);
        return 0;
    }
    catch (...) {
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
