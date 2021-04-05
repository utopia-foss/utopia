#include <iostream>

#include "Opinionet.hh"

using namespace Utopia::Models::Opinionet;

int main (int, char** argv)
{
    try {

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Get opinion space and network kind
        auto model_cfg = pp.get_cfg()["Opinionet"];

        const std::string interaction_function =
            Utopia::get_as<std::string>("interaction_function", model_cfg);
        const std::string opinion_space_type =
            Utopia::get_as<std::string>("type", model_cfg["opinion_space"]);
        const bool nw_is_directed =
            Utopia::get_as<bool>("directed", model_cfg["network"]);
        const bool rewiring =
            Utopia::get_as<bool>("rewiring", model_cfg["network"]["edges"]);

        // Initialize the main model instance
        if (opinion_space_type == "continuous") {

            if (interaction_function == "Deffuant") {

                if (nw_is_directed) {

                    if (rewiring) {
                        Opinionet<Deffuant, continuous, NetworkDirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<Deffuant, continuous, NetworkDirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
                else {
                    if (rewiring) {
                        Opinionet<Deffuant, continuous, NetworkUndirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<Deffuant, continuous, NetworkUndirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
            }
            else {
                if (nw_is_directed) {

                    if (rewiring) {
                        Opinionet<HegselmannKrause, continuous,
                                  NetworkDirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<HegselmannKrause, continuous,
                                  NetworkDirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
                else {
                    if (rewiring) {
                        Opinionet<HegselmannKrause, continuous,
                                  NetworkUndirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<HegselmannKrause, continuous,
                                  NetworkUndirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
            }
        }
        else {
            if (interaction_function == "Deffuant") {

                if (nw_is_directed) {

                    if (rewiring) {
                        Opinionet<Deffuant, discrete, NetworkDirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<Deffuant, discrete, NetworkDirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
                else {
                    if (rewiring) {
                        Opinionet<Deffuant, discrete, NetworkUndirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<Deffuant, discrete, NetworkUndirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
            }
            else {
                if (nw_is_directed) {

                    if (rewiring) {
                        Opinionet<HegselmannKrause, discrete, NetworkDirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<HegselmannKrause, discrete, NetworkDirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
                else {
                    if (rewiring) {
                        Opinionet<HegselmannKrause, discrete,
                                  NetworkUndirected,
                                  RewiringOn> model("Opinionet", pp);
                        model.run();
                    }
                    else {
                        Opinionet<HegselmannKrause, discrete,
                                  NetworkUndirected,
                                  RewiringOff> model("Opinionet", pp);
                        model.run();
                    }
                }
            }
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
