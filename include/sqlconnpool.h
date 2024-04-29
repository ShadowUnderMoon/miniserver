#pragma once
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <mysql++/mysql++.h>
#include <mutex>
#include <condition_variable>

class MysqlConnectionPool;
class MySqlScopedConnection {
  public:
    explicit MySqlScopedConnection(MysqlConnectionPool &pool);

    ~MySqlScopedConnection();

    MySqlScopedConnection(MySqlScopedConnection &&) = default;
    MySqlScopedConnection(const MySqlScopedConnection &) = delete;
    MySqlScopedConnection &operator=(const MySqlScopedConnection &) = delete;
    mysqlpp::Connection *operator->() const { return conn_; }

    mysqlpp::Connection &operator*() const { return *conn_; }

  private:
    MysqlConnectionPool &pool_;
    mysqlpp::Connection *conn_;
};

class MysqlConnectionPool {
  public:
    MysqlConnectionPool(
        std::string db_name,
        std::string server,
        std::string user,
        std::string password,
        unsigned int port,
        std::chrono::seconds max_idle_time = std::chrono::seconds(3600),
        int initial_connections = 2,
        int max_connections = 10)
        : db_name_(std::move(db_name))
        , server_(std::move(server))
        , user_(std::move(user))
        , password_(std::move(password))
        , port_(port)
        , max_idle_time_(max_idle_time)
        , initial_connections_(initial_connections)
        , max_connections_(max_connections) {
        for (int i = 0; i < initial_connections; i++) {
            pool_.push_back(ConnectionInfo(Create()));
        }
        available_connections_ = initial_connections;
    }

    ~MysqlConnectionPool() {}

    mysqlpp::Connection *Grab() {
        std::unique_lock<std::mutex> lock(mutex_);
        RemoveOldConnections_();
        cv_.wait(lock, [this] {
            return available_connections_ > 0
                   || pool_.size() < max_connections_;
        });
        if (mysqlpp::Connection *mru = FindMru()) {
            available_connections_--;
            return mru;
        }
        if (pool_.size() < max_connections_) {
            pool_.push_back(ConnectionInfo(Create()));
            pool_.back().in_use = true;
            return pool_.back().conn.get();
        }
        return nullptr;
    }

    void Release(const mysqlpp::Connection *connection) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            for (auto it = pool_.begin(); it != pool_.end(); it++) {
                if (it->conn.get() == connection) {
                    it->in_use = false;
                    it->last_used = std::chrono::system_clock::now();
                    available_connections_++;
                    break;
                }
            }
        }
        cv_.notify_one();
    }

    size_t Size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return pool_.size();
    }

    size_t AvailableConnections() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return available_connections_;
    }


    std::chrono::seconds MaxIdleTime() { return max_idle_time_; }

    // most recently used
    mysqlpp::Connection *FindMru() {
        auto mru = std::max_element(pool_.begin(), pool_.end());
        if (mru != pool_.end() && !mru->in_use) {
            mru->in_use = true;
            return mru->conn.get();
        }
        return nullptr;
    }

    void RemoveOldConnections_() {
        auto it = pool_.begin();
        while (it != pool_.end()) {
            if (std::chrono::system_clock::now() - it->last_used
                >= MaxIdleTime()) {
                if (!it->in_use) { available_connections_--; }
                pool_.erase(it++);
            } else {
                it++;
            }
        }
    }

  private:
    std::unique_ptr<mysqlpp::Connection> Create() {
        return std::make_unique<mysqlpp::Connection>(
            db_name_.data(), server_.data(), user_.data(), password_.data(),
            port_);
    }

    struct ConnectionInfo {
        std::unique_ptr<mysqlpp::Connection> conn;
        std::chrono::system_clock::time_point last_used =
            std::chrono::system_clock::now();
        bool in_use = false;

        ConnectionInfo(std::unique_ptr<mysqlpp::Connection> connection)
            : conn(std::move(connection)) {}
        ConnectionInfo(const ConnectionInfo &) = delete;
        ConnectionInfo &operator=(const ConnectionInfo &) = delete;
        ConnectionInfo(ConnectionInfo &&) = default;
        ConnectionInfo &operator=(ConnectionInfo &&) = default;
        bool operator<(const ConnectionInfo &rhs) const {
            const ConnectionInfo &lhs = *this;
            return lhs.in_use == rhs.in_use ? lhs.last_used < rhs.last_used
                                            : lhs.in_use;
        }
    };

    std::string db_name_;
    std::string server_;
    std::string user_;
    std::string password_;
    unsigned int port_;
    std::chrono::seconds max_idle_time_;
    int initial_connections_ = 0;
    int max_connections_ = 0;
    int available_connections_ = 0;
    std::list<ConnectionInfo> pool_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

inline MySqlScopedConnection::MySqlScopedConnection(MysqlConnectionPool &pool)
    : pool_(pool), conn_(pool.Grab()) {
}

inline MySqlScopedConnection::~MySqlScopedConnection() {
    pool_.Release(conn_);
}