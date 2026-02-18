#pragma once

#include "third_party/cpp-httplib/httplib.h"

#include "status.h"

namespace fw
{

namespace interaction_http
{

std::string int128_to_string(__int128 value)
{
    if (value == 0)
        return "0";
    bool negative = (value < 0);
    if (negative)
        value = -value;

    std::string result;
    while (value > 0) {
        result.insert(result.begin(), static_cast<char>('0' + (value % 10)));
        value /= 10;
    }
    if (negative)
        result.insert(result.begin(), '-');
    return result;
}

/**
 * Generates an HTML page displaying the current system status.
 * The page auto-refreshes every 10 seconds and includes a progress bar,
 * a stop button, and a table of the best functions found so far.
 *
 * @param status The current status of the system.
 * @return A string containing the complete HTML5 document.
 */
std::string GenerateHTML(const status::Status& status)
{
    std::ostringstream html;

    // Lambda to format a std::chrono::nanoseconds duration as HH:MM:SS.
    // Hours can have an arbitrary number of digits (e.g., H, HH, HHH, ...).
    auto format_duration = [](std::chrono::nanoseconds d) -> std::string
    {
        using namespace std::chrono;
        auto h = duration_cast<hours>(d);
        d -= h;
        auto m = duration_cast<minutes>(d);
        d -= m;
        auto s = duration_cast<seconds>(d);
        std::ostringstream oss;
        oss << h.count() << ':' << std::setw(2) << std::setfill('0') << m.count() << ':' << std::setw(2)
            << std::setfill('0') << s.count();
        return oss.str();
    };

    html << "<!DOCTYPE html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <meta http-equiv=\"refresh\" content=\"10\">\n"  // Auto-refresh every 10 seconds
         << "    <title>System Status</title>\n"
         << "    <style>\n"
         << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
         << "        table { border-collapse: collapse; width: 100%; max-width: 800px; }\n"
         << "        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
         << "        th { background-color: #f2f2f2; }\n"
         << "        .progress { width: 100%; background-color: #f1f1f1; border-radius: 4px; margin: 10px 0; }\n"
         << "        .progress-bar { height: 20px; background-color: #4CAF50; border-radius: 4px; text-align: center; "
            "color: white; line-height: 20px; }\n"
         << "        button { padding: 8px 16px; font-size: 16px; cursor: pointer; }\n"
         << "    </style>\n"
         << "    <script>\n"
         << "        function stopProcess() {\n"
         << "            fetch('/stop', { method: 'POST' })\n"
         << "                .then(response => {\n"
         << "                    if (response.ok) {\n"
         << "                        alert('Process stopped');\n"
         << "                    } else {\n"
         << "                        alert('Failed to stop process');\n"
         << "                    }\n"
         << "                })\n"
         << "                .catch(error => alert('Error: ' + error));\n"
         << "            return false; // Prevent any default action\n"
         << "        }\n"
         << "    </script>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <h1>System Status (auto-refresh every 10 seconds)</h1>\n"

         // Main information table
         << "    <table>\n"
         << "        <tr><th>Field</th><th>Value</th></tr>\n"
         << "        <tr><td>Serial Number</td><td>" << int128_to_string(status.snum) << "</td></tr>\n"
         << "        <tr><td>Max Serial Number</td><td>" << int128_to_string(status.max_sn) << "</td></tr>\n"
         << "        <tr><td>Done Percent</td><td>" << std::fixed << std::setprecision(2) << status.done_percent
         << "%</td></tr>\n"
         << "        <tr><td>Elapsed Time</td><td>" << format_duration(status.elapsed) << "</td></tr>\n"
         << "        <tr><td>Remaining Time</td><td>" << format_duration(status.remaining) << "</td></tr>\n"
         << "        <tr><td>Iterations/sec</td><td>" << status.iterations_per_sec << "</td></tr>\n"
         << "        <tr><td>Total Iterations</td><td>" << status.iterations_count << "</td></tr>\n"
         << "        <tr><td>Current Function</td><td>" << status.current_function << "</td></tr>\n"
         << "    </table>\n"

         // Progress bar (placed above the best functions table)
         << "    <div class=\"progress\">\n"
         << "        <div class=\"progress-bar\" style=\"width:" << status.done_percent << "%;\">" << std::fixed
         << std::setprecision(1) << status.done_percent << "%</div>\n"
         << "    </div>\n"

         // Stop button – sends an asynchronous POST request, stays on the same page
         << "    <button onclick=\"return stopProcess();\" style=\"margin: 10px 0;\">Stop Process</button>\n"

         // Best functions table header
         << "    <h2>Best Functions</h2>\n"
         << "    <table>\n"
         << "        <tr><th>Distance</th><th>Max Level</th><th>Functions Count</th><th>Functions "
            "Unique</th><th>Function</th><th>Match Positions</th></tr>\n";

    // Fill the best functions table
    for (const auto& bf : status.best_functions) {
        html << "        <tr>"
             << "<td>" << bf.suit.distance() << "</td>"
             << "<td>" << bf.suit.max_level() << "</td>"
             << "<td>" << bf.suit.functions_count() << "</td>"
             << "<td>" << bf.suit.functions_unique() << "</td>"
             << "<td>" << bf.function << "</td>"
             << "<td>" << bf.match_positions << "</td>"
             << "</tr>\n";
    }

    html << "    </table>\n"
         << "</body>\n"
         << "</html>\n";

    return html.str();
}

// Create HTTP server instance.
httplib::Server svr;
std::thread svr_thread;

/**
 * Runs an HTTP server that serves the system status page and provides an endpoint
 * to stop the background process.
 *
 * @param status     Constant reference to the current status (read‑only for the server).
 * @param stop_flag  Atomic flag that can be set to true via POST /stop to request termination.
 * @param host       Host address to bind to (e.g., "localhost").
 * @param port       Port number to listen on.
 */
void ServerThread(const status::Status& status, std::atomic<bool>& stop_flag, const std::string& host, int port)
{
    // Serve the main HTML page at the root.
    svr.Get("/", [&status](const httplib::Request& /*req*/, httplib::Response& res)
            { res.set_content(GenerateHTML(status), "text/html"); });

    // Optional JSON endpoint for AJAX requests (e.g., to fetch fresh data without reloading).
    svr.Get("/status",
            [&status](const httplib::Request& /*req*/, httplib::Response& res)
            {
                // In a real application you would use a proper JSON library.
                std::string json = "{\"snum\":\"" + int128_to_string(status.snum) +
                                   "\",\"done_percent\":" + std::to_string(status.done_percent) + "}";
                res.set_content(json, "application/json");
            });

    // Endpoint to stop the background process.
    // This is called asynchronously by the front‑end button via JavaScript fetch().
    svr.Post("/stop",
             [&stop_flag](const httplib::Request& /*req*/, httplib::Response& res)
             {
                 stop_flag.store(true);  // Signal the worker thread to stop
                 res.status = 200;       // OK
                 res.set_content("{\"result\":\"stopped\"}", "application/json");
             });

    std::println("Server starting on http://{}:{}", host, port);
    svr.listen(host, port);  // Blocks until the server is stopped
}

void Run(const status::Status& status, std::atomic<bool>& stop_flag, const std::string& host, int port)
{
    svr_thread = std::thread(ServerThread, std::ref(status), std::ref(stop_flag), host, port);
}

void Stop()
{
    svr.stop();
    if (svr_thread.joinable()) {
        svr_thread.join();
    }
}

}  // namespace interaction_http

}  // namespace fw