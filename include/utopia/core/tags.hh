#ifndef UTOPIA_CORE_TAGS_HH
#define UTOPIA_CORE_TAGS_HH

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

#endif // UTOPIA_CORE_TAGS_HH
