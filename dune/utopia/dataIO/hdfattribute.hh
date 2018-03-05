#ifndef HDFATTRIBUTE_HH
#define HDFATTRIBUTE_HH
#include "hdfbufferfactory.hh"
#include "hdftypefactory.hh"

#include <hdf5.h>
#include <hdf5_hl.h>
#include <string>
namespace Utopia {
namespace DataIO {

/**
 * @brief Class for hdf5 attribute, which can be attached to groups and
 * datasets.

 */
class HDFAttribute {
private:
    // helper for making attribute
    template <typename result_type> hid_t __make_attribute__();

    // helper for writing stuff
    template <typename result_type, typename Iter, typename Adaptor>
    void __write_data__(Iter, Iter, Adaptor &&);

protected:
    /**
     * @brief id of attribute itself
     *
     */
    hid_t _attribute;

    /**
     * @brief reference to id of parent object: dataset or group
     *
     */
    hid_t *_parent_object;

    /**
     * @brief name of the attribute
     *
     */
    std::string _name;

    /**
     * @brief size of the attributes dataspace
     *
     */
    hsize_t _size;

public:
    hid_t get_id();

    std::string get_name();

    template <typename Iter, typename Adaptor>
    void read(Iter, Iter, Adaptor &&);

    template <typename Iter, typename Adaptor>
    void write(Iter, Iter, Adaptor &&);

    template <typename Attrtype> void write(Attrtype);

    void swap(HDFAttribute &);

    HDFAttribute();

    HDFAttribute(const HDFAttribute &);

    HDFAttribute(HDFAttribute &&other) : HDFAttribute() { this->swap(other); }

    HDFAttribute &operator=(HDFAttribute);
    virtual ~HDFAttribute();

    template <typename HDFObject>
    HDFAttribute(const HDFObject &, std::string &&, hsize_t = 1);
}; // namespace DataIO

template <typename T> void swap(HDFAttribute &, HDFAttribute &);
} // namespace DataIO
} // namespace Utopia
#endif