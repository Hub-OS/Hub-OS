#include <iostream>
#include <string>
#include <sstream>
#include <vector>
using namespace std;

#define SMART_ENUM(name, ...) enum class name { __VA_ARGS__, size}; \
const std::vector<std::string> name##_As_List = [] { \
std::string str = #__VA_ARGS__; \
int len = static_cast<int>(str.length()); \
std::vector<std::string> strings; \
std::ostringstream temp; \
for(int i = 0; i < len; i ++) { \
if(isspace(str[i])) continue; \
        else if(str[i] == ',') { \
        strings.push_back(temp.str()); \
        temp.str(std::string());\
        } \
        else temp<< str[i]; \
} \
strings.push_back(temp.str()); \
return strings; \
}();\
inline std::ostream& operator<<(std::ostream& os, name value) { \
os << #name << "::" << name##_As_List[static_cast<int>(value)]; \
return os;}