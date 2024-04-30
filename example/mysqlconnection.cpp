#include <chrono>
#include <mysql++/row.h>
#include <sqlconnpool.h>
#include <spdlog/spdlog.h>
#include <string>

int main() {
    auto& connpool = MysqlConnectionPool::GetInstance();
   connpool.Init(
        std::getenv("MINISERVER_DB_NAME"),
        std::getenv("MINISERVER_DB_HOST"),
        std::getenv("MINISERVER_DB_USER"),
        std::getenv("MINISERVER_DB_PASSWORD"),
        3306,
        std::chrono::seconds(3600),
        2,
        10
    );
    
    auto duration = std::chrono::seconds(3600);
    spdlog::info("duration: {} s", duration.count());
    spdlog::info("duration {} h", std::chrono::duration_cast<std::chrono::hours>(duration).count());
    spdlog::info("{}", std::chrono::seconds(3).count());
    spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());
    MySqlScopedConnection conn(connpool);

    spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());
    spdlog::info("pool size: {}", connpool.Size());
    mysqlpp::Query query = conn->query();
    std::string name = "zhangsan";
    query << "SELECT password FROM user WHERE username = " << mysqlpp::quote << name << " LIMIT 1";

    mysqlpp::StoreQueryResult result = query.store();

    if (result.num_rows() == 1) {
        mysqlpp::Row row = result[0];
        std::string password = std::string(row["password"]);
        spdlog::info("password: {}", password);
    }

    std::string pwd = "7384787374";
    query << "INSERT INTO user(username, password) VALUES(" << mysqlpp::quote << name
            << ", " << mysqlpp::quote << pwd << ")";
    if (query.execute()) {
        spdlog::info("insert success");
    }

    // MySqlScopedConnection conn2(connpool);
    // spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());
    // MySqlScopedConnection conn3(connpool);
    // spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());
    // MySqlScopedConnection conn4(connpool);
    // spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());
    // {
    //     MySqlScopedConnection conn5(connpool);
    //     spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());
    // }
    // spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());

    // sleep(10);

    // MySqlScopedConnection conn6(connpool);
    // spdlog::info("connpool status: {}, {}", connpool.AvailableConnections(), connpool.Size());
   
}