#ifndef TAGS_HH
#define TAGS_HH

namespace Utopia {

class DefaultTag
{

public:
    /// Construct DefaultTag
    DefaultTag(bool b) : _b(b) { }

    /// Return whether is tagged
    bool is_tagged() { return _b; }
    void set_is_tagged(bool b) { _b = b; }

private:
    bool _b;

};


} // namespace Utopia

#endif // TAGS_HH
