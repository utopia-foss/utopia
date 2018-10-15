#ifndef AMEEMULTI_MEMORYPOOL_HH
#define AMEEMULTI_MEMORYPOOL_HH
#include <memory>

namespace Utopia
{
template <typename T>
class MemoryPool
{
private:
    using Type = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::shared_ptr<Type> _buffer;
    std::vector<T*> _free_pointers;
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

    T* allocate()
    {
        if (_free_pointers.size() == 0)
        {
            throw std::bad_alloc();
        }
        T* ptr = _free_pointers.back();
        _free_pointers.pop_back();
        return ptr;
    }

    void deallocate(T* ptr)
    {
        _free_pointers.push_back(ptr);
    }

    T* allocate(std::size_t n)
    {
        if (_free_pointers.size() < n)
        {
            throw std::bad_alloc();
        }

        auto ptr = _free_pointers[_free_pointers.size() - n];
        _free_pointers.erase(_free_pointers.begin() + _free_pointers.size() - n,
                             _free_pointers.end());
        return ptr;
    }

    void deallocate(T* ptr, std::size_t n)
    {
        for (std::size_t i = 0; i < n; ++i)
        {
            _free_pointers.push_back(ptr + i);
        }
    }

    template <typename... Args>
    T* construct(T* ptr, Args&&... args)
    {
        return ::new (ptr) T(std::forward<Args&&...>(args...));
    }

    void destruct(T* ptr)
    {
        ptr->~T();
    }

    MemoryPool(std::size_t size)
        : _buffer(std::shared_ptr<Type>(new Type[size])), _size(size)
    {
        _free_pointers.resize(size);
        for (std::size_t i = 0; i < size; ++i)
        {
            _free_pointers[i] = reinterpret_cast<T*>(&_buffer.get()[i]);
        }
    }

    MemoryPool(const MemoryPool& other) : _size(other._size)
    {
        _buffer = std::shared_ptr<Type>(new Type[other._size]);
        std::memcpy(_buffer.get(), other._buffer.get(), _size);
        T* otherbegin = other._buffer.get();
        _free_pointers.reserve(_size);

        for (auto& ptr : other._free_pointers)
        {
            _free_pointers.push_back(&other._buffer.get()[ptr - otherbegin]);
        }
    }

    MemoryPool(MemoryPool&& other) = default;

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
        std::memcpy(_buffer.get(), other._buffer.get(), _size);
        T* otherbegin = other._buffer.get();
        _free_pointers.reserve(_size);

        for (auto& ptr : other._free_pointers)
        {
            _free_pointers.push_back(&other._buffer.get()[ptr - otherbegin]);
        }
    }

    MemoryPool& operator=(MemoryPool&& other) = default;
};

} // namespace Utopia
#endif