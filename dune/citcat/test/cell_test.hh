#include <cassert>

template<typename Cell, typename Position, typename Index>
void assert_cell_members (const Cell& c, const Position& pos, const Index& id, const bool boundary)
{
    for(std::size_t i=0; i<pos.size(); i++)
        assert(c.position()[i]==pos[i]);
    assert(c.index()==id);
    assert(c.boundary()==boundary);
}