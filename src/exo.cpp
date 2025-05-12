#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <lua.h>
#include <lua5.3/lauxlib.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <lua.hpp>

int constexpr in_fd{0};
char const* TERMESC{"\x1b"};

class Terminal
{
public:
    explicit Terminal() : m_saved_termios{}
    {
        // save original termios
        if (tcgetattr(in_fd, &m_saved_termios) < 0) {
            int sen = errno;
            throw std::runtime_error("Terminal::Terminal::tcgetattr: " + std::string(strerror(sen)));
        }

        termios app_termios{m_saved_termios};
        app_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        app_termios.c_oflag &= ~(OPOST);
        app_termios.c_cflag |= (CS8);
        app_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        // set application termios
        if (tcsetattr(in_fd, TCSAFLUSH, &app_termios) < 0) {
            int sen = errno;
            throw std::runtime_error("Terminal::Terminal::tcsetattr: " + std::string(strerror(sen)));
        }
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        width = w.ws_col;
        height = w.ws_row;
    }
    ~Terminal()
    {
        if (tcsetattr(in_fd, TCSAFLUSH, &m_saved_termios) < 0) {
            int sen = errno;
            std::cerr << "Terminal::~Terminal::tcsetattr: " + std::string(strerror(sen)) << "\n";
        }
    }
    Terminal(const Terminal&) = delete;
    Terminal(Terminal&&) = delete;
    Terminal& operator=(const Terminal&) = delete;
    Terminal& operator=(Terminal&&) = delete;
    void clear(bool flush = true)
    {
        std::cout << TERMESC << "[2J";
        if (flush)
            fflush(stdout);
    }
    void go_to(int x, int y, bool flush = true)
    {
        std::cout << TERMESC << "[" << std::to_string(y) << ";" << std::to_string(x) << "H";
        if (flush)
            fflush(stdout);
    }
    void write_message(std::string const& msg, bool flush = true)
    {
        std::cout << TERMESC << "7";
        go_to(0, height, false);
        std::cout << msg;
        std::cout << TERMESC << "8";
        if (flush)
            fflush(stdout);
    }

public:
    int width;
    int height;

private:
    termios m_saved_termios;
};

class Config
{
public:
    char exit_key;

public:
    explicit Config()
    {
        m_L = luaL_newstate();
        if (check_lua(luaL_dofile(m_L, "config.lua"))) {
            parse_exit_key();
        }
    }
    ~Config() { lua_close(m_L); }
    Config(const Config&) = delete;
    Config(Config&&) = delete;
    Config& operator=(const Config&) = delete;
    Config& operator=(Config&&) = delete;

private:
    void parse_exit_key()
    {
        lua_getglobal(m_L, "exit_key");
        if (lua_isstring(m_L, -1)) {
            size_t len{};
            char const* exit_key_c_str = lua_tolstring(m_L, -1, &len);
            if (len != 1) {
                throw std::runtime_error("Config::Config()::parse_exit_key");
            }
            exit_key = exit_key_c_str[0];
        } else {
            throw std::runtime_error("Config::Config()::parse_exit_key");
        }
    }
    bool check_lua(int r)
    {
        if (r != LUA_OK) {
            std::string error_msg = lua_tostring(m_L, -1);
            std::cout << error_msg << "\n";
            return false;
        }
        return true;
    }

private:
    lua_State* m_L;
};

int main()
{
    Config config{};
    Terminal terminal{};
    terminal.clear();
    terminal.go_to(0, 0);
    terminal.write_message("hello terminal");

    char c;
    while (read(in_fd, &c, 1) > 0 && c != config.exit_key) {
        if (iscntrl(c)) {
            std::cout << static_cast<int>(c) << "\r\n";
        } else {
            std::cout << static_cast<int>(c) << " ('" << c << "')" << "\r\n";
        }
    }
    return EXIT_SUCCESS;
}
