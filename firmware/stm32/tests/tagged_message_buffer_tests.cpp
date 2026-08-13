#include <catch2/catch_test_macros.hpp>
#include "logging/tagged_message_buffer.hpp"
#include <vector>

TEST_CASE("tagged buffer exposes contiguous bytes with tag", "[logging]")
{
    uint8_t buffer_memory[20];
    tagged_message_buffer buffer(buffer_memory, sizeof(buffer_memory));

    const char* data = "abcdef";
    uint8_t tag = 0x42;
    buffer.write(reinterpret_cast<const uint8_t*>(data), 6, tag);

    const auto [read_data, len, read_tag] = buffer.read_next();

    REQUIRE(read_data != nullptr);
    REQUIRE(len == 6);
    REQUIRE(read_tag == tag);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data), len) == "abcdef");
}

TEST_CASE("tagged buffer handles multiple writes and reads", "[logging]")
{
    uint8_t buffer_memory[30];
    tagged_message_buffer buffer(buffer_memory, sizeof(buffer_memory));

    const char* data1 = "abc";
    const char* data2 = "defG";
    uint8_t tag1 = 0x11;
    uint8_t tag2 = 0x22;
    
    buffer.write(reinterpret_cast<const uint8_t*>(data1), 3, tag1);
    buffer.write(reinterpret_cast<const uint8_t*>(data2), 4, tag2);

    const auto [read_data, len, read_tag] = buffer.read_next();

    REQUIRE(read_data != nullptr);
    REQUIRE(len == 3);
    REQUIRE(read_tag == tag1);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data), len) == "abc");

    buffer.release();

    const auto [read_data2, len2, read_tag2] = buffer.read_next();

    REQUIRE(read_data2 != nullptr);
    REQUIRE(len2 == 4);
    REQUIRE(read_tag2 == tag2);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data2), len2) == "defG");
}

TEST_CASE("tagged buffer handles wrap around", "[logging]")
{
    uint8_t buffer_memory[30];
    tagged_message_buffer buffer(buffer_memory, sizeof(buffer_memory));

    const char* data1 = "abcdefgh";
    uint8_t tag1 = 0xAA;
    buffer.write(reinterpret_cast<const uint8_t*>(data1), 8, tag1);

    const auto [read_data, len, read_tag] = buffer.read_next();

    REQUIRE(read_data != nullptr);
    REQUIRE(len == 8);
    REQUIRE(read_tag == tag1);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data), len) == "abcdefgh");

    buffer.release();

    const char* data2 = "xyz";
    uint8_t tag2 = 0xBB;
    buffer.write(reinterpret_cast<const uint8_t*>(data2), 3, tag2);

    const auto [read_data2, len2, read_tag2] = buffer.read_next();

    REQUIRE(read_data2 != nullptr);
    REQUIRE(len2 == 3);
    REQUIRE(read_tag2 == tag2);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data2), len2) == "xyz");
}

TEST_CASE("tagged buffer does not allow overflow", "[logging]")
{
    uint8_t buffer_memory[20];
    tagged_message_buffer buffer(buffer_memory, sizeof(buffer_memory));

    const char* data1 = "abcdefghijklmnop";
    bool result1 = buffer.write(reinterpret_cast<const uint8_t*>(data1), 16, 0x01);
    REQUIRE(result1 == true);

    const char* data2 = "k";
    bool result2 = buffer.write(reinterpret_cast<const uint8_t*>(data2), 1, 0x02);
    REQUIRE(result2 == false);
}

TEST_CASE("tagged buffer writes data in parts", "[logging]")
{
    uint8_t buffer_memory[40];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    uint8_t tag1 = 0x10;
    uint8_t tag2 = 0x20;

    stream.write(reinterpret_cast<const uint8_t*>("abcefgh"), 7, tag1);
    stream.write(reinterpret_cast<const uint8_t*>("ijklm"), 5, tag2);
    
    const auto [data, len, read_tag] = stream.read_next();
    REQUIRE(data != nullptr);
    REQUIRE(len == 7);
    REQUIRE(read_tag == tag1);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data), len) == "abcefgh");

    stream.release();
    const auto [data2, len2, read_tag2] = stream.read_next();
    REQUIRE(data2 != nullptr);
    REQUIRE(len2 == 5);
    REQUIRE(read_tag2 == tag2);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data2), len2) == "ijklm");
}

TEST_CASE("tagged buffer perfect fill", "[logging]")
{
    uint8_t buffer_memory[32];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    uint8_t tag1 = 0xAA;
    uint8_t tag2 = 0xBB;
    uint8_t tag3 = 0xCC;

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("abc"), 3, tag1));
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("def"), 3, tag2));
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("g"), 1, tag3));

    const auto [data, len, read_tag] = stream.read_next();
    REQUIRE(data != nullptr);
    REQUIRE(len == 3);
    REQUIRE(read_tag == tag1);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data), len) == "abc");
    stream.release();

    const auto [data2, len2, read_tag2] = stream.read_next();
    REQUIRE(data2 != nullptr);
    REQUIRE(len2 == 3);
    REQUIRE(read_tag2 == tag2);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data2), len2) == "def");
    stream.release();

    const auto [data3, len3, read_tag3] = stream.read_next();
    REQUIRE(data3 != nullptr);
    REQUIRE(len3 == 1);
    REQUIRE(read_tag3 == tag3);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data3), len3) == "g");
    stream.release();
}

TEST_CASE("tagged buffer perfect across wrap", "[logging]")
{
    uint8_t buffer_memory[32];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    uint8_t tag1 = 0x01;
    uint8_t tag2 = 0x02;
    uint8_t tag3 = 0x03;
    uint8_t tag4 = 0x04;
    uint8_t tag5 = 0x05;

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("abc"), 3, tag1));
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("def"), 3, tag2));

    const auto [data, len, read_tag] = stream.read_next();
    REQUIRE(data != nullptr);
    REQUIRE(len == 3);
    REQUIRE(read_tag == tag1);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data), len) == "abc");
    stream.release();

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("g"), 1, tag3));

    const auto [data2, len2, read_tag2] = stream.read_next();
    REQUIRE(data2 != nullptr);
    REQUIRE(len2 == 3);
    REQUIRE(read_tag2 == tag2);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data2), len2) == "def");
    stream.release();

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("hij"), 3, tag4));
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("klm"), 3, tag5));

    const auto [data3, len3, read_tag3] = stream.read_next();
    REQUIRE(data3 != nullptr);
    REQUIRE(len3 == 1);
    REQUIRE(read_tag3 == tag3);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data3), len3) == "g");
    stream.release();

    const auto [data4, len4, read_tag4] = stream.read_next();
    REQUIRE(data4 != nullptr);
    REQUIRE(len4 == 3);
    REQUIRE(read_tag4 == tag4);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data4), len4) == "hij");
    stream.release();

    const auto [data5, len5, read_tag5] = stream.read_next();
    REQUIRE(data5 != nullptr);
    REQUIRE(len5 == 3);
    REQUIRE(read_tag5 == tag5);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data5), len5) == "klm");
    stream.release();
}

TEST_CASE("tagged buffer write and reuse", "[logging]")
{
    uint8_t buffer_memory[32];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    uint8_t tag1 = 0xF0;

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("abcdefghi"), 9, tag1));

    const auto [data, len, read_tag] = stream.read_next();
    REQUIRE(data != nullptr);
    REQUIRE(len == 9);
    REQUIRE(read_tag == tag1);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data), len) == "abcdefghi");
    stream.release();

    uint8_t tag2 = 0xF1;
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("abcdefghi"), 9, tag2));

    const auto [data2, len2, read_tag2] = stream.read_next();
    REQUIRE(data2 != nullptr);
    REQUIRE(len2 == 9);
    REQUIRE(read_tag2 == tag2);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(data2), len2) == "abcdefghi");
    stream.release();
}

TEST_CASE("tagged buffer different tags with same data", "[logging]")
{
    uint8_t buffer_memory[40];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    const char* same_data = "test";
    uint8_t tag1 = 0x01;
    uint8_t tag2 = 0x02;
    uint8_t tag3 = 0x03;

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>(same_data), 4, tag1));
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>(same_data), 4, tag2));
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>(same_data), 4, tag3));

    const auto [data1, len1, read_tag1] = stream.read_next();
    REQUIRE(read_tag1 == tag1);
    stream.release();

    const auto [data2, len2, read_tag2] = stream.read_next();
    REQUIRE(read_tag2 == tag2);
    stream.release();

    const auto [data3, len3, read_tag3] = stream.read_next();
    REQUIRE(read_tag3 == tag3);
    stream.release();
}

TEST_CASE("tagged buffer zero tag", "[logging]")
{
    uint8_t buffer_memory[20];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    const char* data = "hello";
    uint8_t tag = 0x00;

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>(data), 5, tag));

    const auto [read_data, len, read_tag] = stream.read_next();
    REQUIRE(len == 5);
    REQUIRE(read_tag == 0x00);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data), len) == "hello");
}

TEST_CASE("tagged buffer max tag value", "[logging]")
{
    uint8_t buffer_memory[20];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    const char* data = "world";
    uint8_t tag = 0xFF;

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>(data), 5, tag));

    const auto [read_data, len, read_tag] = stream.read_next();
    REQUIRE(len == 5);
    REQUIRE(read_tag == 0xFF);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data), len) == "world");
}

TEST_CASE("tagged buffer single byte message", "[logging]")
{
    uint8_t buffer_memory[20];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    const char* data = "X";
    uint8_t tag = 0x99;

    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>(data), 1, tag));

    const auto [read_data, len, read_tag] = stream.read_next();
    REQUIRE(len == 1);
    REQUIRE(read_tag == tag);
    REQUIRE(std::string_view(reinterpret_cast<const char*>(read_data), len) == "X");
}

TEST_CASE("tagged buffer multiple releases and writes", "[logging]")
{
    uint8_t buffer_memory[50];
    tagged_message_buffer stream(buffer_memory, sizeof(buffer_memory));

    uint8_t tag1 = 0x11;
    uint8_t tag2 = 0x22;
    uint8_t tag3 = 0x33;
    uint8_t tag4 = 0x44;

    // Write and read cycle 1
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("msg1"), 4, tag1));
    {
        const auto [data, len, tag] = stream.read_next();
        REQUIRE(tag == tag1);
        stream.release();
    }

    // Write and read cycle 2
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("msg2"), 4, tag2));
    {
        const auto [data, len, tag] = stream.read_next();
        REQUIRE(tag == tag2);
        stream.release();
    }

    // Write and read cycle 3
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("msg3"), 4, tag3));
    {
        const auto [data, len, tag] = stream.read_next();
        REQUIRE(tag == tag3);
        stream.release();
    }

    // Write and read cycle 4
    REQUIRE(stream.write(reinterpret_cast<const uint8_t*>("msg4"), 4, tag4));
    {
        const auto [data, len, tag] = stream.read_next();
        REQUIRE(tag == tag4);
        stream.release();
    }
}
