#include <reimu/os/fs.h>

#include <wayland-server.h>

#include <string>
#include <format>

#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>

#include "gpu.h"

int main() {
    const char *sessionId = getenv("REIMU_SESSION");
    if (!sessionId) {
        fprintf(stderr, "Invalid session ID");
        return 1;
    }

    std::string server_path = "/var/run/reimu/";
    std::string socket_name;
    if (strcmp(sessionId, "root") == 0) {
        socket_name = "root-wm";
    } else {
        server_path += "session/";
        server_path += sessionId;
        socket_name = "wm";
    }

    auto res = reimu::os::make_path(server_path, 0755);
    if (res.is_err()) {
        puts(std::format("make_path: {}", res.move_err()).c_str());
        return 1;
    }

    std::string socket_path = server_path + socket_name;

    puts(std::format("opening display server at {}", socket_path).c_str());
    
    int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }

    struct wl_display *display = wl_display_create();
    if (!display) {
        // do something
        return 1;
    }

    if(wl_display_add_socket_fd(display, socket_fd)) {
        printf("Failed to add socket fd to display");
        return 1;
    }

    GPU *gpu = GPU::open_first_gpu().ensure();

    return 0;
}
