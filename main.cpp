#include <iostream>
#include <future>
#include <chrono>
#include <cxxabi.h>

#define LOG if (::logger::m_level == ::logger::level::VERBOSE) std::puts(__PRETTY_FUNCTION__);

namespace logger
{
    enum class level
    {
        NONE = 0, VERBOSE = 1
    };
    static ::logger::level m_level = ::logger::level::NONE;
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
            LOG;
        }

        helper(const helper &other) :
                f(other.f), w(other.w)
        {
            LOG;
        }

        helper(helper &&other) noexcept :
                f(std::move(other.f)), w(std::move(other.w))
        {
            LOG;
        }

        helper &operator=(helper other)
        {
            LOG;
            f = std::move(other.f);
            w = std::move(other.w);
            return *this;
        }

        auto operator()() -> decltype(w(std::move(f)))
        {
            LOG;
            f.wait();
            return w(std::move(f));
        }
    };
}

template<typename Future, typename Worker>
auto then(Future m_future, Worker worker) -> std::future<decltype(worker(std::forward<Future>(m_future)))>
{
    LOG;
    return std::async(std::launch::async,
                      detail::helper<Future, Worker>(std::forward<Future>(m_future), std::forward<Worker>(worker)));
}

template<class T, class... Args>
std::future<T> make_ready_future(Args &&... args)
{
    std::promise<T> promise;
    promise.set_value(T(std::forward<Args>(args)...));
    return promise.get_future();
}

template<class T>
std::future<T> make_ready_future(T t)
{
    std::promise<T> promise;
    promise.set_value(std::move(t));
    return promise.get_future();
}

std::future<std::tuple<>> when_all()
{
    return make_ready_future<std::tuple<>>();
}

template<class Head, class... Tails>
auto when_all(Head &&head,
              Tails &&... tails) -> std::future<std::tuple<typename std::decay<Head>::type, typename std::decay<Tails>::type...>>
{
    auto wait_on_tail = when_all(std::move(tails)...);
    return then(std::forward<Head>(head), [t = std::move(wait_on_tail)](Head h) mutable
    {
        return tuple_cat(make_tuple(std::move(h)), t.get());
    });
}

int task()
{
    LOG;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 42;
}

std::string task_s()
{
    LOG;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return "42";
}

float task_f()
{
    LOG;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 42.0f;
}

template<typename T>
void solution_1()
{
    LOG;
    auto fut1 = reallyAsync(task);
    auto fut2 = reallyAsync(task);

    auto f2 = then(std::move(fut1), [](std::future<decltype(fut1.get())> f)
    {
        return f.get() * 2;
    });

    std::cout << "res: " << f2.get() << std::endl;
}

template<typename T>
void solution_2()
{
    LOG;
    auto fut1 = reallyAsync(task);
    auto fut2 = reallyAsync(task);

    auto fut_res = when_all(std::move(fut1), std::move(fut2));
    auto t = fut_res.get();
    auto f1 = then(std::move(std::get<0>(t)), [](std::future<decltype(fut1.get())> f)
    {
        return f.get() * 2;
    });
    std::cout << "res: " << f1.get() << std::endl;
    auto f2 = then(std::move(std::get<1>(t)), [](std::future<decltype(fut2.get())> f)
    {
        return f.get() * 2;
    });
    std::cout << "res: " << f2.get() << std::endl;
}

int main(int argc, const char **argv)
{
    LOG;
//    solution_1<void>();
    solution_2<void>();
    return 0;
}
