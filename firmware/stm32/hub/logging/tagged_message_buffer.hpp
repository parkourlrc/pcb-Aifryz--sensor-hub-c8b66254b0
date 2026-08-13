#pragma once
#include <cstdint>
#include <cstddef>
#include <tuple>

class tagged_message_buffer
{
public:
    tagged_message_buffer(uint8_t* buffer, size_t buffer_size);

    /// Writes data to the buffer with an 8-bit tag. Returns true on success.
    bool write(const uint8_t* data, size_t len, uint8_t tag);

    /// Reads next contiguous data from the buffer, returns a pointer to the data, the length, and the tag
    std::tuple<const uint8_t*, size_t, uint8_t> read_next();

    /// Releases the read data, this must be called after read_next to free up space in the buffer
    void release();

private:
    size_t get_next_read_size() const;
    uint8_t get_next_read_tag() const;

    uint32_t read_message_header(size_t header_index) const;
    void write_message_header(size_t header_index, uint32_t header);

    uint8_t* m_buffer = nullptr;
    size_t m_buffer_size;

    size_t m_write_index;
    size_t m_last_write_index;
    size_t m_read_index;
    bool m_full;

    // Fixed 4 byte header: [tag:8][size:24] stored little-endian
    const size_t m_message_header_size = 4;
};
