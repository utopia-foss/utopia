#include "hdftypefactory.hh"
namespace Utopia {
namespace DataIO {
template <> hid_t HDFTypeFactory::__get_type__<float>() {
    return H5T_NATIVE_FLOAT;
}
template <> hid_t HDFTypeFactory::__get_type__<double>() {
    return H5T_NATIVE_DOUBLE;
}
template <> hid_t HDFTypeFactory::__get_type__<long double>() {
    return H5T_NATIVE_LDOUBLE;
}
template <> hid_t HDFTypeFactory::__get_type__<int>() { return H5T_NATIVE_INT; }
template <> hid_t HDFTypeFactory::__get_type__<short int>() {
    return H5T_NATIVE_SHORT;
}
template <> hid_t HDFTypeFactory::__get_type__<long int>() {
    return H5T_NATIVE_LONG;
}
template <> hid_t HDFTypeFactory::__get_type__<long long int>() {
    return H5T_NATIVE_LLONG;
}
template <> hid_t HDFTypeFactory::__get_type__<unsigned int>() {
    return H5T_NATIVE_UINT;
}
template <> hid_t HDFTypeFactory::__get_type__<unsigned short int>() {
    return H5T_NATIVE_UINT16;
}
template <> hid_t HDFTypeFactory::__get_type__<std::size_t>() {
    return H5T_NATIVE_ULLONG;
}
template <> hid_t HDFTypeFactory::__get_type__<bool>() {
    return H5T_NATIVE_HBOOL;
}
template <> hid_t HDFTypeFactory::__get_type__<char>() { return H5T_C_S1; }

/**
 * @brief returns a HDF5 type from a given C++ primitive type
 *
 * @tparam T
 * @tparam T,
std::enable_if_t<
is_container<typename std::decay_t<T>>::value == false, int>
 * @return hid_t
 */
template <typename T,
          std::enable_if_t<
              is_container<typename std::decay_t<T>>::value == false, int>>
inline hid_t HDFTypeFactory::type(std::size_t size = 0) {
    return __get_type__<typename std::decay_t<T>>();
}

/**
 * @brief returns a HDF5 type from a given C++ fixed-or variable length C++
 * container type
 *
 * @tparam T
 * @tparam 1
 * @param size
 * @return hid_t
 */
template <typename T,
          std::enable_if_t<
              is_container<typename std::decay_t<T>>::value == true, int> = 1>
inline hid_t HDFTypeFactory::type(std::size_t size = 0) {
    if (size == 0) {
        return H5Tvlen_create(__get_type__<typename T::value_type>());
    } else {
        hid_t type = H5Tcopy(__get_type__<typename T::value_type>());
        H5Tset_size(type, size);
        return type;
    }
}
} // namespace DataIO
} // namespace Utopia
