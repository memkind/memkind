
#include <memory>
#include <string>
#include <exception>
#include <type_traits>
#include <cstddef>
#include <utility>
#include <atomic>

#include "memkind.h"


namespace pmem {
template<typename T>
struct allocator {

    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    template<class U>
    struct rebind {
        typedef allocator<U> other;
    };

    template<typename U>
    friend struct allocator;

private:
    memkind_t kind_ptr;
    std::atomic<size_t> * kind_cnt;

    void clean_up() {		
        if (kind_cnt->fetch_sub(1) == 1) {

            memkind_destroy_kind(kind_ptr);
            delete kind_cnt;
        }
    }

    template <typename U>
    inline void assign(U&&  other) {
        kind_ptr = other.kind_ptr;
        kind_cnt = other.kind_cnt;
        ++(*kind_cnt);
    }


public:

    /* Parameterized constructors */

    explicit allocator(const char *dir, size_t max_size) {

        int err_c  = memkind_create_pmem(dir, max_size, &kind_ptr);
        if (err_c) {
            throw std::runtime_error(std::string("An error occured while creating pmem kind; error code: ") + std::to_string(err_c));
        }

        kind_cnt = new std::atomic<size_t>(1);

    }

    explicit allocator(const std::string& dir, size_t max_size) { allocator(dir.c_str(), max_size); };

    /* Copy constructors */		

    allocator(const allocator& other) noexcept {
        assign(other);
    }

    template <typename U>
    allocator(const allocator<U>& other) noexcept {
        assign(other);
    }

    /* Move constructors */		

    allocator(const allocator&& other) noexcept {
        assign(other);
    }

    template <typename U>
    allocator(const allocator<U>&& other) noexcept {
        assign(other);
    }

    /* Copy-assign operators */		

    void operator = (const allocator<T>& other) noexcept {
        clean_up();		
        assign(other);
    }

    template <typename U>
    void operator = (const allocator<U>& other) noexcept {
        clean_up();		
        assign(other);
    }

    /* Move-assign operators */		

    void operator = (const allocator<T>&& other) noexcept {
        clean_up();		
        assign(other);
    }

    template <typename U>
    void operator = (const allocator<U>&& other) noexcept {
        clean_up();		
        assign(other);
    }

    /* Compare operators */		

    template <typename U>
    bool operator ==(const allocator<U>& other) const {
        return kind_ptr == other.kind_ptr;
    }

    template <typename U>
    bool operator !=(const allocator<U>& other) const {
        return !(*this ==other);
    }

    /* Allocator methods */		

    T* allocate(std::size_t n) const {
        void* mptr = memkind_calloc(kind_ptr, n, sizeof(T));
        return static_cast<T*>(mptr);
    }

    void deallocate(T* const p, std::size_t n) const {
        memkind_free(kind_ptr, static_cast<void*>(p));
    }

    template <class U, class... Args>
    void construct(U* p, Args... args) const {
        ::new((void *)p) U(std::forward<Args>(args)...); 
    }

    void destroy(T* const p) const {
        p->~value_type();
    }


    /* Destructor */		

    ~allocator() {
        clean_up();
    }
};
}
