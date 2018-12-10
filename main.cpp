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

// https://stackoverflow.com/a/46329595/2794395 (Quuxplusone)
template<class... Futures>
struct when_any_shared_state
{
    std::promise<std::tuple<Futures...>> m_promise;
    std::tuple<Futures...> m_tuple;
    std::atomic<bool> m_done;
    std::atomic<bool> m_count_to_two;

    explicit when_any_shared_state(std::promise<std::tuple<Futures...>> p) :
            m_promise(std::move(p)), m_done(false), m_count_to_two(false)
    {
        LOG;
    }
};

template<class... Futures>
auto when_any(Futures... futures) -> std::future<std::tuple<Futures...>>
{
    LOG;
    using R = std::tuple<Futures...>;
    using shared_state = when_any_shared_state<Futures...>;
    using future_type = std::tuple_element<0, std::tuple<Futures...>>;

    std::promise<R> p;
    std::future<R> result = p.get_future();

    auto sptr = std::make_shared<shared_state>(std::move(p));
    auto satisfy_combined_promise = [sptr](typename future_type::type f)
    {
        if (!sptr->m_done.exchange(true))
        {
            if (sptr->m_count_to_two.exchange(true))
            {
                sptr->m_promise.set_value(std::move(sptr->m_tuple));
            }
        }
        return f.get();
    };

    sptr->m_tuple = std::tuple<Futures...>(then(std::move(futures), satisfy_combined_promise)...);
    if (sptr->m_count_to_two.exchange(true))
    {
        sptr->m_promise.set_value(std::move(sptr->m_tuple));
    }
    return result;
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
    auto fut1 = reallyAsync(task_s);
    auto fut2 = reallyAsync(task_s);

    auto fut_res = when_any(std::move(fut1), std::move(fut2));
    std::puts(typeid(fut_res).name());
    int status;
    char *realname = abi::__cxa_demangle(typeid(fut_res).name(), 0, 0, &status);
    std::puts(realname);
    auto fut3 = then(std::move(std::get<0>(fut_res.get())), [](std::future<decltype(fut1.get())> f)
    {
        return " *** " + f.get() + " *** ";
    });

    std::cout << "res: " << fut3.get() << std::endl;
}

template<typename T>
void solution_3()
{
    LOG;
    auto fut1 = reallyAsync(task_f);
    auto fut2 = reallyAsync(task_f);

    auto fut_res = when_any(std::move(fut1), std::move(fut2));
    auto fut3 = then(std::move(std::get<0>(fut_res.get())), [](std::future<decltype(fut1.get())> f)
    {
        return f.get() * 2.00001;
    });

    std::cout << "res: " << fut3.get() << std::endl;
}

int main(int argc, const char **argv)
{
    LOG;
//    solution_1<void>();
    solution_2<void>();
//    solution_3<void>();
    return 0;
}
