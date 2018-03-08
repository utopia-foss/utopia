#ifndef TAGS_HH
#define TAGS_HH

namespace Utopia {

class EmptyTag{};

class DefaultTag
{

public:
    /// Construct DefaultTag
    DefaultTag() : is_tagged(false) { }

    /// Report whether is tagged
    bool is_tagged;

};

} // namespace Utopia

#endif // TAGS_HH
