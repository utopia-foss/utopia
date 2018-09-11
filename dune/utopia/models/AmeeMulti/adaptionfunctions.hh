#ifndef UTOPIA_MODELS_AGENT_STATE_HH
#define UTOPIA_MODELS_AGENT_STATE_HH
#include <cmath>
#include <iostream>
#include <vector>
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
struct multi_notnormed
{
    template <class Agenttype>
    std::vector<double> operator()(Agenttype agent)
    {
        auto& state = agent->state();
        auto start = state.start;
        auto end = state.end;
        const auto& celltraits = state.habitat->state().celltraits;
        const auto& phenotype = state.phenotype;
        std::vector<double> adaption(end - start, 0.);

        if (start > 0 and end > 0 and end > start and phenotype.size() > 0 and
            (int) celltraits.size() > 0 and start < (int)phenotype.size() and
            start < (int)celltraits.size())
        {
            for (int i = start;
                 i < end && i < (int)celltraits.size() && i < (int)phenotype.size(); ++i)
            {
                adaption[i] = (phenotype[i] * celltraits[i]) /
                              (1. + (std::abs(phenotype[i] - celltraits[i])));

                if (std::isnan(adaption[i]) or std::isinf(adaption[i]))
                {
                    std::cout << "  "
                              << "nan or inf in multi adaption " << std::endl;
                    std::cout << "  " << start << "," << end << "," << state.intensity
                              << ", " << (start == end ? "equal" : "unequal")
                              << ", " << adaption << std::endl;
                    std::cout << "  " << phenotype << std::endl;
                    std::cout << "  " << celltraits << std::endl;

                    adaption[i] = 0.;
                }
                else if (adaption < 0.)
                {
                    adaption[i] = 0.;
                }
            }
        }
        return adaption;
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif