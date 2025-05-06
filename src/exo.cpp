#include <cctype>
#include <cstdlib>
#include <iostream>
#include <termios.h>
#include <unistd.h>

int constexpr in_fd{0};

class Terminal
{
public:
    explicit Terminal() : m_saved_termios{}
    {
        // save original termios
        tcgetattr(in_fd, &m_saved_termios);

        termios app_termios{m_saved_termios};
        app_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        app_termios.c_oflag &= ~(OPOST);
        app_termios.c_cflag |= (CS8);
        app_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        // set application termios
        tcsetattr(in_fd, TCSAFLUSH, &app_termios);
    }
    ~Terminal() { tcsetattr(in_fd, TCSAFLUSH, &m_saved_termios); }
    Terminal(const Terminal&) = delete;
    Terminal(Terminal&&) = delete;
    Terminal& operator=(const Terminal&) = delete;
    Terminal& operator=(Terminal&&) = delete;

private:
    termios m_saved_termios;
};

int main()
{
    Terminal terminal{};
    char c;
    while (read(in_fd, &c, 1) > 0 && c != 'q') {
        if (iscntrl(c)) {
            std::cout << static_cast<int>(c) << "\r\n";
        } else {
            std::cout << static_cast<int>(c) << " ('" << c << "')" << "\r\n";
        }
    }
    return EXIT_SUCCESS;
}
