#include "tagged_message_buffer.hpp"

#include <algorithm>
#include <cstring>
#include <memory>

// Header layout (32-bit little endian): lower 24 bits = size, upper 8 bits = tag
static inline uint32_t make_header(size_t size, uint8_t tag)
{
    return (static_cast<uint32_t>(tag) << 24) | (static_cast<uint32_t>(size) & 0x00FFFFFFU);
}

static inline size_t header_size_to_length(uint32_t header)
{
    return static_cast<size_t>(header & 0x00FFFFFFU);
}

static inline uint8_t header_to_tag(uint32_t header)
{
    return static_cast<uint8_t>(header >> 24);
}



tagged_message_buffer::tagged_message_buffer(uint8_t* buffer, size_t buffer_size)
    : m_buffer(buffer)
    , m_buffer_size(buffer_size)
    , m_write_index(0)
    , m_read_index(0)
    , m_last_write_index(0)
    , m_full(false)
{
    std::align(alignof(uint32_t), m_message_header_size, reinterpret_cast<void*&>(m_buffer), m_buffer_size);
    // Initialize header area
    std::fill(m_buffer, m_buffer + m_message_header_size, 0);
}

// Align length up to 4 bytes
static inline size_t padded_len(size_t len)
{
    return (len + 3) & ~static_cast<size_t>(3);
}

bool tagged_message_buffer::write(const uint8_t* data, size_t len, uint8_t tag)
{
    if (m_full)
        return false;

    if (len > 0x00FFFFFFU)
        return false; // size field limited to 24 bits

    size_t payload_padded = padded_len(len);
    size_t total_message_size = m_message_header_size + payload_padded;

    size_t remaining_capacity = (m_write_index >= m_read_index)
        ? (m_buffer_size - m_write_index)
        : (m_read_index - m_write_index);

    if (total_message_size <= remaining_capacity)
    {
        write_message_header(m_write_index, make_header(len, tag));
        size_t data_index = m_write_index + m_message_header_size;
        std::memcpy(&m_buffer[data_index], data, len);
        // Zero pad the trailing bytes to keep deterministic contents
        if (payload_padded > len)
            std::memset(&m_buffer[data_index + len], 0, payload_padded - len);

        m_write_index += total_message_size;
        if (m_write_index == m_buffer_size) m_write_index = 0;
        if (m_write_index == m_read_index) m_full = true;
        return true;
    }

    // check front space
    size_t front_space = m_read_index;
    if (total_message_size <= front_space)
    {
        m_last_write_index = m_write_index;
        write_message_header(0, make_header(len, tag));
        std::memcpy(&m_buffer[m_message_header_size], data, len);
        if (payload_padded > len)
            std::memset(&m_buffer[m_message_header_size + len], 0, payload_padded - len);
        m_write_index = total_message_size;
        if (m_write_index == m_buffer_size) m_write_index = 0;
        if (m_write_index == m_read_index) m_full = true;
        return true;
    }

    return false;
}

std::tuple<const uint8_t*, size_t, uint8_t> tagged_message_buffer::read_next()
{
    size_t message_length = get_next_read_size();
    if (message_length == 0)
        return {nullptr, 0, 0};

    uint8_t tag = get_next_read_tag();
    size_t data_index = (m_read_index + m_message_header_size);
    return {&m_buffer[data_index], message_length, tag};
}

void tagged_message_buffer::release()
{
    size_t message_length = get_next_read_size();
    if (message_length == 0)
        return;

    size_t total_message_size = m_message_header_size + padded_len(message_length);
    m_read_index += total_message_size;

    if (m_read_index == m_buffer_size)
        m_read_index = 0;

    if (m_read_index == m_write_index)
    {
        m_read_index = 0;
        m_write_index = 0;
        m_last_write_index = 0;
    }

    if (m_read_index == m_last_write_index)
    {
        m_read_index = 0;
    }
    m_full = false;
}

size_t tagged_message_buffer::get_next_read_size() const
{
    if (m_write_index == m_read_index && !m_full)
        return 0;

    uint32_t header = read_message_header(m_read_index);
    return header_size_to_length(header);
}

uint8_t tagged_message_buffer::get_next_read_tag() const
{
    if (m_write_index == m_read_index && !m_full)
        return 0;
    uint32_t header = read_message_header(m_read_index);
    return header_to_tag(header);
}

uint32_t tagged_message_buffer::read_message_header(size_t header_index) const
{
    // Header is always written contiguous (write ensures this), so read directly
    uint32_t h = 0;
    for (size_t i = 0; i < m_message_header_size; ++i)
    {
        h |= static_cast<uint32_t>(m_buffer[header_index + i]) << (8 * i);
    }
    return h;
}

void tagged_message_buffer::write_message_header(size_t header_index, uint32_t header)
{
    for (size_t i = 0; i < m_message_header_size; ++i)
    {
        m_buffer[header_index + i] = static_cast<uint8_t>(header >> (8 * i));
    }
}
