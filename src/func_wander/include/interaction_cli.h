#pragma once

#include <atomic>
#include <csignal>
#include <print>

#include "func_node.h"
#include "interaction_http.h"
#include "search_manager.h"

namespace fw
{
std::atomic_bool g_stop = false;
std::atomic_bool g_graceful_stop = false;

void SignalHandler(int signal)
{
    std::println("got signal {}", signal);
    if (signal == SIGINT) {
        if (not g_graceful_stop) {
            std::println("terminating by Ctrl+C: graceful stop, press again to stop immediately");
            g_graceful_stop = true;
        }
        else {
            std::println("terminating by Ctrl+C: stop immediately");
            g_stop = true;
        }
    }
}

bool LoadTask(const Settings& settings, SearchManager<Value_t, true, true>& manager)
{
    if (settings.save_file.empty()) {
        return true;
    }

    const std::ifstream file(settings.save_file);
    if (file) {
        std::ostringstream buffer;
        buffer << file.rdbuf();
        const auto task_json = buffer.str();

        if (not manager.FromJSON(task_json)) {
            std::println("Failed to parse JSON from file: {}", settings.save_file);
            return false;
        }
        std::println("Loaded JSON from file: {}", settings.save_file);
    }
    else {
        std::println("Failed to open file: {}", settings.save_file);
    }

    return true;
}

bool SaveTask(const Settings& settings, SearchManager<Value_t, true, true>& manager)
{
    if (settings.save_file.empty()) {
        return true;
    }

    std::ofstream file(settings.save_file, std::ios::trunc);

    if (not file.is_open()) {
        std::println("Failed to open file: {}", settings.save_file);
        return false;
    }

    auto task_json = manager.ToJSON();
    file << task_json;

    std::println("Current status saved to {}", settings.save_file);

    return true;
}

template <typename TVal>
int MainLoop(const Settings& settings, AtomFuncs<TVal>& atoms, Target<TVal>& target)
{
    auto previous_handler = std::signal(SIGINT, SignalHandler);
    if (previous_handler == SIG_ERR) {
        std::println("Failed to set signal handler");
        return EXIT_FAILURE;
    }

    SearchManager<Value_t, true, true> manager{settings, &atoms, &target};

    if (not LoadTask(settings, manager)) {
        return EXIT_FAILURE;
    }

    status::Status status;

    if (settings.http_enabled) {
        interaction_http::Run(status, g_stop, settings.http_host, settings.http_port);
    }

    manager.Run();

    while (not g_stop) {
        constexpr auto PauseOutput = std::chrono::seconds(10);
        std::this_thread::sleep_for(PauseOutput);

        if (g_graceful_stop) {
            manager.StopGraceful();
        }

        if (manager.Done()) {
            g_stop = true;
        }

        status = manager.GetStatus();
        if (settings.http_enabled) {
            std::println("iterations_count={:12}; snum={} ({}); done={}", status.iterations_count,
                         status.func_serial_number, status.max_func_serial_number, status.done_percent);
        }
        else {
            std::println("{}", status.to_string());
        }
    }

    manager.Stop();

    if (settings.http_enabled) {
        interaction_http::Stop();
    }

    if (not SaveTask(settings, manager)) {
        return EXIT_FAILURE;
    }

    std::println("{}", manager.GetStatus().to_string());

    return EXIT_SUCCESS;
}

}  // namespace fw