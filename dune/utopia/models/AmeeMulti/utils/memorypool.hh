#ifndef UTOPIA_MODELS_AMEEMULTI_MEMORYPOOL_HH
#define UTOPIA_MODELS_AMEEMULTI_MEMORYPOOL_HH
#include <iostream>
#include <memory>
#include <numeric>
#include <type_traits>
#include <vector>

namespace Utopia
{
/**
 * @brief A class which provides a fixed size memory pool f
 *
 * @tparam T
 */
template <typename T>
class MemoryPool
{
private:
    using Type = std::aligned_storage_t<sizeof(T), alignof(T)>;
    Type* _buffer;
    std::vector<std::size_t> _free_pointers;
    std::size_t _size;

public:
    auto get_buffer()
    {
        return _buffer;
    }

    auto& free_pointers()
    {
        return _free_pointers;
    }

    auto size()
    {
        return _size;
    }

    /**
     * @brief Allocate continuous space for n elements of type T. If n does not
     *        fit into the currently allocated region, a new region will be
     *        be allocated and the content
     *        and free pointers will be copied over. The new size doubles
     *        and reassigns the orignial size until the requested size fits in
     *
     * @param n number of elements to allocate
     * @return T*
     */
    T* allocate(std::size_t n)
    {
        if (_free_pointers.size() < n)
        {
            std::size_t s = _size;
            while (s - _free_pointers.size() < n)
            {
                s *= 2;
            }

            auto new_buffer = new Type[s];
            std::memcpy(new_buffer, _buffer, _size * sizeof(T));
            for (std::size_t i = _size; i < s; ++i)
            {
                _free_pointers.push_back(i);
            }
        }

        auto ptr =
            reinterpret_cast<T*>(&_buffer[_free_pointers[_free_pointers.size() - n]]);
        _free_pointers.erase(_free_pointers.begin() + _free_pointers.size() - n,
                             _free_pointers.end());
        return ptr;
    }

    /**
     * @brief deallocate the pointer ptr of type T* and size n
     *
     * @param ptr
     * @param n
     */
    void deallocate(T* ptr, std::size_t n)
    {
        std::size_t start = ptr - _buffer;
        for (std::size_t i = start; i < n + start; ++i)
        {
            _free_pointers.push_back(i);
        }
    }

    /**
     * @brief Deallocate a pointer ptr of type T* of size 1
     *
     * @param ptr
     */
    void deallocate(T* ptr)
    {
        _free_pointers.push_back(ptr - _buffer);
    }

    /**
     * @brief allocate a space for a single object of type T
     *
     * @return T*
     */
    T* allocate()
    {
        return allocate(1);
    }

    /**
     * @brief Inplace Construct an object of type T at pointer ptr with args
     * 'args' forwarded to its constructor
     * @tparam Args
     * @param ptr
     * @param args
     * @return T*
     */
    template <typename... Args>
    T* construct(T* ptr, Args&&... args)
    {
        return ::new (ptr) T(std::forward<Args&&...>(args...));
    }

    /**
     * @brief Destropy object pointed to by ptr
     *
     * @param ptr
     */
    void destroy(T* ptr)
    {
        ptr->~T();
    }

    /**
     * @brief Construct a new Memory Pool object with initial size 'size'
     *
     * @param size
     */
    MemoryPool(std::size_t size) : _buffer(new Type[size]), _size(size)
    {
        _free_pointers.resize(size);
        std::iota(_free_pointers.begin(), _free_pointers.end(), 0);
    }

    ~MemoryPool()
    {
        delete[] _buffer;
    }

    MemoryPool(const MemoryPool& other) : _size(other._size)
    {
        _buffer = new Type[other._size];
        std::memcpy(_buffer, other._buffer, _size);
        T* otherbegin = other._buffer;
        _free_pointers = other._free_pointers;
    }

    MemoryPool(MemoryPool&& other)
    {
        _buffer = other._buffer;
        other._buffer = nullptr;
        _free_pointers = std::move(other._free_pointers);
        _size = std::move(other._size);
    }

    void swap(MemoryPool& other)
    {
        if (this == &other)
        {
            return;
        }
        else
        {
            using std::swap;
            swap(_buffer, other._buffer);
            swap(_free_pointers, other._free_pointers);
            swap(_size, other._size);
        }
    }

    MemoryPool& operator=(const MemoryPool& other)
    {
        _size = other._size;
        _buffer = std::shared_ptr<Type>(new Type[other._size]);
        std::memcpy(_buffer, other._buffer, _size);
        T* otherbegin = other._buffer;
        _free_pointers = other._free_pointers;
        return *this;
    }

    MemoryPool& operator=(MemoryPool&& other)
    {
        _buffer = other._buffer;
        other._buffer = nullptr;
        _free_pointers = std::move(other._free_pointers);
        _size = std::move(other._size);
        return *this;
    }
};

} // namespace Utopia
#endif