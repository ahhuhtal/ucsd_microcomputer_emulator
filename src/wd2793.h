#ifndef WD2793_H
#define WD2793_H

#include <cstdint>
#include <stdexcept>
#include <fmt/format.h>

class WD2793 {
public:
    static constexpr size_t SectorsPerTrack = 26;
    static constexpr size_t BytesPerSector = 128;
    static constexpr size_t Tracks = 77;

    WD2793() {
    };

    uint8_t read_status_register() {
        switch (this->last_command) {
            case Command::Restore:
            case Command::Seek:
            case Command::StepIn:
            case Command::StepOut: {
                // never busy, no index pulse, no errors
                return (this->heads_loaded ? 0x20 : 0x00) | ((this->track == 0) ? 0x04 : 0x00);
            } break;
            case Command::ReadSector: {
                if (this->data_index < this->disk_data.size()) {
                    // always new data, never lost data or CRC error, always ready, never busy
                    this->data_register = this->disk_data[this->data_index];

                    return 0x02;
                } else {
                    return 0x10;
                }
            } break;
            case Command::WriteSector: {
                if (this->data_index < this->disk_data.size()) {
                    return 0x02;
                } else {
                    return 0x10;
                }
            } break;
        }
        return 0x00;
    }

    void write_command_register(uint8_t value) {
        // Is the command type I?
        if ((value & 0x80) == 0x00) {
            this->heads_loaded = (value & 0x08) != 0;

            // is the command a restore?
            if ((value & 0xf0) == 0x00) {
                this->track = 0;
                this->track_register = 0;

                last_command = Command::Restore;
                return;
            }

            // is the command a seek?
            if ((value & 0xf0) == 0x10) {
                // seek command assumes track register contains actual track
                // it then steps so that track register goes to data register

                // keep track of actual track and track register reparately
                int diff = static_cast<int>(this->data_register) - static_cast<int>(this->track_register);

                // in the end, track register will reach data register
                this->track_register = this->data_register;

                if (this->track + diff < 0) {
                    // head hit track 0 stop
                    this->track = 0;
                } else if (this->track + diff > 0xff) {
                    // head hit top track stop
                    this->track = 0xff;
                } else {
                    // head moved by diff
                    this->track += diff;
                }
                last_command = Command::Seek;
                return;
            }

            // is the command a step-in?
            if ((value & 0xe0) == 0x40) {
                this->track++;
                if (value & 0x10) {
                    this->track_register++;
                }
                last_command = Command::StepIn;
                return;
            }

            // is the command a step-out?
            if ((value & 0xe0) == 0x60) {
                if (this->track > 0) {
                    this->track--;
                }
                if (value & 0x10) {
                    this->track_register--;
                }
                last_command = Command::StepIn;
                return;
            }

            throw std::runtime_error(fmt::format("Unknown WD2793 type I command {:02x}", value));
        }

        // Is the command a type II?
        if ((value & 0xc0) == 0x80) {
            // Is the command a read sector?
            if ((value & 0xe0) == 0x80) {
                // TODO: support multiple records
                if (value & 0x10) {
                    throw std::runtime_error(fmt::format("WD2793 multiple records not supported: cmd = {:02x}", value));
                }

                // TODO: check disk side
                last_command = Command::ReadSector;
                this->data_index = this->track_register * SectorsPerTrack * BytesPerSector + (this->sector_register - 1) * BytesPerSector;
                return;
            }

            // Is the command a write sector?
            if ((value & 0xe0) == 0xa0) {
                // TODO: support multiple records
                if (value & 0x10) {
                    throw std::runtime_error(fmt::format("WD2793 multiple records not supported: cmd = {:02x}", value));
                }

                // TODO: check disk side
                last_command = Command::WriteSector;
                this->data_index = this->track_register * SectorsPerTrack * BytesPerSector + (this->sector_register - 1) * BytesPerSector;
                return;
            }

            throw std::runtime_error(fmt::format("Unknown WD2793 type II command {:02x}", value));
        }

        // Is the command a force interrupt?
        if ((value & 0xf0) == 0xd0) {
            last_command = Command::ForceInterrupt;
            return;
        }

        throw std::runtime_error(fmt::format("Unknown WD2793 command {:02x}", value));
    }

    uint8_t read_sector_register() {
        return this->sector_register;
    }

    void write_sector_register(uint8_t value) {
        this->sector_register = value;
    }

    uint8_t read_track_register() {
        return this->track_register;
    }

    void write_track_register(uint8_t value) {
        this->track_register = value;
    }

    uint8_t read_data_register() {
        if (this->last_command == Command::ReadSector) {
            this->data_index++;
        }

        return this->data_register;
    }

    void write_data_register(uint8_t value) {
        if (this->last_command == Command::WriteSector) {
            if (this->data_index < this->disk_data.size()) {
                this->disk_data[this->data_index] = value;
                this->data_index++;
            }
        }

        this->data_register = value;
    }

    void load_disk_image(const std::string& file_name) {
        std::ifstream fid;
        fid.open(file_name,std::ios::in);
        if (!fid) {
            throw std::runtime_error(fmt::format("WD2792 could not open disk image {}", file_name));
        }

        fid.seekg(0, std::ios::end);
        size_t image_size = fid.tellg();
        fid.seekg(0, std::ios::beg);

        fid.read(reinterpret_cast<char*>(this->disk_data.data()),
            (image_size < this->disk_data.size()) ? image_size : this->disk_data.size());

        fid.close();
    }
private:
    enum class Command {
        Restore, Seek, Step, StepIn, StepOut,
        ReadSector, WriteSector, ReadAddress, ReadTrack, WriteTrack,
        ForceInterrupt
    };

    /**
     * Last command
     */
    Command last_command{};

    /**
     * Current actual track
     */
    uint8_t track{};

    /**
     * Are heads loaded?
     */
    bool heads_loaded{};

    /**
     * Value in track register
     */
    uint8_t track_register{};

    /**
     * Value in sector register
     */
    uint8_t sector_register{};

    /**
     * Value in data register
     */
    uint8_t data_register{};

    /**
     * Sector data index for reading
     */
    size_t data_index{};

    /**
     * Disk image data in LBA type addressing
     * First 128 bytes are track 0, sector 1, then
     * 128 bytes of track 0, sector 2, etc.
     * with 26 sectors per track.
     */
    std::array<uint8_t, BytesPerSector * SectorsPerTrack * Tracks> disk_data{};
};

#endif
