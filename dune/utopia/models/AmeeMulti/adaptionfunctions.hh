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
auto multi_notnormed = [](auto agent) -> std::vector<double> {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.trait;
    std::vector<double> adaption(end - start, 0.);

    if (start > 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        for (int i = start;
             i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i)
        {
            adaption[i] = (trait[i] * celltrait[i]) /
                          (1. + (std::abs(trait[i] - celltrait[i])));

            if (std::isnan(adaption[i]) or (adaption[i] < 0.) or std ::isinf(adaption[i]))
            {
                adaption[i] = 0.;
            }
        }
    }
    return adaption;
};

auto multi_normed = [](auto agent) -> std::vector<double> {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.trait;
    std::vector<double> adaption(end - start, 0.);

    if (start > 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        for (int i = start;
             i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i)
        {
            adaption[i] = ((trait[i] * celltrait[i]) /
                           (1. + (std::abs(trait[i] - celltrait[i])))) /
                          (end - start);

            if (std::isnan(adaption[i]) or (adaption[i] < 0.) or std ::isinf(adaption[i]))
            {
                adaption[i] = 0.;
            }
        }
    }
    return adaption;
};

auto simple_notnormed = [](auto agent) {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.trait;
    std::vector<double> adaption(end - start, 0.);

    if (start > 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        for (int i = start;
             i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i)
        {
            adaption[i] = 1. / (1. + (std::abs(trait[i] - celltrait[i])));

            if (std::isnan(adaption[i]) or (adaption[i] < 0.) or std ::isinf(adaption[i]))
            {
                adaption[i] = 0.;
            }
        }
    }
    return adaption;
};

auto simple_normed = [](auto agent) {
    auto& state = agent->state();
    auto start = state.start;
    auto end = state.end;
    const auto& celltrait = state.habitat->state().celltrait;
    const auto& trait = state.trait;
    std::vector<double> adaption(end - start, 0.);

    if (start > 0 and end > 0 and end > start and trait.size() > 0 and
        (int) celltrait.size() > 0 and start < (int)trait.size() and
        start < (int)celltrait.size())
    {
        for (int i = start;
             i < end && i < (int)celltrait.size() && i < (int)trait.size(); ++i)
        {
            adaption[i] = (1. / (1. + (std::abs(trait[i] - celltrait[i])))) / (end - start);

            if (std::isnan(adaption[i]) or (adaption[i] < 0.) or std ::isinf(adaption[i]))
            {
                adaption[i] = 0.;
            }
        }
    }
    return adaption;
};

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif