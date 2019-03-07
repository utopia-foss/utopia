#include <cassert>
#include <cstring>
#include <csignal>
#include <iostream>

#include <dune/utopia/core/exceptions.hh>

#include "testtools.hh"


int main() {
    try {
        std::cout << "Commencing test ..." << std::endl;

        // Test basic interface
        Utopia::Exception e0("what");
        assert(strcmp(e0.what(), "what") == 0);
        assert(e0.exit_code == 1);

        // Test specific exception classes
        // GotSignal ..........................................................
        Utopia::GotSignal gs0(SIGINT);
        assert(strcmp(gs0.what(), "Received signal: 2") == 0);
        assert(gs0.exit_code == 128 + 2);

        Utopia::GotSignal gs1(-2);
        assert(strcmp(gs1.what(), "Received signal: -2") == 0);
        assert(gs1.exit_code == 128 + abs(-2));

        // KeyError ...........................................................
        using KeyError = Utopia::KeyError;
        using Config = Utopia::DataIO::Config;

        // Empty node
        KeyError ke0("foo", {});
        assert(str_match(ke0.what(), "KeyError: foo"));
        assert(str_match(ke0.what(),
                         "The given node contains no entries!"));

        // Zombie node
        auto node = Config{};
        KeyError ke1("foo", node["invalid_key"]);
        assert(str_match(ke1.what(), "KeyError: foo"));
        assert(str_match(ke1.what(),
                         "The given node is a Zombie!"));
        
        // Populated node, but key not available
        node["some_entry"] = 123;
        KeyError ke2("foo", node);
        assert(str_match(ke2.what(), "KeyError: foo"));
        assert(str_match(ke2.what(),
                         "Make sure the desired key is available."));


        std::cout << "Test successful." << std::endl;
        return 0;
    }
    catch (...) {
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
