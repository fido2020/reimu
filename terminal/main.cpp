#define NTDDI_VERSION 0x0A000006
#define _WIN32_WINNT 0x0A00

#include <reimu/video/video.h>
#include <reimu/video/driver.h>
#include <reimu/core/event.h>
#include <reimu/core/unicode.h>
#include <reimu/gui/terminal.h>
#include <reimu/gui/window.h>
#include <reimu/gui/dialog.h>
#include <reimu/os/error.h>
#include <reimu/os/fs.h>

#include <list>

#ifdef __linux__

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <spawn.h>
#include <pty.h>

reimu::Result<int, reimu::OSError> os_open_pty(os_handle_t &master_in, os_handle_t &slave_in, os_handle_t &master_out, os_handle_t &slave_out) {
    int master_fd, slave_fd;
    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) < 0) {
        return ERR(reimu::OSError{errno});
    }

    master_in = master_fd;
    master_out = dup(master_fd);
    slave_in = slave_fd;
    slave_out = dup(slave_fd);

    return OK(master_fd);
}

reimu::Result<size_t, reimu::OSError> os_pty_read(os_handle_t handle, void *buffer, size_t size) {
    auto ret = read(handle, buffer, size);
    if (ret < 0) {
        return ERR(reimu::OSError{errno});
    }

    return OK(ret);
}

reimu::Result<int, reimu::OSError> os_create_process_pty(const char *path, char *const argv[], int pty,
        os_handle_t master_in, os_handle_t slave_in, os_handle_t master_out, os_handle_t slave_out) {
    (void)pty;

    int master_fd, slave_fd;
    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) < 0) {
        return ERR(reimu::OSError{errno});
    }

    pid_t pid = fork();
    if (pid < 0) {
        return ERR(reimu::OSError{errno});
    }

    if (pid == 0) {
        if (setsid() < 0) {
            reimu::logger::fatal("Failed to setsid: {}", strerror(errno));
        }

        // Child process
        close(master_in);
        close(master_out);

        if (dup2(slave_in, STDIN_FILENO) < 0) {
            reimu::logger::fatal("Failed to redirect stdin");
        }

        if (dup2(slave_out, STDOUT_FILENO) < 0) {
            reimu::logger::fatal("Failed to redirect stdout");
        }

        if (dup2(slave_out, STDERR_FILENO) < 0) {
            reimu::logger::fatal("Failed to redirect stderr");
        }

        if(ioctl(slave_in, TIOCSCTTY, 0)) {
            reimu::logger::fatal("Failed to set controlling TTY: {}", strerror(errno));
        }

        close(slave_in);
        close(slave_out);

        execvp(path, argv);
        return ERR(reimu::OSError{errno});
    }

    return OK(pid);
}

void os_pty_set_size(os_handle_t handle, int cols, int rows) {
    struct winsize ws;
    ws.ws_col = cols;
    ws.ws_row = rows;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    ioctl(handle, TIOCSWINSZ, &ws);
}

#elif defined(REIMU_WIN32)

#include <windows.h>
#include <wincon.h>

reimu::Result<os_handle_t, reimu::OSError> os_open_pty(os_handle_t &master_in, os_handle_t &slave_in, os_handle_t &master_out, os_handle_t &slave_out) {
    // Create pipes for the PTY
    HANDLE master_read, master_write;
    HANDLE slave_read, slave_write;

    SECURITY_ATTRIBUTES sa = {0};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&master_read, &slave_write, &sa, 0)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    if (!CreatePipe(&slave_read, &master_write, &sa, 0)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    master_in = master_read;
    master_out = master_write;

    slave_in = slave_read;
    slave_out = slave_write;

    // Create a PTY (will only work on Windows 10 and up)
    HPCON hpcon;

    if (CreatePseudoConsole({80, 25}, slave_read, slave_write, 0, &hpcon) != S_OK) {
        return ERR(reimu::OSError{GetLastError()});
    }

    SetConsoleMode(master_write, ENABLE_VIRTUAL_TERMINAL_INPUT);

    CloseHandle(slave_read);
    CloseHandle(slave_write);

    return OK(hpcon);
}

reimu::Result<size_t, reimu::OSError> os_pty_read(os_handle_t handle, void *buffer, size_t size) {
    // Get available data in the PTY
    DWORD available;
    if (!PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    size = std::min(size, (size_t)available);
    if (size == 0) {
        return OK(0);
    }

    DWORD read;
    if (!ReadFile(handle, buffer, size, &read, nullptr)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    return OK(read);
}

reimu::Result<int, reimu::OSError> os_create_process_pty(const char *path, char *const argv[], HPCON hpcon,
        os_handle_t master_in, os_handle_t slave_in, os_handle_t master_out, os_handle_t slave_out) {
    // Create a new process
    STARTUPINFOEX si = {0};
    si.StartupInfo.cb = sizeof(STARTUPINFOEX);

    size_t size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);

    std::unique_ptr<uint8_t[]> attr = std::make_unique<uint8_t[]>(size);
    si.lpAttributeList = (PPROC_THREAD_ATTRIBUTE_LIST)attr.get();

    if (!InitializeProcThreadAttributeList(si.lpAttributeList, 1, 0,
            &size)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    // Attach the PTY to the new process
    if (!UpdateProcThreadAttribute(si.lpAttributeList, 0,
            PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hpcon, sizeof(HPCON), nullptr, nullptr)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    PROCESS_INFORMATION pi = {0};

    if (!CreateProcess(path, nullptr, nullptr, nullptr, FALSE, EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &si.StartupInfo, &pi)) {
        return ERR(reimu::OSError{GetLastError()});
    }

    DeleteProcThreadAttributeList(si.lpAttributeList);

    return OK(pi.dwProcessId);
}

void os_pty_set_size(os_handle_t handle, int cols, int rows) {
    if(ResizePseudoConsole(handle, {(SHORT)cols, (SHORT)rows}) != S_OK) {
        reimu::logger::warn("Failed to resize PTY");
    }
}

#endif

using namespace reimu;

class TerminalApp {
public:
    TerminalApp() {
        m_event_loop = std::unique_ptr<EventLoop>{
            EventLoop::create().ensure()
        };

        os_handle_t slave_in;
        os_handle_t slave_out;
        
        m_pty_handle = os_open_pty(m_pty_in, slave_in, m_pty_out, slave_out).ensure();

        m_event_loop->watch_os_handle(m_pty_in, [this]() {
            pty_read();
        });

        m_event_loop->watch_os_handle(video::get_driver()->get_window_client_handle(), [this]() {
            video::get_driver()->window_client_dispatch();

            if (!m_window->is_open()) {
                m_event_loop->end();
            }
        });

        auto win = gui::Window::create({96*8, 52*16}).ensure();
        win->set_title("reimu-terminal");

        m_window = std::unique_ptr<gui::Window>{ win };

        auto *root = &m_window->root();
        root->layout.layout_direction = gui::LayoutDirection::Horizontal;

        auto *terminal_widget = new gui::TerminalWidget();
        terminal_widget->layout.width = terminal_widget->layout.height = gui::Size::inherit();

        terminal_widget->bind_event_callback("on_key_down"_hashid, [this]() {
            auto ev = m_window->get_last_input_event();

            switch (ev.key.key) {
            case video::Key::Return:
#ifdef REIMU_UNIX
                os::write(m_pty_out, "\n", 1);
#else
                // For SOME reason (i can't figure it out),
                // we only need to send \r on Windows.
                os::write(m_pty_out, "\r", 1);
#endif
                break;
            case video::Key::Backspace:
#ifdef REIMU_UNIX
                os::write(m_pty_out, "\b", 1);
#else
                // For SOME reason windows likes to use \x7f (DEL) for backspace.
                os::write(m_pty_out, "\x7f", 1);
#endif
                break;
            case video::Key::Tab:
                os::write(m_pty_out, "\t", 1);
                break;
            case video::Key::Left:
                os::write(m_pty_out, "\e[D", 3);
                break;
            case video::Key::Right:
                os::write(m_pty_out, "\e[C", 3);
                break;
            case video::Key::Up:
                os::write(m_pty_out, "\e[A", 3);
                break;
            case video::Key::Down:
                os::write(m_pty_out, "\e[B", 3);
                break;
            case video::Key::Space:
                os::write(m_pty_out, " ", 1);
                break;
            case 'c':
            case 'C':
                if (ev.key.is_ctrl) {
#ifdef REIMU_UNIX
                    // Get the foreground process group
                    pid_t pgid = tcgetpgrp(m_pty_out);
                    if (pgid < 0) {
                        logger::warn("Failed to get foreground process group: {}", strerror(errno));
                        break;
                    }

                    // Send SIGINT to the process group
                    if (killpg(pgid, SIGINT) < 0) {
                        logger::warn("Failed to send SIGINT to process group: {}", strerror(errno));
                    }
#endif
                } else {
                    os::write(m_pty_out, &ev.key.key, 1);
                }
                break;
            case 0:
                break;
            default:
                if (isprint(ev.key.key)) {
                    os::write(m_pty_out, &ev.key.key, 1);
                }
                break;
            }
        });

        m_terminal_widget = std::unique_ptr<gui::TerminalWidget>{ terminal_widget };

        root->add_child(terminal_widget);
        m_window->render();

        os_pty_set_size(m_pty_handle, m_terminal_widget->get_size().x, m_terminal_widget->get_size().y);

        auto shell = os::default_shell_path().ensure();
        char * const argv[] = { shell.data(), nullptr};

        os_create_process_pty(shell.c_str(), argv, m_pty_handle, m_pty_in, slave_in, m_pty_out, slave_out)
            .ensure();
    }

    void run() {
        m_window->render();
        m_event_loop->run();

        reimu::logger::debug("Exiting\n");
    }

    void pty_read() {
        char buf[1024];
        ssize_t n = os_pty_read(m_pty_in, buf, sizeof(buf)).ensure();
        if (n < 0) {
            logger::warn("Failed to read from pty");
            return;
        }

        char *end = buf + n;
        for (auto it = buf; it < end; it++) {
            if (m_escape_state != EscapeState::None) {
                //logger::debug("seq: {}", *it);

                parse_escape_sequence(*it);
                continue;
            }

            if (isprint(*it)) {
                m_terminal_widget->put_char(*it);
            } else if ((*it) & 0x80) {
                size_t consumed;
                auto cp = single_utf8_to_utf32(it, end - it, consumed);

                if (cp.has_some()) {
                    it += consumed - 1;

                    m_terminal_widget->put_char(cp.move_val());
                }
            } else {
                switch (*it) {
                case '\n':
                    m_terminal_widget->line_break();
                    break;
                case '\r': {
                    int cur_y = m_terminal_widget->get_cursor().y;

                    m_terminal_widget->set_cursor({0, cur_y});
                    break;
                } 
                case '\b':
                    m_terminal_widget->backspace();
                    break;
                case '\e':
                    m_escape_state = EscapeState::Escape;
                    m_escape_buf.clear();
                    break;
                case reimu::video::Key::Tab:
                    m_terminal_widget->put_char('\t');
                    break;
                default:
                    break;
                }
            }
        }

        m_window->render();
    }

    std::vector<int> parse_params(int empty_value_default) {
        // Parse semicolon separated parameters
        std::vector<int> params;
        std::string param_str;

        for (auto c : m_escape_buf) {
            if (isdigit(c)) {
                param_str += c;
            } else if (c == ';') {
                if (!param_str.empty()) {
                    params.push_back(std::stoi(param_str));
                    param_str.clear();
                } else {
                    // Empty parameter
                    params.push_back(empty_value_default);
                }
            } else if (c == ' ') {
                // Ignore space
                continue;
            } else {
                // Invalid character
                return {};
            }
        }

        if (!param_str.empty()) {
            params.push_back(std::stoi(param_str));
        }

        return params;
    }

    bool is_csi_final_byte(char c) {
        return c >= 0x40 && c <= 0x7e;
    }

    void parse_escape_sequence(char c) {
        switch(m_escape_state) {
        case EscapeState::Escape:
            if (c == CSI) {
                m_escape_state = EscapeState::CSI;
            } else if (c == OSC) {
                m_escape_state = EscapeState::OSC;
            } else if (c == 'c') {
                m_terminal_widget->set_cursor({0, 0});
                m_terminal_widget->erase_display(gui::TerminalWidget::EraseMode::All);
                m_terminal_widget->reset_attributes();

                m_escape_state = EscapeState::None;
            } else {
                // Unsupported escape sequence
                logger::warn("Unsupported escape cmd: {}", c);
                m_escape_state = EscapeState::None;
            }
            break;
        case EscapeState::CSI: {
            const auto params = parse_params(0);
            const auto first_param_or_one = [&]() {
                if (params.empty() || params[0] == 0) {
                    return 1;
                } else {
                    return params[0];
                }
            };
            
            switch (c) {
            case CursorUp: {
                int amt = first_param_or_one();

                m_terminal_widget->move_cursor(0, -amt);
                break;
            } case CursorDown: {
                int amt = first_param_or_one();

                m_terminal_widget->move_cursor(0, amt);
                break;
            } case CursorForward: {
                int amt = first_param_or_one();

                m_terminal_widget->move_cursor(amt, 0);
                break;
            } case CursorBackward: {
                int amt = first_param_or_one();

                m_terminal_widget->move_cursor(-amt, 0);
                break;
            } case CursorNextLine: {
                int amt = first_param_or_one();

                m_terminal_widget->move_cursor(0, amt);
                break;
            } case CursorPrevLine: {
                int amt = first_param_or_one();

                m_terminal_widget->move_cursor(0, -amt);
                break;
            } case CursorHorizontalAbsolute: {
                int amt = first_param_or_one();

                m_terminal_widget->set_cursor({amt - 1, m_terminal_widget->get_cursor().y});
                break;
            } case CursorPosition: {
                int row = first_param_or_one();

                int col = 1;
                if (params.size() > 1) {
                    col = params[1];
                }

                if (row < 1) {
                    row = 1;
                }

                if (col < 1) {
                    col = 1;
                }

                m_terminal_widget->set_cursor({col - 1, row - 1});
                break;
            }
            case EraseDisplay: {
                int mode = 0;
                if (!params.empty()) {
                    mode = params[0];
                }

                switch (mode) {
                case 0:
                    m_terminal_widget->erase_display(gui::TerminalWidget::EraseMode::CursorToEnd);
                    break;
                case 1:
                    m_terminal_widget->erase_display(gui::TerminalWidget::EraseMode::StartToCursor);
                    break;
                case 2:
                    m_terminal_widget->erase_display(gui::TerminalWidget::EraseMode::All);
                    break;
                }
                break;
            }
            case EraseLine: {
                int mode = 0;
                if (!params.empty()) {
                    mode = params[0];
                }

                switch (mode) {
                case 0:
                    m_terminal_widget->erase_line(gui::TerminalWidget::EraseMode::CursorToEnd);
                    break;
                case 1:
                    m_terminal_widget->erase_line(gui::TerminalWidget::EraseMode::StartToCursor);
                    break;
                case 2:
                    m_terminal_widget->erase_line(gui::TerminalWidget::EraseMode::All);
                    break;
                }
                break;
            }
            case ScrollUp:
            case ScrollDown:
                logger::warn("Escape sequence \\e[{}{} not implemented", m_escape_buf, c);
                break;
            case SetGraphicsRendition:
                parse_sgr();
                break;
            case Enable:
                // TODO: this is really hacky
                if (m_escape_buf.compare("?25")) {
                    m_terminal_widget->set_cursor_visible(true);
                }
                break;
            case Disable:
                if (m_escape_buf.compare("?25")) {
                    m_terminal_widget->set_cursor_visible(false);
                }
                break;
            default:
                if (is_csi_final_byte(c)) {
                    logger::warn("Unsupported escape sequence \\e[{}{}", m_escape_buf, c);
                } else {
                    // Continue parsing
                    m_escape_buf += c;
                    return;
                }
            }

            m_escape_state = EscapeState::None;
            break;
        }
        case EscapeState::OSC:
            // OSC sequence ends with BEL or ST
            if (c == '\a' || c == '\x9c') {
                // End of OSC sequence
                // TODO: handle OSC sequence
                m_escape_state = EscapeState::None;
            } else {
                // Continue parsing
                m_escape_buf += c;
                return;
            }
            break;
        }
    }

    void parse_sgr() {
        //logger::debug("sgr sequence {}", m_escape_buf);

        auto params = parse_params(0);

        if (params.empty()) {
            // No parameters
            return;
        }

        int sgr_cmd = params[0];
        switch (sgr_cmd)
        {
        case 0:
            // Reset all attributes
            m_terminal_widget->reset_attributes();
            break;
        case 1:
            // Bold
            m_terminal_widget->set_bold(true);
            break;
        case 7:
            // Invert colors
            m_terminal_widget->invert_colors();
            break;
        case 38:
            // Set foreground color
            if (params.size() == 3 && params[1] == 5) {
                // Look up 8-bit color
                m_terminal_widget->set_fg_color(params[2]);
            } else if (params.size() == 6 && params[1] == 2) {
                // Set 24-bit color
                m_terminal_widget->set_fg_color({params[3], params[4], params[5], 255});
            }
            break;
        case 39:
            // Reset foreground color
            m_terminal_widget->set_fg_color(15);
            break;
        case 48:
            // Set background color
            if (params.size() == 3 && params[1] == 5) {
                // Look up 8-bit color
                m_terminal_widget->set_bg_color(params[2]);
            } else if (params.size() == 6 && params[1] == 2) {
                // Set 24-bit color
                m_terminal_widget->set_bg_color({params[3], params[4], params[5], 255});
            }
            break;
        case 49:
            // Reset background color
            m_terminal_widget->set_bg_color(0);
            break;
        default:
            if (sgr_cmd >= 30 && sgr_cmd <= 37) {
                // Set foreground color
                m_terminal_widget->set_fg_color(sgr_cmd - 30);
            } else if (sgr_cmd >= 40 && sgr_cmd <= 47) {
                // Set background color
                m_terminal_widget->set_bg_color(sgr_cmd - 40);
            } else if (sgr_cmd >= 90 && sgr_cmd <= 97) {
                // Set bright foreground color
                m_terminal_widget->set_fg_color(sgr_cmd - 90 + 8);
            } else if (sgr_cmd >= 100 && sgr_cmd <= 107) {
                // Set bright background color
                m_terminal_widget->set_bg_color(sgr_cmd - 100 + 8);
            } else {
                logger::warn("SGR command {} not implemented", sgr_cmd);
            }

            break;
        }
    }

private:
    enum class EscapeState {
        None,
        Escape, // Following an '\e' byte
        CSI, // CSI (Control Sequence Introducer)
        OSC, // OSC (Operating System Command)
    };

    enum EscapeCode {
        CSI = '[',
        OSC = ']',
    };

    enum CSICharacter {
        None,
        CursorUp = 'A',
        CursorDown = 'B',
        CursorForward = 'C',
        CursorBackward = 'D',
        CursorNextLine = 'E',
        CursorPrevLine = 'F',
        CursorHorizontalAbsolute = 'G',
        CursorPosition = 'H',
        EraseDisplay = 'J',
        EraseLine = 'K',
        ScrollUp = 'S',
        ScrollDown = 'T',
        SetGraphicsRendition = 'm',
        Enable = 'h',
        Disable = 'l',
    };

    std::string m_escape_buf;
    EscapeState m_escape_state = EscapeState::None;

    os_handle_t m_pty_out;
    os_handle_t m_pty_in;
    os_handle_t m_pty_handle;

    pid_t m_child_pid;

    std::unique_ptr<EventLoop> m_event_loop;

    std::unique_ptr<gui::Window> m_window;
    std::unique_ptr<gui::TerminalWidget> m_terminal_widget;
};

int main() {
    video::init();

    TerminalApp app;
    app.run();

    return 0;
}
