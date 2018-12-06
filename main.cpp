#include <iostream>
#include <future>
#include <chrono>

// Scott Meyers docet
template<typename F, typename... Ts>
inline auto reallyAsync(F &&f, Ts &&... params) -> decltype(std::async(std::launch::async, std::forward<F>(f),
                                                                       std::forward<Ts>(params)...))
{
    std::puts(__PRETTY_FUNCTION__);
    return std::async(std::launch::async, std::forward<F>(f), std::forward<Ts>(params)...);
}

// https://stackoverflow.com/a/14200861/2794395 (ronag)
namespace detail
{
    template<typename F, typename W, typename R>
    struct helper
    {
        F f;
        W w;

        helper(F f, W w) :
                f(std::move(f)), w(std::move(w))
        {
        }

        helper(const helper &other) :
                f(other.f), w(other.w)
        {
        }

        helper(helper &&other) noexcept :
                f(std::move(other.f)), w(std::move(other.w))
        {
        }

        helper &operator=(helper other)
        {
            f = std::move(other.f);
            w = std::move(other.w);
            return *this;
        }

        auto operator()() -> decltype(w(std::move(f)))
        {
            f.wait();
            return w(std::move(f));
        }
    };

}

template<typename F, typename W>
auto then(F f, W w) -> std::future<decltype(w(std::move(f)))>
{
    return std::async(std::launch::async, detail::helper<F, W, decltype(w(std::move(f)))>(std::move(f), std::move(w)));
}

int task()
{
    std::this_thread::sleep_for(std::chrono::seconds(3));
    return 42;
}

int main(int argc, const char **argv)
{
    std::future<int> fut1 = reallyAsync(task);
    std::future<int> fut2 = reallyAsync(task);

    auto f2 = then(std::move(fut1), [](std::future<int> f)
    {
        return f.get() * 2;
    });

    std::cout << "res: " << f2.get() << std::endl;

    return 0;
}