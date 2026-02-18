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

std::string GenerateHTML(const status::Status& status)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html lang=\"en\">\n"
         << "<head>\n"
         << "    <meta charset=\"UTF-8\">\n"
         << "    <meta http-equiv=\"refresh\" content=\"10\">\n"
         << "    <title>System Status</title>\n"
         << "    <style>\n"
         << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
         << "        table { border-collapse: collapse; width: 100%; max-width: 800px; }\n"
         << "        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n"
         << "        th { background-color: #f2f2f2; }\n"
         << "        .progress { width: 100%; background-color: #f1f1f1; border-radius: 4px; }\n"
         << "        .progress-bar { height: 20px; background-color: #4CAF50; border-radius: 4px; text-align: center; "
            "color: white; line-height: 20px; }\n"
         << "    </style>\n"
         << "</head>\n"
         << "<body>\n"
         << "    <h1>System Status (auto-refresh every 10 seconds)</h1>\n"
         << "    <table>\n"
         << "        <tr><th>Field</th><th>Value</th></tr>\n"
         << "        <tr><td>Serial Number</td><td>" << int128_to_string(status.snum) << "</td></tr>\n"
         << "        <tr><td>Max Serial Number</td><td>" << int128_to_string(status.max_sn) << "</td></tr>\n"
         << "        <tr><td>Done Percent</td><td>" << std::fixed << std::setprecision(2) << status.done_percent
         << "%</td></tr>\n"
         << "        <tr><td>Elapsed Time</td><td>" << status.elapsed.count() << " s</td></tr>\n"
         << "        <tr><td>Remaining Time</td><td>" << status.remaining.count() << " s</td></tr>\n"
         << "        <tr><td>Iterations/sec</td><td>" << status.iterations_per_sec << "</td></tr>\n"
         << "        <tr><td>Total Iterations</td><td>" << status.iterations_count << "</td></tr>\n"
         << "        <tr><td>Current Function</td><td>" << status.current_function << "</td></tr>\n"
         << "    </table>\n"
         << "    <h2>Best Functions</h2>\n"
         << "    <table>\n"
         << "        <tr><th>Distance</th><th>Max Level</th><th>Functions Count</th><th>Functions "
            "Unique</th><th>Function</th><th>Match Positions</th></tr>\n";

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
         << "    <div class=\"progress\">\n"
         << "        <div class=\"progress-bar\" style=\"width:" << status.done_percent << "%;\">" << std::fixed
         << std::setprecision(1) << status.done_percent << "%</div>\n"
         << "    </div>\n"
         << "</body>\n"
         << "</html>\n";

    return html.str();
}

// Create HTTP server instance.
httplib::Server svr;
std::thread svr_thread;

void ServerThread(const status::Status& status, const std::string& host, int port)
{
    // Serve the main page at root.
    svr.Get("/", [&status](const httplib::Request& /*req*/, httplib::Response& res)
            { res.set_content(GenerateHTML(status), "text/html"); });

    // Optional: serve a simple JSON endpoint for AJAX (not required by spec, but handy)
    svr.Get("/status",
            [&status](const httplib::Request& /*req*/, httplib::Response& res)
            {
                // For brevity, we return a minimal JSON. In a real app, you'd use a JSON library.
                std::string json = "{\"snum\":\"" + int128_to_string(status.snum) +
                                   "\","
                                   "\"done_percent\":" +
                                   std::to_string(status.done_percent) + "}";
                res.set_content(json, "application/json");
            });

    std::println("Server starting on http://{}:{}", host, port);

    // Run the server on port 8080.
    // This blocks until the server is stopped.
    svr.listen(host, port);
}

void Run(const status::Status& status, const std::string& host, int port)
{
    svr_thread = std::thread(ServerThread, std::ref(status), host, port);
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