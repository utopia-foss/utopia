#include <dune/utopia/tags.hh>
#include <iostream>
#include <cassert>

int main()
{
    try {
        Utopia::DefaultTag t_true;
        assert(!t_true.is_tagged);
        t_true.is_tagged = true;
        assert(t_true.is_tagged);
        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
