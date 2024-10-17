#include <reimu/video/video.h>
#include <reimu/video/driver.h>
#include <reimu/core/event.h>
#include <reimu/core/unicode.h>
#include <reimu/gui/terminal.h>
#include <reimu/gui/window.h>
#include <reimu/gui/dialog.h>

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <spawn.h>
#include <pty.h>

#include <list>

using namespace reimu;

class TerminalApp {
public:
    TerminalApp() {
        m_event_loop = std::unique_ptr<EventLoop>{
            EventLoop::create().ensure()
        };

        int slave_fd;
        if (openpty(&m_pty_fd, &slave_fd, nullptr, nullptr, nullptr) < 0) {
            gui::message_box("openpty() failed", "Error", gui::MessageBoxButtons::ok());
            logger::fatal("Failed to openpty");
        }

        m_event_loop->watch_os_handle(m_pty_fd, [this]() {
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

            logger::debug("Key down: {} is_ctrl: {} is_shift: {} is_alt: {}",
                ev.key.key, ev.key.is_ctrl, ev.key.is_shift, ev.key.is_alt);

            switch (ev.key.key) {
            case video::Key::Return:
                write(m_pty_fd, "\n", 1);
                break;
            case video::Key::Backspace:
                write(m_pty_fd, "\b", 1);
                break;
            case video::Key::Left:
                write(m_pty_fd, "\e[D", 3);
                break;
            case video::Key::Right:
                write(m_pty_fd, "\e[C", 3);
                break;
            case video::Key::Up:
                write(m_pty_fd, "\e[A", 3);
                break;
            case video::Key::Down:
                write(m_pty_fd, "\e[B", 3);
                break;
            case 'c':
            case 'C':
                if (ev.key.is_ctrl) {
                    // Get the foreground process group
                    pid_t pgid = tcgetpgrp(m_pty_fd);
                    if (pgid < 0) {
                        logger::warn("Failed to get foreground process group: {}", strerror(errno));
                        break;
                    }

                    // Send SIGINT to the process group
                    if (killpg(pgid, SIGINT) < 0) {
                        logger::warn("Failed to send SIGINT to process group: {}", strerror(errno));
                    }
                } else {
                    write(m_pty_fd, &ev.key.key, 1);
                }
                break;
            case 0:
                break;
            default:
                write(m_pty_fd, &ev.key.key, 1);
                break;
            }
        });

        m_terminal_widget = std::unique_ptr<gui::TerminalWidget>{ terminal_widget };

        root->add_child(terminal_widget);
        m_window->render();

        // Update the window size.
        struct winsize ws;
        ws.ws_row = m_terminal_widget->get_size().y;
        ws.ws_col = m_terminal_widget->get_size().x;

        ioctl(m_pty_fd, TIOCSWINSZ, &ws);

        // Spawn a child process with the pty as its stdin/stdout/stderr.
        m_child_pid = fork();
        if (m_child_pid < 0) {
            gui::message_box("Failed to fork", "Error", gui::MessageBoxButtons::ok());
            logger::fatal("Failed to fork");
        }

        if (m_child_pid == 0) {
            if (setsid() < 0) {
                logger::fatal("Failed to setsid: {}", strerror(errno));
            }

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            dup2(slave_fd, STDIN_FILENO);
            dup2(slave_fd, STDOUT_FILENO);
            dup2(slave_fd, STDERR_FILENO);

            if(ioctl(slave_fd, TIOCSCTTY, 0)) {
                logger::fatal("Failed to set controlling TTY: {}", strerror(errno));
            }

            close(slave_fd);
            close(m_pty_fd);

            char *const argv[] = { "/bin/zsh", nullptr };
            auto *envp = environ;

            if (execve("/bin/zsh", argv, envp) < 0) {
                logger::fatal("Failed to execve");
            }
        }

        close(slave_fd);
    }

    void run() {
        m_window->render();
        m_event_loop->run();
    }

    void pty_read() {
        char buf[1024];
        ssize_t n = read(m_pty_fd, buf, sizeof(buf));
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
                    m_terminal_widget->move_cursor(-1, 0);
                    break;
                case '\e':
                    m_escape_state = EscapeState::Escape;
                    m_escape_buf.clear();
                    break;
                case ' ':
                default:
                    m_terminal_widget->put_char(*it);
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
    };

    std::string m_escape_buf;
    EscapeState m_escape_state = EscapeState::None;

    int m_pty_fd;

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
