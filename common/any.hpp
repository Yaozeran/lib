#pragma once


#include <memory>


/**
 * A class container which is able to container any type of data
 */
class Any {

private:

    /**
     * Use base class so that the pointer to base can point to any derived class
     *      i.e. store any derived class through base pointer
     */
    class Base {

    public:
        virtual ~Base() = default;
    };

    template <typename T>
    class Derived : public Base {

    private:
        T data;
    
    public:
        explicit Derived(const T& value) : data(value) {}
    };

    std::unique_ptr<Base> ptr; // pointer to base

public:
    
    template<typename T>
    Any(T data) : ptr(std::make_unique<Derived<T>>(data)) {}

};
