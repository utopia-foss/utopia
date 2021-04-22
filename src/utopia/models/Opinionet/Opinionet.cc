#include <iostream>

#include "Opinionet.hh"

using namespace Utopia::Models::Opinionet;

int main (int, char** argv)
{
    try {

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Get network kind
        auto model_cfg = pp.get_cfg()["Opinionet"];
        const bool nw_is_directed =
            Utopia::get_as<bool>("directed", model_cfg["network"]);

        // Initialize the main model instance
        if (nw_is_directed) {
            Opinionet<NetworkDirected> model("Opinionet", pp);
            model.run();
        }

        else {
            Opinionet<NetworkUndirected> model("Opinionet", pp);
            model.run();
        }

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
