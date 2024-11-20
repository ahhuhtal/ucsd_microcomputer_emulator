#ifndef CRT9028_H
#define CRT9028_H

#include <array>
#include <cstdint>
#include <string>

#include <fmt/format.h>

class CRT9028 {
public:
    CRT9028() : video_memory{}, screen_changed{true} {}

    /**
     * Perform a read on the status register
     */
    uint8_t read_status_register() const {
        // always say operation is done
        return 0x80; // DONE (DB7) = 1
    };

    /**
     * Perform a read on the data register
     */
    uint8_t read_data_register() {
        switch (this->address_register) {
            case AddressRegisterValue::CHARACTER: {
                // data at memory address register
                uint8_t value = this->video_memory[this->memory_address_register];

                // increment address if autoincrement is enabled
                if (this->autoincrement_enabled) {
                    this->memory_address_register = (this->memory_address_register + 1) % this->video_memory.size();
                }

                return value;
            }
            default: {
                // other registers are not readable
                return 0x00;
            }
        }
    };

    void write_address_register(uint8_t value) {
        switch (value & 0x0f) {
            case 0x06: {
                this->address_register = AddressRegisterValue::CHIP_RESET;
            } break;

            case 0x08: {
                this->address_register = AddressRegisterValue::TOSADD;
            } break;

            case 0x09: {
                this->address_register = AddressRegisterValue::CURLO;
            } break;

            case 0x0a: {
                this->address_register = AddressRegisterValue::CURHI;
            } break;

            case 0x0b: {
                this->address_register = AddressRegisterValue::FILADD;
            } break;

            case 0x0c: {
                this->address_register = AddressRegisterValue::ATTDAT;
            } break;

            case 0x0d: {
                this->address_register = AddressRegisterValue::CHARACTER;
            } break;

            case 0x0e: {
                this->address_register = AddressRegisterValue::MODE_REGISTER;
            } break;

            default: break; // unknown register value. keep register as it was.
        }
    };

    void write_data_register(uint8_t value) {
        switch (this->address_register) {
            case AddressRegisterValue::CHIP_RESET: {
                // any write to the reset register will reset the chip
                // TODO: restore reset values?
            } break;

            case AddressRegisterValue::TOSADD: {
                // ignore bit 7, which selects the raster scan settings
                // the lower bits denote the address as da[10..4], with da[3..0] = 0 fixed.

                // the following is a guess in how to handle TOS values that go beyond 1920 (or 1984?)
                this->top_of_screen_address = ((value & 0x7f) * 16) % this->video_memory.size();

                this->screen_changed = true;
            } break;

            case AddressRegisterValue::CURLO: {
                // update the low bits of the cursor register
                this->cursor_register = (this->cursor_register & 0xff00) | value;
                // set the memory address register as the cursor, but don't allow overflow
                this->memory_address_register = this->cursor_register % this->video_memory.size();
            } break;

            case AddressRegisterValue::CURHI: {
                // bit 7 is status line enable
                if (value & 0x80) {
                    this->status_line_enabled = true;
                } else {
                    this->status_line_enabled = false;
                }

                // bits 6..4 are smooth scrolling. TODO.

                // update the high bits of the cursor register
                this->cursor_register = ((value & 0x0f) << 8) | (this->cursor_register & 0x00ff);
                // set the memory address register as the cursor, but don't allow overflow
                this->memory_address_register = this->cursor_register % this->video_memory.size();
            } break;

            case AddressRegisterValue::FILADD: {
                this->fill_requested = true;
                this->fill_address = ((value & 0x7f) * 16) % this->video_memory.size();
            } break;

            case AddressRegisterValue::ATTDAT: {
                // TODO. screen attributes.
            } break;

            case AddressRegisterValue::CHARACTER: {
                this->screen_changed = true;

                if (this->fill_requested) {
                    uint16_t fill_index;
                    fill_index = this->cursor_register % this->video_memory.size();

                    if (fill_index == this->fill_address) {
                        // special case. fill entire memory.
                        this->video_memory.fill(value);
                    } else {
                        while (fill_index != this->fill_address) {
                            this->video_memory[fill_index] = value;
                            fill_index = (fill_index + 1) % this->video_memory.size();
                        }
                    }
                    this->fill_requested = false;
                } else {
                    // store received data to video memory
                    this->video_memory[this->memory_address_register] = value;

                    // increment address if autoincrement is enabled
                    if (this->autoincrement_enabled) {
                        this->memory_address_register = (this->memory_address_register + 1) % this->video_memory.size();
                    }
                }
            } break;

            case AddressRegisterValue::MODE_REGISTER: {
                // bit 7 determines whether memory address auto increment is enabled
                if ((value & 0x80) != 0) {
                    this->autoincrement_enabled = true;
                } else {
                    this->autoincrement_enabled = false;
                }
                // bits 6..0 are DNC.
            } break;
        }
    };

    std::string render_screen(bool render_border = false) {
        this->screen_changed = false;

        std::string screen;

        auto determine_character = [&](uint8_t memory_value) {
            if ( (memory_value & 0x7f) < 32) {
                return ' ';
            }

            return static_cast<char>(memory_value & 0x7f);
        };


        if (render_border) {
            // start with a boundary
            screen += std::string(82, '-');
            screen += '\n';
        }

        // output 24 lines of output
        for (size_t offset = 0; offset < 1920; offset++) {
            uint16_t address;

            if (this->status_line_enabled) {
                address = (this->top_of_screen_address + offset) % (this->video_memory.size() - 80);
            } else {
                address = (this->top_of_screen_address + offset) % this->video_memory.size();
            }

            if (render_border) {
                if ((offset % 80) == 0) {
                    screen += '|';
                }
            }
            screen += determine_character(this->video_memory[address]);
            if (render_border) {
                if ((offset % 80) == 79) {
                    screen += "|\n";
                }
            } else {
                if ((offset % 80) == 79) {
                    screen += '\n';
                }
            }
        }
        // if status line is enabled, output it on line 25
        if (this->status_line_enabled) {
            if (render_border) {
                screen += '|';
            }
            for (size_t status_address = 1920; status_address < this->video_memory.size(); status_address++) {
                screen += determine_character(this->video_memory[status_address]);
            }
            if (render_border) {
                screen += "|\n";
            }
        }

        if (render_border) {
            screen += std::string(82, '-');
        }

        return screen;
    };

    std::string render_screen_vt100(bool render_border = false) {
        if (this->screen_changed == false) {
            return "";
        }

        this->screen_changed = false;

        std::string screen;

        auto determine_character = [&](uint8_t memory_value) {
            if ( (memory_value & 0x7f) < 32) {
                return std::string(" ");
            }

            std::string repr;

            if (memory_value & 0x80) {
                repr += "\033[7m";
            }

            repr += static_cast<char>(memory_value & 0x7f);

            if (memory_value & 0x80) {
                repr += "\033[m";
            }

            return repr;
        };


        if (render_border) {
            screen += "\033[H";
            screen += std::string(82, '-');
        }

        // output 24 lines of output
        for (size_t offset = 0; offset < 1920; offset++) {
            uint16_t address;

            if (this->status_line_enabled) {
                address = (this->top_of_screen_address + offset) % (this->video_memory.size() - 80);
            } else {
                address = (this->top_of_screen_address + offset) % this->video_memory.size();
            }

            if ((offset % 80) == 0) {
                if (render_border) {
                    screen += fmt::format("\033[{}H|", (offset / 80) + 2);
                } else {
                    screen += fmt::format("\033[{}H", (offset / 80) + 1);
                }
            }
            screen += determine_character(this->video_memory[address]);
            if ((offset % 80) == 79) {
                if (render_border) {
                    screen += '|';
                }
            }
        }

        // if status line is enabled, output it on line 25
        if (this->status_line_enabled) {
            if (render_border) {
                screen += "\033[26H|";
            } else {
                screen += "\033[25H";
            }
            for (size_t status_address = 1920; status_address < 2000; status_address++) {
                screen += determine_character(this->video_memory[status_address]);
            }
            if (render_border) {
                screen += '|';
            }
        }

        if (render_border) {
            if (this->status_line_enabled) {
                screen += "\033[27H";
                screen += std::string(82, '-');
            } else {
                screen += "\033[26H";
                screen += std::string(82, '-');
            }
        }

        // move cursor to where the crt9028 cursor is
        size_t row, col;

        if (this->cursor_register >= this->top_of_screen_address) {
            size_t cursor_screen_address = this->cursor_register - this->top_of_screen_address;
            row = cursor_screen_address / 80;
            col = cursor_screen_address % 80;
        } else {
            size_t cursor_screen_address = this->cursor_register + this->video_memory.size() - this->top_of_screen_address;
            row = cursor_screen_address / 80;
            col = cursor_screen_address % 80;
        }
        if (render_border) {
            screen += fmt::format("\033[{};{}H", row+2, col+2);
        } else {
            screen += fmt::format("\033[{};{}H", row+1, col+1);
        }
        return screen;
    };

private:
    /**
     * Possible values that can be written into the address register
     */
    enum class AddressRegisterValue {
        CHIP_RESET,
        TOSADD, CURLO, CURHI, FILADD,
        ATTDAT, CHARACTER, MODE_REGISTER,
    };

    /**
     * Selected address register value
     */
    AddressRegisterValue address_register{};

    /**
     * Cursor position register
     */
    uint16_t cursor_register{};

    /**
     * Memory access address
     */
    uint16_t memory_address_register{};

    /**
     * Status line enabled
     */
    bool status_line_enabled{};

    /**
     * Memory access address auto increment enabled
     */
    bool autoincrement_enabled{};

    /**
     * Top of screen address
     */
    uint16_t top_of_screen_address{};

    /**
     * Video memory
     */
    std::array<uint8_t, 2000> video_memory;

    /**
     * Have screen contents changes since last render?
     */
    bool screen_changed{};

    /**
     * Is a fill requested?
     */
    bool fill_requested{};

    /**
     * Upper fill address
     */
    uint16_t fill_address{};
};

#endif
