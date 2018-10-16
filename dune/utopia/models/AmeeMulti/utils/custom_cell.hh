#ifndef UTOPIA_MODELS_AMEEMULTI_CUSTOM_CELL_HH
#define UTOPIA_MODELS_AMEEMULTI_CUSTOM_CELL_HH
#include <dune/utopia/core/cell.hh>

template <typename T, bool sync, class Tags, typename PositionType, typename IndexType, std::size_t custom_neighborhood_count = 0>
class StaticCell
    : public Cell<T, sync, Tags, PositionType, IndexType, custom_neighborhood_count>
{
private:
    std::vector<std::shared_ptr<StaticCell>> _neighborhood;

public:
    auto& neighborhood()
    {
        return _neighborhood;
    }

    void swap(StaticCell& other)
    {
        if (this = &other)
        {
            return;
        }
        else
        {
            using std::swap;
            swap(this->_position, other._position);
            swap(this->_boundary, other._boundary);
            swap(this->_neighborhood, other._neighborhood);
        }
    }

    virtual ~StaticCell() = default;

    StaticCell(const StaticCell& other)
        : Base(static_cast<const Base&>(other)), _neighborhood(other._neighborhood)
    {
    }

    StaticCell(StaticCell&& other)
        : Base(static_cast<Base&&>(other)),
          _neighborhood(std::move(other._neighborhood))
    {
    }

    StaticCell& operator=(const StaticCell& other)
    {
        static_cast<Base&>(*this).operator=(static_cast<const Base&>(other));
        _neighborhood = std::move(other._neighborhood);
        return *this;
    }

    StaticCell& operator=(StaticCell&& other)
    {
        static_cast<Base&>(*this).operator=(static_cast<Base&&>(other));
        _neighborhood = std::move(other._neighborhood);
        return *this;
    }

    StaticCell(T t, PositionType pos, const bool boundary, IndexType index)
        : Base(t, pos, boundary, index)
    {
    }
};
#endif