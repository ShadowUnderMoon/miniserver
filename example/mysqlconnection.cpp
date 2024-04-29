#include <chrono>
#include <mysql++/row.h>
#include <sqlconnpool.h>
#include <spdlog/spdlog.h>
#include <string>

int main() {
    MysqlConnectionPool connpool(
        std::getenv("MINISERVER_DB_NAME"),
        std::getenv("MINISERVER_DB_HOST"),
        std::getenv("MINISERVER_DB_USER"),
        std::getenv("MINISERVER_DB_PASSWORD"),
        3306,
        std::chrono::seconds(5),
        2,
        10
    );

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