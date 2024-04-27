#include <utils.h>
#include <iostream>
#include <string>
using namespace std;
int main() {
    string s = "hello\r\nworld";
    cout << EscapeString(s) << endl;
}