#ifndef DATABASE_H
#define DATABASE_H

#include <pqxx/pqxx>
#include <string>

class Database
{
    pqxx::connection *conn;

public:
    Database(const std::string &conninfo)
    {
        conn = new pqxx::connection(conninfo);
        pqxx::work W(*conn);
        W.exec("CREATE TABLE IF NOT EXISTS kv_store (key TEXT PRIMARY KEY, value TEXT);");
        W.commit();
    }

    bool insert_or_update(const std::string &key, const std::string &value)
    {
        pqxx::work W(*conn);
        W.exec_params("INSERT INTO kv_store (key,value) VALUES ($1,$2) "
                      "ON CONFLICT (key) DO UPDATE SET value=$2;",
                      key, value);
        W.commit();
        return true;
    }

    bool get(const std::string &key, std::string &value)
    {
        pqxx::nontransaction N(*conn);
        pqxx::result R = N.exec_params("SELECT value FROM kv_store WHERE key=$1;", key);
        if (R.empty())
            return false;
        value = R[0][0].as<std::string>();
        return true;
    }

    bool remove(const std::string &key)
    {
        pqxx::work W(*conn);
        W.exec_params("DELETE FROM kv_store WHERE key=$1;", key);
        W.commit();
        return true;
    }

    ~Database()
    {
        delete conn;
    }
};

#endif
