#include <iostream>
#include <future>
#include <chrono>

#define LOG if (::logger::m_level == ::logger::level::VERBOSE) std::puts(__PRETTY_FUNCTION__);

namespace logger
{
    enum class level
    {
        NONE = 0, VERBOSE = 1
    };
    static ::logger::level m_level = ::logger::level::VERBOSE;
}

// Scott Meyers docet
template<typename F, typename... Ts>
inline auto reallyAsync(F &&f, Ts &&... params) -> decltype(std::async(std::launch::async, std::forward<F>(f),
                                                                       std::forward<Ts>(params)...))
{
    LOG;
    return std::async(std::launch::async, std::forward<F>(f), std::forward<Ts>(params)...);
}

// https://stackoverflow.com/a/14200861/2794395 (ronag)
namespace detail
{
    template<typename F, typename W>
    struct helper
    {
        F f;
        W w;

        helper(F f, W w) :
                f(std::move(f)), w(std::move(w))
        {
            if (::logger::m_level == ::logger::level::VERBOSE)
            { std::puts(__PRETTY_FUNCTION__); }
        }

        helper(const helper &other) :
                f(other.f), w(other.w)
        {
            if (::logger::m_level == ::logger::level::VERBOSE)
            { std::puts(__PRETTY_FUNCTION__); }
        }

        helper(helper &&other) noexcept :
                f(std::move(other.f)), w(std::move(other.w))
        {
            if (::logger::m_level == ::logger::level::VERBOSE)
            { std::puts(__PRETTY_FUNCTION__); }
        }

        helper &operator=(helper other)
        {
            if (::logger::m_level == ::logger::level::VERBOSE)
            { std::puts(__PRETTY_FUNCTION__); }
            f = std::move(other.f);
            w = std::move(other.w);
            return *this;
        }

        auto operator()() -> decltype(w(std::move(f)))
        {
            if (::logger::m_level == ::logger::level::VERBOSE)
            { std::puts(__PRETTY_FUNCTION__); }
            f.wait();
            return w(std::move(f));
        }
    };
}

template<typename Worker, typename Future>
auto then(Future m_future, Worker worker) -> std::future<decltype(worker(std::forward<Future>(m_future)))>
{
    LOG;
    return std::async(std::launch::async,
            detail::helper<Future, Worker>(std::forward<Future>(m_future), std::forward<Worker>(worker)));
}

int task()
{
    LOG;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 42;
}

template<typename T>
void solution_1()
{
    LOG;
    std::future<int> fut1 = reallyAsync(task);
    std::future<int> fut2 = reallyAsync(task);

    auto f2 = then(std::move(fut1), [](std::future<int> f)
    {
        return f.get() * 2;
    });

    std::cout << "res: " << f2.get() << std::endl;
}

int main(int argc, const char **argv)
{
    LOG;
    solution_1<void>();
    return 0;
}
