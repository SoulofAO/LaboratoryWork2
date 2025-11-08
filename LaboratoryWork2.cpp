#include <cassert>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>
#include <cmath>
#include <iostream>
#include <gtest/gtest.h>


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

    explicit Array(int capacity)
        : data_(nullptr), size_(0), capacity_(0)
    {
        reserve(capacity > 0 ? capacity : default_initial_capacity);
    }

    ~Array() {
        clear_and_free();
    }

    //Copyswap
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

    Array(Array&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

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

    int insert(const T& value) {
        ensure_capacity_for_one_more();
        construct_at(size_, value);
        ++size_;
        return size_ - 1;
    }

	//WHY WE SHOULD RETURN INDEX? OLEG FROM FUTURE, ASK THE TEACHER ABOUT IT.
    int insert(int index, const T& value) {
        assert(index >= 0 && index <= size_);
        ensure_capacity_for_one_more();

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
        //!
        if (!raw) throw std::bad_alloc();
        T* newData = static_cast<T*>(raw);

        if (data_) {
            if constexpr (std::is_move_constructible<T>::value) {
                for (int i = 0; i < size_; ++i) {
                    ::new (newData + i) T(std::move(data_[i]));
                    data_[i].~T();
                }
            }
            else {
                for (int i = 0; i < size_; ++i) {
                    ::new (newData + i) T(data_[i]);
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


TEST(ArrayTest, InsertAndModifyInts) {
    Array<int> A;
    for (int i = 0; i < 10; ++i) {
        A.insert(i + 1);
    }
    ASSERT_EQ(A.size(), 10);
    for (int i = 0; i < A.size(); ++i) {
        A[i] *= 2;
    }
    ASSERT_EQ(A.size(), 10);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(A[i], (i + 1) * 2);
    }
}

static std::vector<int> CollectForward(Array<int>& A) {
    std::vector<int> Out;
    for (auto It = A.iterator(); It.hasNext(); It.next()) {
        Out.push_back(It.get());
    }
    return Out;
}

static std::vector<int> CollectReverse(Array<int>& A) {
    std::vector<int> Out;
    for (auto It = A.reverseIterator(); It.hasNext(); It.next()) {
        Out.push_back(It.get());
    }
    return Out;
}

//Добавить =

TEST(ArrayTest, ForwardAndReverseIteration) {
    Array<int> A;
    for (int i = 0; i < 10; ++i) {
        A.insert(i + 1);
    }
    for (int i = 0; i < A.size(); ++i) {
        A[i] *= 2;
    }
    std::vector<int> Forward = CollectForward(A);
    std::vector<int> ExpectedForward;
    for (int i = 0; i < 10; ++i) ExpectedForward.push_back((i + 1) * 2);
    EXPECT_EQ(Forward, ExpectedForward);
    std::vector<int> Reverse = CollectReverse(A);
    std::reverse(ExpectedForward.begin(), ExpectedForward.end());
    EXPECT_EQ(Reverse, ExpectedForward);
}

TEST(ArrayTest, InsertAtIndexAndRemove) {
    Array<int> A;
    for (int i = 0; i < 10; ++i) {
        A.insert(i + 1);
    }
    int InsertedIndex = A.insert(3, 999);
    EXPECT_EQ(InsertedIndex, 3);
    ASSERT_EQ(A.size(), 11);
    EXPECT_EQ(A[3], 999);
    Array<int> Snapshot = A;
    A.remove(3);
    ASSERT_EQ(A.size(), 10);
    for (int i = 0; i < A.size(); ++i) {
        EXPECT_EQ(A[i], Snapshot[i < 3 ? i : i + 1]);
    }
}

TEST(ArrayTest, TestStructValues) {
    Array<TestStruct> Arr;
    for (int i = 0; i < 5; ++i) {
        Arr.insert(TestStruct(i + 10));
    }
    ASSERT_EQ(Arr.size(), 5);
    std::vector<int> Collected;
    for (auto It = Arr.iterator(); It.hasNext(); It.next()) {
        Collected.push_back(It.get().x);
    }
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(Collected[i], i + 10);
    }
}

TEST(ArrayTest, CopyAndMoveSemanticsAndRangeFor) {
    Array<int> A;
    for (int i = 0; i < 10; ++i) {
        A.insert(i + 1);
    }
    for (int i = 0; i < A.size(); ++i) {
        A[i] *= 2;
    }
    Array<int> Copied = A;
    ASSERT_EQ(Copied.size(), A.size());
    for (int i = 0; i < A.size(); ++i) {
        EXPECT_EQ(Copied[i], A[i]);
    }
    Array<int> Moved = std::move(Copied);
    EXPECT_EQ(Moved.size(), A.size());
    EXPECT_EQ(Copied.size(), 0);
    std::vector<int> RangeForCollected;
    for (int V : Moved) {
        RangeForCollected.push_back(V);
    }
    std::vector<int> Expected;
    for (int i = 0; i < 10; ++i) Expected.push_back((i + 1) * 2);
    EXPECT_EQ(RangeForCollected, Expected);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
