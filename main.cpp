#include "httplib.h"
#include "LRUCache.h"
#include "Database.h"
#include <iostream>
#include <thread>

int main()
{
    const int PORT = 8080;
    const size_t CACHE_SIZE = 100;
    const std::string DB_CONN = "dbname=testdb user=postgres password=postgres host=localhost";

    LRUCache cache(CACHE_SIZE);
    Database db(DB_CONN);
    httplib::Server svr;

    svr.Post("/kv", [&](const httplib::Request &req, httplib::Response &res)
             {
        auto key = req.get_param_value("key");
        auto value = req.get_param_value("value");
        db.insert_or_update(key, value);
        cache.put(key, value);
        res.set_content("Inserted key=" + key, "text/plain"); });

    svr.Get("/kv", [&](const httplib::Request &req, httplib::Response &res)
            {
        auto key = req.get_param_value("key");
        std::string value;
        if (cache.get(key, value)) {
            res.set_content("CacheHit: " + value, "text/plain");
            return;
        }
        if (db.get(key, value)) {
            cache.put(key, value);
            res.set_content("DBHit: " + value, "text/plain");
        } else {
            res.status = 404;
            res.set_content("Key not found", "text/plain");
        } });

    svr.Delete("/kv", [&](const httplib::Request &req, httplib::Response &res)
               {
        auto key = req.get_param_value("key");
        db.remove(key);
        cache.erase(key);
        res.set_content("Deleted key=" + key, "text/plain"); });

    std::cout << "Server running on port " << PORT << "..." << std::endl;
    svr.new_task_queue = []
    { return new httplib::ThreadPool(8); };
    svr.listen("0.0.0.0", PORT);
    return 0;
}
