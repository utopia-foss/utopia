#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

/// Compare two containers
template<typename A, typename B>
bool compare_containers (const A& a, const B& b)
{
    if (a.size() != b.size())
        return false;
    
    std::vector<bool> res(a.size());
    std::transform(a.begin(), a.end(), b.begin(),
        res.begin(),
        [](const auto x, const auto y) { return x == y; }
    );

    return std::all_of(res.begin(), res.end(),
        [](const auto x){ return x; }
    );
}

#endif // UTOPIA_TEST_MODEL_TEST_HH
