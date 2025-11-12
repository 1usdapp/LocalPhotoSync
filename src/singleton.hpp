#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>

namespace lps {

template<typename T>
class singleton : private boost::noncopyable
{
public:
    static T& instance()
    {
        std::call_once(once_flag_, &singleton::create_instance);
        return *instance_;
    }

    static T* instance_ptr()
    {
        std::call_once(once_flag_, &singleton::create_instance);
        return instance_.get();
    }

private:
    static void create_instance() { instance_.reset(new T()); }

    static std::unique_ptr<T> instance_;
    static std::once_flag once_flag_;
};

template<typename T>
std::unique_ptr<T> singleton<T>::instance_ = nullptr;
template<typename T>
std::once_flag singleton<T>::once_flag_;

}   // namespace lps

// 使用示例
// class MyClass {
// public:
//     void doSomething() { /* ... */ }
// };

// using MySingleton = lps::singleton<MyClass>;
// 使用: MySingleton::instance().doSomething();

#endif