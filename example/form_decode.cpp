#include <iostream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <iomanip>

std::string url_decode(const std::string& str) {
    std::ostringstream decoded;
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int value;
            std::istringstream hex(str.substr(i + 1, 2));
            hex >> std::hex >> value;
            decoded << static_cast<char>(value);
            i += 2;
        } else if (str[i] == '+') {
            decoded << ' ';
        } else {
            decoded << str[i];
        }
    }
    return decoded.str();
}

std::unordered_map<std::string, std::string> parse_form_data(const std::string& data) {
    std::unordered_map<std::string, std::string> result;
    std::istringstream iss(data);
    std::string pair;
    while (std::getline(iss, pair, '&')) {
        std::size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            key = url_decode(key);
            value = url_decode(value);
            result[key] = value;
        }
    }
    return result;
}

int main() {
    // 示例字符串
    std::string data = "username=user123&password=pass%40word";

    // 解析字符串
    std::unordered_map<std::string, std::string> parsed_data = parse_form_data(data);

    // 打印解析结果
    for (const auto& pair : parsed_data) {
        std::cout << pair.first << " = " << pair.second << std::endl;
    }

    return 0;
}
