#include "../agentstates/agentstate.hh"
#include "../agentstates/agentstate_policy_complex.hh"
#include "../agentstates/agentstate_policy_simple.hh"
#include "../cellstate.hh"
#include <chrono>
#include <iostream>
#include <list>
#include <random>
#include <thread>
using namespace Utopia::Models::AmeeMulti;
using C = std::vector<double>;
using G = std::vector<double>;
using P = std::vector<double>;
using CS = Cellstate<C>;
using AS = AgentState<CS, Agentstate_policy_simple<G, P, std::mt19937>>;

int main()
{
    std::mt19937 rnd(678923);
    std::vector<double> g(64);
    std::generate(g.begin(), g.end(), [&rnd]() {
        return std::uniform_real_distribution<double>(-5., .5)(rnd);
    });

    std::vector<double> m{0.1, 0.1, 0.1};
    CS eden(std::vector<double>(25, 1.), std::vector<double>(25, 5.),
            std::vector<double>(25, 5.));

    AS adam(g, std::make_shared<CS>(eden), 1., std::make_shared<std::mt19937>(rnd));

    std::list<AS> p;
    for (std::size_t i = 0; i < int(5e6); ++i)
    {
        if (i % 500000 == 0)
        {
            std::cout << " i = " << i << std::endl;
        }
        p.push_back(AS(adam, 1., m));
    }

    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(20s);

    return 0;
}