#include "bitreader.hpp"
#include "test.hpp"

#include <random>

static BitReader br;
static std::default_random_engine rng;
static auto &rx = br.rx_fifo;
const int bit_length = 100;

void send_byte(uint32_t time, uint8_t b)
{
  br.edge(time, 0);
  time += bit_length;
  int last_level = 0;
  for (int i = 0; i < 8; ++i) {
    int level = b & 1;
    if (level != last_level) {
      br.edge(time, level);
      last_level = level;
    }
    b >>= 1;
    time += bit_length;
  }
  if (last_level == 0)
    br.edge(time, 1);
  time += bit_length;
}

void send_byte_jitter(uint32_t time, uint8_t b, uint32_t max_jitter)
{
  std::uniform_int_distribution<int> dist(0, max_jitter);
  br.edge(time + dist(rng), 0);
  time += bit_length;
  int last_level = 0;
  for (int i = 0; i < 8; ++i) {
    int level = b & 1;
    if (level != last_level) {
      br.edge(time + dist(rng), level);
      last_level = level;
    }
    b >>= 1;
    time += bit_length;
  }
  if (last_level == 0)
    br.edge(time + dist(rng), 1);
  time += bit_length;
}

TEST(basic1)
{
  send_byte(0, 251);
  br.check(1000);
  assert(!rx.empty());
  assert_equal(251, rx.get());
}

TEST(basic2)
{
  br.edge(0, 0);
  br.edge(900, 1);
  assert(!rx.empty());
  assert_equal(0x00, rx.get());
}

TEST(basic3)
{
  br.edge(0, 0);
  br.edge(100, 1);
  br.check(900);
  assert(rx.empty());
  br.check(1000);
  assert(!rx.empty());
  assert_equal(0xFF, rx.get());
}

TEST(corruption_delayed)
{
  br.edge(0, 0);
  br.edge(599, 1);
  br.edge(600, 0);
  br.edge(601, 1);
  br.edge(602, 0);
  br.edge(900, 1);
  br.check(1000);
  send_byte(1000, 0x55);
  while (!rx.empty()) {
    uint8_t byte = rx.get();
    if (rx.empty())
      assert_equal(0x55, byte);
  }
}

TEST(corruption2)
{
  std::uniform_int_distribution<int> dist(0, 30);
  int level = 0;
  for (uint32_t time = 0; time < 10000; time += dist(rng)) {
    br.edge(time, level);
    level = !level;
  }
  if (level == 0)
    br.edge(11000, 1);
  br.check(14000);
  while (!rx.empty())
    rx.get();
  for (int i = 0; i < 256; ++i) {
    send_byte(15000 + i * 1000, i);
    br.check(15000 + i * 1000 + 1000);
    assert(!rx.empty());
    assert_equal(i, rx.get());
    assert(rx.empty());
  }
}

TEST(corruption_delayed2)
{
  br.edge(0, 0);
  br.check(1000);
  br.edge(1101, 1);
  br.edge(1102, 0);
  br.edge(1103, 1);
  br.edge(1104, 0);
  br.edge(1108, 1);
  br.check(2000);
  send_byte(2000, 0x55);
  br.check(3000);
  br.check(5000);
  send_byte(6000, 0x55);
  while (!rx.empty()) {
    uint8_t byte = rx.get();
    if (rx.empty())
      assert_equal(0x55, byte);
  }
}

TEST(stop_bit_delayed)
{
  br.edge(0, 0);
  br.check(1049);
  assert(!rx.empty());
  assert_equal(0x00, rx.get());
  br.edge(1000 + 110, 1);
  br.edge(1100 + 20, 0);
  br.edge(2000 + 20, 1);
  assert(!rx.empty());
  assert_equal(0x00, rx.get());
}

TEST(buffering)
{
  for (int i = 0; i < 6; ++i) {
    br.edge(i * 1000, 0);
    br.edge(i * 1000 + 900, 1);
  }
  for (int i = 0; i < 6; ++i) {
    assert(!rx.empty());
    assert_equal(0x00, rx.get());
  }
}

TEST(all_bytes)
{
  for (int i = 0; i < 256; ++i) {
    send_byte(i * 1000, i);
    br.check(i * 1000 + 1000);
    assert(!rx.empty());
    assert_equal(i, rx.get());
  }
}

TEST(all_bytes_fifo)
{
  for (int i = 0; i < 8; ++i)
    send_byte(i * 1000, i);
  for (int i = 8; i < 256; ++i) {
    assert_equal(i - 8, rx.get());
    send_byte(i * 1000, i);
  }
  for (int i = 248; i < 256; ++i) {
    if (i == 255)
      br.check(256000);
    assert_equal(i, rx.get());
  }
}

TEST(all_bytes_with_jitter)
{
  for (int i = 0; i < 256; ++i) {
    send_byte_jitter(i * 1000, i, 49);
    br.check(i * 1000 + 1000);
    assert(!rx.empty());
    assert_equal(i, rx.get());
  }
}

TEST(seq2)
{
  br.edge(0, 0);
  br.edge(100, 1);
  assert(rx.empty());
  br.edge(1000, 0);
  assert(!rx.empty());
  assert_equal(0xff, rx.get());
  br.edge(1100, 1);
  assert(rx.empty());
  br.edge(2000, 0);
  assert(!rx.empty());
  assert_equal(0xff, rx.get());
  br.edge(2900, 1);
  assert(!rx.empty());
  assert_equal(0x00, rx.get());
}

int main()
{
  for (auto fn : tests)
    fn();
  std::cout << failed << " failed, " << passed << " passed" << std::endl;
}