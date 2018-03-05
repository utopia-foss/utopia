#ifndef TAGS_HH
#define TAGS_HH

namespace Utopia {

class DefaultTag
{

public:
    /// Construct DefaultTag
    DefaultTag(bool b) : is_tagged(b) { }

    /// Report whether is tagged
    bool is_tagged;

};

} // namespace Utopia

#endif // TAGS_HH
