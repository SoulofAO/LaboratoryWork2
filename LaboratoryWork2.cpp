#include <cassert>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>
#include <cmath>
#include <iostream>

template<typename T>
class Array final {
public:
    class ConstIterator;
    class Iterator {
    public:
        Iterator(const Array<T>* owner, int pos, bool reverse = false)
            : owner_(owner), pos_(pos), reverse_(reverse) {
        }

        const T get() const {
            return (*owner_)[pos_];
        }

        void set(const T& value) const {
            (*owner_)[pos_] = value;
        }

        void next() {
            if (!reverse_) ++pos_;
            else --pos_;
        }

        bool hasNext() const {
            if (!reverse_) return pos_ < owner_->size();
            else return pos_ >= 0;
        }

        Iterator& operator++() { next(); return *this; }
        T& operator*() const { return get(); }

    private:
        const Array<T>* owner_;
        int pos_;
        bool reverse_;
    };

    class ConstIterator {
    public:
        ConstIterator(const Array<T>* owner, int pos, bool reverse = false)
            : owner_(owner), pos_(pos), reverse_(reverse) {
        }

        const T& get() const {
            return (*owner_)[pos_];
        }

        void next() {
            if (!reverse_) ++pos_;
            else --pos_;
        }

        bool hasNext() const {
            if (!reverse_) return pos_ < owner_->size();
            else return pos_ >= 0;
        }

        const T& operator*() const { return get(); }
        ConstIterator& operator++() { next(); return *this; }

    private:
        const Array<T>* owner_;
        int pos_;
        bool reverse_;
    };

    Array()
        : data_(nullptr), size_(0), capacity_(0)
    {
        reserve(default_initial_capacity);
    }

    //�� ����� ��������. ��������� �����, ��� �������� ������� ��������������.
    explicit Array(int capacity)
        : data_(nullptr), size_(0), capacity_(0)
    {
        reserve(capacity > 0 ? capacity : default_initial_capacity);
    }

    ~Array() {
        clear_and_free();
    }

    //�����������
    Array(const Array& other)
        : data_(nullptr), size_(0), capacity_(0)
    {
        if (other.size_ > 0) {
            reserve(other.capacity_);
            for (int i = 0; i < other.size_; ++i) {
                construct_at(i, other.data_[i]);
            }
            size_ = other.size_;
        }
        else {
            reserve(default_initial_capacity);
        }
    }

    //Move �����������
    Array(Array&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    // ������������.
    Array& operator=(const Array& other) {
        if (this == &other) return *this;
        clear_and_free();
        if (other.size_ > 0) {
            reserve(other.capacity_);
            for (int i = 0; i < other.size_; ++i) {
                construct_at(i, other.data_[i]);
            }
            size_ = other.size_;
        }
        else {
            reserve(default_initial_capacity);
        }
        return *this;
    }

    //�������.
    int insert(const T& value) {
        ensure_capacity_for_one_more();
        construct_at(size_, value);
        ++size_;
        return size_ - 1;
    }

    int insert(int index, const T& value) {
        assert(index >= 0 && index <= size_);
        ensure_capacity_for_one_more();
        if (size_ == 0 || index == size_) {
            construct_at(size_, value);
            ++size_;
            return size_ - 1;
        }

        construct_at(size_, data_[size_ - 1]);
        for (int i = size_ - 2; i >= index; --i) {
            destroy_at(i + 1);
            construct_at(i + 1, data_[i]);
            destroy_at(i);
        }
        destroy_at(index);
        construct_at(index, value);

        ++size_;
        return index;
    }

    void remove(int index) {
        assert(index >= 0 && index < size_);
        destroy_at(index);
        for (int i = index; i + 1 < size_; ++i) {
            destroy_at(i);
            if constexpr (std::is_move_constructible<T>::value) {
                construct_at(i, std::move(data_[i + 1]));
            }
            else {
                construct_at(i, data_[i + 1]);
            }
            destroy_at(i + 1);
        }
        --size_;
    }

    T& operator[](int index) {
        assert(index >= 0 && index < size_);
        return data_[index];
    }

    const T& operator[](int index) const {
        assert(index >= 0 && index < size_);
        return data_[index];
    }

    int size() const { return size_; }
    Iterator iterator() { return Iterator(this, 0, false); }
    ConstIterator iterator() const { return ConstIterator(this, 0, false); }

    Iterator reverseIterator() { return Iterator(this, size_ - 1, true); }
    ConstIterator reverseIterator() const { return ConstIterator(this, size_ - 1, true); }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }
    const T* cbegin() const { return data_; }
    const T* cend() const { return data_ + size_; }

private:
    T* data_;
    int size_;
    int capacity_;
    static constexpr int default_initial_capacity = 8;
    static constexpr double growth_factor = 1.6;

    void ensure_capacity_for_one_more() {
        if (size_ + 1 > capacity_) {
            int newCap = capacity_ > 0 ? static_cast<int>(std::ceil(capacity_ * growth_factor)) : default_initial_capacity;
            if (newCap <= capacity_) newCap = capacity_ + 1;
            if (newCap < size_ + 1) newCap = size_ + 1;
            reallocate(newCap);
        }
    }

    void reserve(int newCapacity) {
        if (newCapacity <= capacity_) return;
        reallocate(newCapacity);
    }

    void reallocate(int newCapacity) {
        assert(newCapacity >= size_);
        void* raw = std::malloc(static_cast<size_t>(newCapacity) * sizeof(T));
        if (!raw) throw std::bad_alloc();
        T* newData = static_cast<T*>(raw);

        if (data_) {
            if constexpr (std::is_move_constructible<T>::value) {
                for (int i = 0; i < size_; ++i) {
                    ::new (static_cast<void*>(newData + i)) T(std::move(data_[i]));
                    data_[i].~T();
                }
            }
            else {
                for (int i = 0; i < size_; ++i) {
                    ::new (static_cast<void*>(newData + i)) T(data_[i]);
                    data_[i].~T();
                }
            }
            std::free(data_);
        }

        data_ = newData;
        capacity_ = newCapacity;
    }

    void construct_at(int index, const T& value) {
        ::new (static_cast<void*>(data_ + index)) T(value);
    }

    void construct_at(int index, T&& value) {
        ::new (static_cast<void*>(data_ + index)) T(std::move(value));
    }

    void destroy_at(int index) {
        data_[index].~T();
    }

    void clear_and_free() {
        if (data_) {
            for (int i = 0; i < size_; ++i) {
                data_[i].~T();
            }
            std::free(data_);
            data_ = nullptr;
        }
        size_ = 0;
        capacity_ = 0;
    }
};

struct TestStruct {
    int x;
    TestStruct() : x(0) { /*std::cout<<"default\n";*/ }
    TestStruct(int v) : x(v) { /*std::cout<<"ctor\n";*/ }
    TestStruct(const TestStruct& o) : x(o.x) { /*std::cout<<"copy\n";*/ }
    TestStruct(TestStruct&& o) noexcept : x(o.x) { o.x = 0; /*std::cout<<"move\n";*/ }
    TestStruct& operator=(const TestStruct& o) { x = o.x; return *this; }
    TestStruct& operator=(TestStruct&& o) noexcept { x = o.x; o.x = 0; return *this; }
};

int main() {
    Array<int> a;
    for (int i = 0; i < 10; ++i) a.insert(i + 1);

    for (int i = 0; i < a.size(); ++i) a[i] *= 2;

    std::cout << "Forward iteration:\n";
    for (auto it = a.iterator(); it.hasNext(); it.next()) {
        std::cout << it.get() << ' ';
    }
    std::cout << "\n";

    std::cout << "Reverse iteration:\n";
    for (auto it = a.reverseIterator(); it.hasNext(); it.next()) {
        std::cout << it.get() << ' ';
    }
    std::cout << "\n";

    a.insert(3, 999);
    std::cout << "After inserting 999 at index 3:\n";
    for (int i = 0; i < a.size(); ++i) std::cout << a[i] << ' ';
    std::cout << "\n";

    a.remove(3);
    std::cout << "After removing index 3:\n";
    for (int i = 0; i < a.size(); ++i) std::cout << a[i] << ' ';
    std::cout << "\n";

    Array<TestStruct> arr;
    for (int i = 0; i < 5; ++i) arr.insert(TestStruct(i + 10));
    std::cout << "TestStruct values:\n";
    for (auto it = arr.iterator(); it.hasNext(); it.next()) {
        std::cout << it.get().x << ' ';
    }
    std::cout << "\n";

    Array<int> copied = a;
    std::cout << "Copied array:\n";
    for (int i = 0; i < copied.size(); ++i) std::cout << copied[i] << ' ';
    std::cout << "\n";

    Array<int> moved = std::move(copied);
    std::cout << "Moved array (from copied):\n";
    for (int i = 0; i < moved.size(); ++i) std::cout << moved[i] << ' ';
    std::cout << "\n";

    std::cout << "Range-for support:\n";
    for (int v : moved) std::cout << v << ' ';
    std::cout << "\n";

    std::cout << "All demo operations completed successfully.\n";
    return 0;
}
