#ifndef UTOPIA_MODELS_AMEEMULTI_MEMORYPOOL_HH
#define UTOPIA_MODELS_AMEEMULTI_MEMORYPOOL_HH
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <queue>
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
    std::queue<T*> _free_pointers;
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
    T* allocate()
    {
        if (_free_pointers.empty())
        {
            throw std::bad_alloc();
        }

        auto ptr = reinterpret_cast<T*>(_free_pointers.front());
        _free_pointers.pop();
        return ptr;
    }

    /**
     * @brief Deallocate a pointer ptr of type T* of size 1
     *
     * @param ptr
     */
    void deallocate(T* ptr)
    {
        _free_pointers.push(ptr);
    }

    /**
     * @brief deallocate everything
     *
     */
    void clear()
    {
        _free_pointers = std::queue<T*>();
        for (std::size_t i = 0; i < _size; ++i)
        {
            _free_pointers.push(reinterpret_cast<T*>(_buffer + i));
        }
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
        return ::new (ptr) T(std::forward<Args&&>(args)...);
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
     * @param size: initial size of the memorypool
     */
    MemoryPool(std::size_t size) : _buffer(new Type[size]), _size(size)
    {
        for (std::size_t i = 0; i < _size; ++i)
        {
            _free_pointers.push(reinterpret_cast<T*>(_buffer + i));
        }
    }

    /**
     * @brief Destroy the Memory Pool object
     *
     */
    virtual ~MemoryPool()
    {
        delete[] _buffer;
    }

    /**
     * @brief Construct a new Memory Pool object
     *
     * @param other
     */
    MemoryPool(const MemoryPool& other) : _size(other._size)
    {
        _buffer = new Type[other._size];
        std::memcpy(_buffer, other._buffer, _size);

        T* otherbuff = reinterpret_cast<T*>(other._buffer);
        auto fqueue = other._free_pointers;
        while (!fqueue.empty())
        {
            _free_pointers.push(reinterpret_cast<T*>(_buffer + (fqueue.front() - otherbuff)));
            fqueue.pop();
        }
    }

    /**
     * @brief Construct a new Memory Pool object
     *
     * @param other
     */
    MemoryPool(MemoryPool&& other)
    {
        _buffer = other._buffer;
        other._buffer = nullptr;
        _free_pointers = std::move(other._free_pointers);
        _size = std::move(other._size);
    }

    /**
     * @brief swap states with other
     *
     * @param other
     */
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

    /**
     * @brief copy assignment
     *
     * @param other
     * @return MemoryPool&
     */
    MemoryPool& operator=(const MemoryPool& other)
    {
        _size = other._size;
        _buffer = new Type[other._size];
        std::memcpy(_buffer, other._buffer, _size);

        T* otherbuff = reinterpret_cast<T*>(other._buffer);
        auto fqueue = other._free_pointers;
        while (!fqueue.empty())
        {
            _free_pointers.push(reinterpret_cast<T*>(_buffer + (fqueue.front() - otherbuff)));
            fqueue.pop();
        }
        return *this;
    }

    /**
     * @brief move assignment
     *
     * @param other
     * @return MemoryPool&
     */
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