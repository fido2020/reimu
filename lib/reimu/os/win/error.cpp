#include <string>
#include <cstring>

namespace reimu {

std::string platform_err_to_str(int err_no) {
    char err_buf[512];
    strerror_s(err_buf, sizeof(err_buf), err_no);
    return std::string(err_buf);
}

}