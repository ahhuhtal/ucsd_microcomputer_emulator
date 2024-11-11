#ifndef TERMINAL_H
#define TERMINAL_H

#include <errno.h>
#include <string.h>
#include <pty.h>
#include <unistd.h>
#include <poll.h>

#include <stdexcept>
#include <vector>

#include <fmt/format.h>

class Terminal {
public:
    Terminal() {
        int clientfd;

        auto e = openpty(&this->fd, &clientfd, nullptr, nullptr, nullptr);

        if (e < 0) {
            throw std::runtime_error(fmt::format("openpty: {}\n", strerror(errno)));
        }

        this->ptyname = std::string(ttyname(clientfd));

        // close terminal end to not interfere with connection from outside
        close(clientfd);
    };

    /**
     * Return name of PTY device
     */
    const std::string& get_pty_name() const {
        return this->ptyname;
    };

    /**
     * Check if a client is connected to PTY
     */
    bool is_pty_attached() const {
        auto events = this->pollfd();

        if (events & POLLHUP) {
            return false;
        }

        return true;
    };

    int input_waiting() const {
        int bytes_available;

        if (ioctl(this->fd, FIONREAD, &bytes_available) < 0) {
            throw std::runtime_error(fmt::format("ioctl: {}", strerror(errno)));
        }

        return bytes_available;
    };

    const std::string read_input() const {
        int bytes = input_waiting();

        std::vector<char> data(bytes);
        int e = read(this->fd, data.data(), bytes);
        if (e < 0) {
            throw std::runtime_error(fmt::format("read: {}", strerror(errno)));
        }

        std::string input_string(data.data(), data.size());

        return input_string;
    }

    void write_output(const std::string& output) const {
        auto events = this->pollfd();

        if (events & POLLOUT) {
            int e = write(this->fd, output.data(), output.size());

            if (e < 0) {
                throw std::runtime_error(fmt::format("write: {}", strerror(errno)));
            }
        }
    };

private:
    /**
     * Perform a peek into the file descriptor
     */
    short pollfd() const {
        struct pollfd fds = {
            .fd = this->fd,
            .events = POLLIN | POLLOUT
        };

        int e = poll(&fds, 1, 0);
        if (e < 0) {
            throw std::runtime_error(fmt::format("poll: {}", strerror(errno)));
        }

        return fds.revents;
    }

    /**
     * Pseudo terminal server side file descriptor
     */
    int fd;

    /**
     * Pseudo terminal client side PTY name
     */
    std::string ptyname;
};

#endif
