#include <cstdint>

#include <iostream>
#include <fstream>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <string>

#include <fmt/format.h>

#include <Z80.h>

#ifdef USE_SDL
#include <SDL.h>
#endif

#include "crt9028.h"
#include "wd2793.h"
#include "terminal.h"

class MachineContext {
public:
    MachineContext() : cpu{
        .context = this,
        .fetch_opcode = MachineContext::read_memory,
        .fetch = MachineContext::read_memory,
        .read = MachineContext::read_memory,
        .write = MachineContext::write_memory,
        .in = MachineContext::read_io,
        .out = MachineContext::write_io,
    }, rom_mapped{true}
    {
        this->ram.fill(0);
        z80_power(&this->cpu, true);
    };

    static uint8_t read_memory(void* context, uint16_t address) {
        MachineContext* machine = static_cast<MachineContext*>(context);

        uint8_t value;
        if (machine->rom_mapped && address <= 0x1fff) {
            value = machine->rom[address];
        } else {
            value = machine->ram[address];
        }
        return value;
    };

    static void write_memory(void* context, uint16_t address, uint8_t value) {
        MachineContext* machine = static_cast<MachineContext*>(context);
        machine->ram[address] = value;
    };

    static uint8_t read_io(void* context, uint16_t address) {
        MachineContext* machine = static_cast<MachineContext*>(context);

        /* First IO READ will remove ROM from circuit */
        machine->rom_mapped = false;

        // Is CRT9028 status register being read?
        if ((address & 0xff) == 0x45) {
            return machine->crt.read_status_register();
        }

        // Is CRT9028 data register being read?
        if ((address & 0xff) == 0x44) {
            return machine->crt.read_data_register();
        }

        // Is the keyboard register being read?
        if ((address & 0xff) == 0x41) {
            if (machine->keyboard_state == 0) {
                if (machine->keyboard_buffer.size() > 0) {
                    machine->keyboard_char = machine->keyboard_buffer.back();
                    machine->keyboard_buffer.pop_back();

                    machine->keyboard_state++;

                    return machine->keyboard_char | 0x80;
                } else {
                    return 0x00;
                }
            }

            if (machine->keyboard_state < 3) {
                machine->keyboard_state++;

                return machine->keyboard_char | 0x80;
            }

            if (machine->keyboard_state == 6) {
                machine->keyboard_state = 0;
            } else {
                machine->keyboard_state++;
            }

            return machine->keyboard_char;
        }

        // Is the WD2793 status register being read?
        if ((address & 0xff) == 0x48) {
            return machine->disk.read_status_register();
        }

        // Is the WD2793 track register being read?
        if ((address & 0xff) == 0x49) {
            return machine->disk.read_track_register();
        }

        // Is the WD2793 sector register being read?
        if ((address & 0xff) == 0x4a) {
            return machine->disk.read_sector_register();
        }

        // Is the WD2793 data register being read?
        if ((address & 0xff) == 0x4b) {
            return machine->disk.read_data_register();
        }

        throw std::runtime_error(fmt::format("Unknown port: in {:02x}", address & 0xff));
    };

    static void write_io(void* context, uint16_t address, uint8_t value) {
        MachineContext* machine = static_cast<MachineContext*>(context);

        // Is CRT9028 address register being written?
        if ((address & 0xff) == 0x45) {
            machine->crt.write_address_register(value);
            return;
        }

        // Is CRT9028 data register being written?
        if ((address & 0xff) == 0x44) {
            machine->crt.write_data_register(value);
            return;
        }

        // Is the WD2793 command register being written?
        if ((address & 0xff) == 0x48) {
            machine->disk.write_command_register(value);
            return;
        }

        // Is the WD2793 track register being written?
        if ((address & 0xff) == 0x49) {
            machine->disk.write_track_register(value);
            return;
        }

        // Is the WD2793 sector register being written?
        if ((address & 0xff) == 0x4a) {
            machine->disk.write_sector_register(value);
            return;
        }

        // Is the WD2793 data register being written?
        if ((address & 0xff) == 0x4b) {
            machine->disk.write_data_register(value);
            return;
        }

        // Are the LEDs being programmed?
        if ((address & 0xff) == 0x00) {
            // write goes to the LEDs.
            // TODO.
            return;
        }

        throw std::runtime_error(fmt::format("Unknown port: out {:02x} <= {:02x}", address & 0xff, value));
    };

    size_t step(void) {
        size_t cycles = z80_run(&this->cpu, 256);

        return cycles;
    };

    void load_ram_image(const std::string& file_name) {
        std::ifstream fid;
        fid.open(file_name,std::ios::in);
        if (!fid) {
            throw std::runtime_error(fmt::format("could not open RAM image file {}", file_name));
        }

        fid.seekg(0, std::ios::end);
        size_t image_size = fid.tellg();
        fid.seekg(0, std::ios::beg);

        // read up to RAM size bytes to memory
        fid.read(reinterpret_cast<char*>(this->ram.data()), (image_size < ram.size()) ? image_size : ram.size());

        fid.close();
    }

    void load_rom_image(const std::string& file_name) {
        std::ifstream fid;
        fid.open(file_name,std::ios::in);
        if (!fid) {
            throw std::runtime_error(fmt::format("could not open ROM image file {}", file_name));
        }

        fid.seekg(0, std::ios::end);
        size_t image_size = fid.tellg();
        fid.seekg(0, std::ios::beg);

        // read up to ROM size bytes to memory
        fid.read(reinterpret_cast<char*>(this->rom.data()), (image_size < rom.size()) ? image_size : rom.size());

        fid.close();
    }

    uint16_t get_pc() const {
        return Z80_PC(this->cpu);
    }

    std::string render_screen_vt100(bool render_border = true) {
        return this->crt.render_screen_vt100(render_border);
    }

#ifdef USE_SDL
    void render_screen_sdl(SDL_Texture* screen) {
        this->crt.render_screen_sdl(screen);
    }
#endif

    void keyboard_input(const std::string& input) {
        for (const auto& ch : input) {
            keyboard_input_char(ch);
        }
    }

    void keyboard_input_char(char ch) {
        this->keyboard_buffer.push_back(ch);
    }

    void load_disk_image(const std::string& file_name) {
        this->disk.load_disk_image(file_name);
    }

    void save_disk_image(const std::string& file_name) {
        this->disk.save_disk_image(file_name);
    }

    void reset() {
        this->rom_mapped = true;
        z80_instant_reset(&this->cpu);
    }

private:
    /**
     * CPU state
     */
    Z80 cpu;

    /**
     * RAM
     */
    std::array<uint8_t, 65536> ram;

    /**
     * ROM
     */
    std::array<uint8_t, 2048> rom;

    /**
     * ROM mapped to memory flag
     */
    bool rom_mapped;

    /**
     * CRT9028 CRT controller
     */
    CRT9028 crt{};

    /**
     * WD2793 disk controller
     */
    WD2793 disk{};

    /**
     * Keyboard buffer
     */
    std::vector<char> keyboard_buffer;

    /**
     * Keyboard feed state counter
     */
    int keyboard_state{};

    /**
     * Active char from keyboard buffer
    */
    char keyboard_char{};
};

const std::string poll_input() {
    int bytes_available;

    if (ioctl(0, FIONREAD, &bytes_available) < 0) {
        throw std::runtime_error(fmt::format("ioctl: {}", strerror(errno)));
    }

    if (bytes_available > 0) {
        std::string line;
        std::getline(std::cin, line);

        return line;
    }

    return std::string{};
}

int main() {
    Terminal pty;

    fmt::print("Terminal is at {}\n", pty.get_pty_name());

    MachineContext machine;

    try {
        machine.load_rom_image("rom.bin");
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: failure loading ROM image: {}\n", e.what());
        return 1;
    }

    std::string disk_file_name("disk.bin");
    try {
        machine.load_disk_image(disk_file_name);
    } catch (const std::exception& e) {
        fmt::print(stderr, "Warning: failure loading default disk image: {}\n", e.what());
    }

    bool quit = false;
    size_t pty_cycles_since_update{};
    size_t cycles_since_start{};

#if USE_SDL
    size_t sdl_cycles_since_update{};

    SDL_Window* sdl_window = SDL_CreateWindow(
        "UCSD Microcomputer Emulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1000, 750,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 560, 1250);
    SDL_SetTextureScaleMode(texture, SDL_ScaleModeBest);

    SDL_StartTextInput();
#endif

    const auto start = std::chrono::steady_clock::now();

    fmt::print("> ");
    std::flush(std::cout);

    while (!quit) {
        // interactive
        std::string input = poll_input();
        if (input.size() > 0) {
            if (input == "help" || input == "?") {
                fmt::print("Available commands:\n");
                fmt::print("  help, ?: Show this help message\n");
                fmt::print("  exit, quit: Exit the emulator\n");
                fmt::print("  save_disk [file_name]: Save disk image to file (default: {})\n", disk_file_name);
                fmt::print("  load_disk [file_name]: Load disk image from file (default: {})\n", disk_file_name);
                fmt::print("  reset: Reset the system\n");
            } else if (input == "exit" || input == "quit") {
                quit = true;
                continue;
            } else if (input.compare(0, 9, "save_disk") == 0) {
                std::string arg;
                if (input.size() >= 10) {
                    arg = input.substr(10);
                }
                try {
                    if (arg.size() > 0) {
                        machine.save_disk_image(arg);
                    } else {
                        machine.save_disk_image(disk_file_name);
                    }
                    fmt::print("Saved disk image\n");
                } catch (const std::exception& e) {
                    fmt::print(stderr, "Warning: failure saving disk image: {}\n", e.what());
                }
            } else if (input.compare(0, 9, "load_disk") == 0) {
                std::string arg;
                if (input.size() >= 10) {
                    arg = input.substr(10);
                }
                try {
                    if (arg.size() > 0) {
                        machine.load_disk_image(arg);
                    } else {
                        machine.load_disk_image(disk_file_name);
                    }
                    fmt::print("Loaded disk image\n");
                } catch (const std::exception& e) {
                    fmt::print(stderr, "Warning: failure loading disk image: {}\n", e.what());
                }
            } else if (input.compare(0, 5, "reset") == 0) {
                fmt::print("Resetting system\n");
                machine.reset();
            } else {
                fmt::print("Unknown command\n");
            }

            // show prompt again after command
            fmt::print("> ");
            std::flush(std::cout);
        }

        size_t cycles = machine.step();
        cycles_since_start += cycles;

        std::this_thread::sleep_until(
            start + std::chrono::microseconds(cycles_since_start / 4)
        );

        pty_cycles_since_update += cycles;

        if (pty.is_pty_attached()) {
            if (pty_cycles_since_update >= 80000) {
                pty_cycles_since_update = 0;
                pty.write_output(machine.render_screen_vt100(true));
            }

            machine.keyboard_input(pty.read_input());
        }

#ifdef USE_SDL
        sdl_cycles_since_update += cycles;

        if (sdl_cycles_since_update >= 80000) {
            sdl_cycles_since_update = 0;
            machine.render_screen_sdl(texture);

            SDL_RenderClear(sdl_renderer);
            SDL_RenderCopyF(sdl_renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(sdl_renderer);
        }

        SDL_Event sdl_event;
        while (SDL_PollEvent(&sdl_event)) {
            if (sdl_event.type == SDL_QUIT) {
                quit = true;
            }
            if (sdl_event.type == SDL_TEXTINPUT) {
                machine.keyboard_input(sdl_event.text.text);
            }
            if (sdl_event.type == SDL_KEYDOWN) {
                if (sdl_event.key.keysym.sym == SDLK_RETURN) {
                    machine.keyboard_input_char(13);
                }

                if (sdl_event.key.keysym.sym == SDLK_BACKSPACE) {
                    machine.keyboard_input_char(127);
                }

                if (sdl_event.key.keysym.sym == SDLK_ESCAPE) {
                    machine.keyboard_input_char(27);
                }
            }
        }
#endif
    }

#ifdef USE_SDL
    SDL_StopTextInput();
    SDL_Quit();
#endif
}
