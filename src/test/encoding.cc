#include "include/buffer.h"
#include "include/encoding.h"
#include "include/small_encoding.h"

#include "gtest/gtest.h"

using namespace std;

template < typename T >
static void test_encode_and_decode(const T& src)
{
  bufferlist bl(1000000);
  encode(src, bl);
  T dst;
  bufferlist::iterator i(bl.begin());
  decode(dst, i);
  ASSERT_EQ(src, dst) << "Encoding roundtrip changed the string: orig=" << src << ", but new=" << dst;
}

TEST(EncodingRoundTrip, StringSimple) {
  string my_str("I am the very model of a modern major general");
  test_encode_and_decode < std::string >(my_str);
}

TEST(EncodingRoundTrip, StringEmpty) {
  string my_str("");
  test_encode_and_decode < std::string >(my_str);
}

TEST(EncodingRoundTrip, StringNewline) {
  string my_str("foo bar baz\n");
  test_encode_and_decode < std::string >(my_str);
}

typedef std::multimap < int, std::string > multimap_t;
typedef multimap_t::value_type my_val_ty;

namespace std {
static std::ostream& operator<<(std::ostream& oss, const multimap_t &multimap)
{
  for (multimap_t::const_iterator m = multimap.begin();
       m != multimap.end();
       ++m)
  {
    oss << m->first << "->" << m->second << " ";
  }
  return oss;
}
}

TEST(EncodingRoundTrip, Multimap) {
  multimap_t multimap;
  multimap.insert( my_val_ty(1, "foo") );
  multimap.insert( my_val_ty(2, "bar") );
  multimap.insert( my_val_ty(2, "baz") );
  multimap.insert( my_val_ty(3, "lucky number 3") );
  multimap.insert( my_val_ty(10000, "large number") );

  test_encode_and_decode < multimap_t >(multimap);
}



///////////////////////////////////////////////////////
// ConstructorCounter
///////////////////////////////////////////////////////
template <typename T>
class ConstructorCounter
{
public:
  ConstructorCounter() : data(0)
  {
    default_ctor++;
  }

  explicit ConstructorCounter(const T& data_)
    : data(data_)
  {
    one_arg_ctor++;
  }

  ConstructorCounter(const ConstructorCounter &rhs)
    : data(rhs.data)
  {
    copy_ctor++;
  }

  ConstructorCounter &operator=(const ConstructorCounter &rhs)
  {
    data = rhs.data;
    assigns++;
    return *this;
  }

  static void init(void)
  {
    default_ctor = 0;
    one_arg_ctor = 0;
    copy_ctor = 0;
    assigns = 0;
  }

  static int get_default_ctor(void)
  {
    return default_ctor;
  }

  static int get_one_arg_ctor(void)
  {
    return one_arg_ctor;
  }

  static int get_copy_ctor(void)
  {
    return copy_ctor;
  }

  static int get_assigns(void)
  {
    return assigns;
  }

  bool operator<(const ConstructorCounter &rhs) const
  {
    return data < rhs.data;
  }

  bool operator==(const ConstructorCounter &rhs) const
  {
    return data == rhs.data;
  }

  friend void decode(ConstructorCounter &s, bufferlist::iterator& p)
  {
    ::decode(s.data, p);
  }

  friend void encode(const ConstructorCounter &s, bufferlist& p)
  {
    ::encode(s.data, p);
  }

  friend ostream& operator<<(ostream &oss, const ConstructorCounter &cc)
  {
    oss << cc.data;
    return oss;
  }

  T data;
private:
  static int default_ctor;
  static int one_arg_ctor;
  static int copy_ctor;
  static int assigns;
};

template class ConstructorCounter <int32_t>;
template class ConstructorCounter <int16_t>;

typedef ConstructorCounter <int32_t> my_key_t;
typedef ConstructorCounter <int16_t> my_val_t;
typedef std::multimap < my_key_t, my_val_t > multimap2_t;
typedef multimap2_t::value_type val2_ty;

template <class T> int ConstructorCounter<T>::default_ctor = 0;
template <class T> int ConstructorCounter<T>::one_arg_ctor = 0;
template <class T> int ConstructorCounter<T>::copy_ctor = 0;
template <class T> int ConstructorCounter<T>::assigns = 0;

static std::ostream& operator<<(std::ostream& oss, const multimap2_t &multimap)
{
  for (multimap2_t::const_iterator m = multimap.begin();
       m != multimap.end();
       ++m)
  {
    oss << m->first << "->" << m->second << " ";
  }
  return oss;
}

TEST(EncodingRoundTrip, MultimapConstructorCounter) {
  multimap2_t multimap2;
  multimap2.insert( val2_ty(my_key_t(1), my_val_t(10)) );
  multimap2.insert( val2_ty(my_key_t(2), my_val_t(20)) );
  multimap2.insert( val2_ty(my_key_t(2), my_val_t(30)) );
  multimap2.insert( val2_ty(my_key_t(3), my_val_t(40)) );
  multimap2.insert( val2_ty(my_key_t(10000), my_val_t(1)) );

  my_key_t::init();
  my_val_t::init();
  test_encode_and_decode < multimap2_t >(multimap2);

  EXPECT_EQ(my_key_t::get_default_ctor(), 5);
  EXPECT_EQ(my_key_t::get_one_arg_ctor(), 0);
  EXPECT_EQ(my_key_t::get_copy_ctor(), 5);
  EXPECT_EQ(my_key_t::get_assigns(), 0);

  EXPECT_EQ(my_val_t::get_default_ctor(), 5);
  EXPECT_EQ(my_val_t::get_one_arg_ctor(), 0);
  EXPECT_EQ(my_val_t::get_copy_ctor(), 5);
  EXPECT_EQ(my_val_t::get_assigns(), 0);
}

// make sure that the legacy encode/decode methods are selected
// over the ones defined using templates. the later is likely to
// be slower, see also the definition of "WRITE_INT_DENC" in
// include/denc.h
template<>
void encode<uint64_t, denc_traits<uint64_t>>(const uint64_t&,
                                             bufferlist&,
                                             uint64_t f) {
  static_assert(denc_traits<uint64_t>::supported,
                "should support new encoder");
  static_assert(!denc_traits<uint64_t>::featured,
                "should not be featured");
  ASSERT_EQ(0UL, f);
  // make sure the test fails if i get called
  ASSERT_TRUE(false);
}

template<>
void encode<ceph_le64, denc_traits<ceph_le64>>(const ceph_le64&,
                                               bufferlist&,
                                               uint64_t f) {
  static_assert(denc_traits<ceph_le64>::supported,
                "should support new encoder");
  static_assert(!denc_traits<ceph_le64>::featured,
                "should not be featured");
  ASSERT_EQ(0UL, f);
  // make sure the test fails if i get called
  ASSERT_TRUE(false);
}

TEST(EncodingRoundTrip, Integers) {
  // int types
  {
    uint64_t i = 42;
    test_encode_and_decode(i);
  }
  {
    int16_t i = 42;
    test_encode_and_decode(i);
  }
  {
    bool b = true;
    test_encode_and_decode(b);
  }
  {
    bool b = false;
    test_encode_and_decode(b);
  }
  // raw encoder
  {
    ceph_le64 i;
    i = 42;
    test_encode_and_decode(i);
  }
}

const char* expected_what[] = {
  "buffer::malformed_input: void lame_decoder(int) unknown encoding version > 100",
  "buffer::malformed_input: void lame_decoder(int) no longer understand old encoding version < 100",
  "buffer::malformed_input: void lame_decoder(int) decode past end of struct encoding",
};

void lame_decoder(int which) {
  switch (which) {
  case 0:
    throw buffer::malformed_input(DECODE_ERR_VERSION(__PRETTY_FUNCTION__, 100));
  case 1:
    throw buffer::malformed_input(DECODE_ERR_OLDVERSION(__PRETTY_FUNCTION__, 100));
  case 2:
    throw buffer::malformed_input(DECODE_ERR_PAST(__PRETTY_FUNCTION__));
  }
}

TEST(EncodingException, Macros) {
  for (unsigned i = 0; i < sizeof(expected_what)/sizeof(expected_what[0]); i++) {
    try {
      lame_decoder(i);
    } catch (const exception& e) {
      ASSERT_EQ(string(expected_what[i]), string(e.what()));
    }
  }
}


TEST(small_encoding, varint) {
  uint32_t v[][4] = {
    /* value, varint bytes, signed varint bytes, signed varint bytes (neg) */
    {0, 1, 1, 1},
    {1, 1, 1, 1},
    {2, 1, 1, 1},
    {31, 1, 1, 1},
    {32, 1, 1, 1},
    {0xff, 2, 2, 2},
    {0x100, 2, 2, 2},
    {0xfff, 2, 2, 2},
    {0x1000, 2, 2, 2},
    {0x2000, 2, 3, 3},
    {0x3fff, 2, 3, 3},
    {0x4000, 3, 3, 3},
    {0x4001, 3, 3, 3},
    {0x10001, 3, 3, 3},
    {0x20001, 3, 3, 3},
    {0x40001, 3, 3, 3},
    {0x80001, 3, 3, 3},
    {0x7f0001, 4, 4, 4},
    {0xff00001, 4, 5, 5},
    {0x1ff00001, 5, 5, 5},
    {0xffff0001, 5, 5, 5},
    {0xffffffff, 5, 5, 5},
    {1074790401, 5, 5, 5},
    {0, 0, 0, 0}
  };
  for (unsigned i=0; v[i][1]; ++i) {
    {
      bufferlist bl;
      small_encode_varint(v[i][0], bl);
      cout << std::hex << v[i][0] << "\t" << v[i][1] << "\t";
      bl.hexdump(cout, false);
      cout << std::endl;
      ASSERT_EQ(bl.length(), v[i][1]);
      uint32_t u;
      auto p = bl.begin();
      small_decode_varint(u, p);
      ASSERT_EQ(v[i][0], u);
    }
    {
      bufferlist bl;
      small_encode_signed_varint(v[i][0], bl);
      cout << std::hex << v[i][0] << "\t" << v[i][2] << "\t";
      bl.hexdump(cout, false);
      cout << std::endl;
      ASSERT_EQ(bl.length(), v[i][2]);
      int32_t u;
      auto p = bl.begin();
      small_decode_signed_varint(u, p);
      ASSERT_EQ((int32_t)v[i][0], u);
    }
    {
      bufferlist bl;
      int64_t x = -(int64_t)v[i][0];
      small_encode_signed_varint(x, bl);
      cout << std::dec << x << std::hex << "\t" << v[i][3] << "\t";
      bl.hexdump(cout, false);
      cout << std::endl;
      ASSERT_EQ(bl.length(), v[i][3]);
      int64_t u;
      auto p = bl.begin();
      small_decode_signed_varint(u, p);
      ASSERT_EQ(x, u);
    }
  }
}

TEST(small_encoding, varint_lowz) {
  uint32_t v[][4] = {
    /* value, bytes encoded */
    {0, 1, 1, 1},
    {1, 1, 1, 1},
    {2, 1, 1, 1},
    {15, 1, 1, 1},
    {16, 1, 1, 1},
    {31, 1, 2, 2},
    {63, 2, 2, 2},
    {64, 1, 1, 1},
    {0xff, 2, 2, 2},
    {0x100, 1, 1, 1},
    {0x7ff, 2, 2, 2},
    {0xfff, 2, 3, 3},
    {0x1000, 1, 1, 1},
    {0x4000, 1, 1, 1},
    {0x8000, 1, 1, 1},
    {0x10000, 1, 2, 2},
    {0x20000, 2, 2, 2},
    {0x40000, 2, 2, 2},
    {0x80000, 2, 2, 2},
    {0x7f0000, 2, 2, 2},
    {0xffff0000, 4, 4, 4},
    {0xffffffff, 5, 5, 5},
    {0x41000000, 3, 4, 4},
    {0, 0, 0, 0}
  };
  for (unsigned i=0; v[i][1]; ++i) {
    {
      bufferlist bl;
      small_encode_varint_lowz(v[i][0], bl);
      cout << std::hex << v[i][0] << "\t" << v[i][1] << "\t";
      bl.hexdump(cout, false);
      cout << std::endl;
      ASSERT_EQ(bl.length(), v[i][1]);
      uint32_t u;
      auto p = bl.begin();
      small_decode_varint_lowz(u, p);
      ASSERT_EQ(v[i][0], u);
    }
    {
      bufferlist bl;
      int64_t x = v[i][0];
      small_encode_signed_varint_lowz(x, bl);
      cout << std::hex << x << "\t" << v[i][1] << "\t";
      bl.hexdump(cout, false);
      cout << std::endl;
      ASSERT_EQ(bl.length(), v[i][2]);
      int64_t u;
      auto p = bl.begin();
      small_decode_signed_varint_lowz(u, p);
      ASSERT_EQ(x, u);
    }
    {
      bufferlist bl;
      int64_t x = -(int64_t)v[i][0];
      small_encode_signed_varint_lowz(x, bl);
      cout << std::dec << x << "\t" << v[i][1] << "\t";
      bl.hexdump(cout, false);
      cout << std::endl;
      ASSERT_EQ(bl.length(), v[i][3]);
      int64_t u;
      auto p = bl.begin();
      small_decode_signed_varint_lowz(u, p);
      ASSERT_EQ(x, u);
    }    
  }
}

TEST(small_encoding, lba) {
  uint64_t v[][2] = {
    /* value, bytes encoded */
    {0, 4},
    {1, 4},
    {0xff, 4},
    {0x10000, 4},
    {0x7f0000, 4},
    {0xffff0000, 4},
    {0x0fffffff, 4},
    {0x1fffffff, 5},
    {0xffffffff, 5},
    {0x3fffffff000, 4},
    {0x7fffffff000, 5},
    {0x1fffffff0000, 4},
    {0x3fffffff0000, 5},
    {0xfffffff00000, 4},
    {0x1fffffff00000, 5},
    {0x41000000, 4},
    {0, 0}
  };
  for (unsigned i=0; v[i][1]; ++i) {
    bufferlist bl;
    small_encode_lba(v[i][0], bl);
    cout << std::hex << v[i][0] << "\t" << v[i][1] << "\t";
    bl.hexdump(cout, false);
    cout << std::endl;
    ASSERT_EQ(bl.length(), v[i][1]);
    uint64_t u;
    auto p = bl.begin();
    small_decode_lba(u, p);
    ASSERT_EQ(v[i][0], u);
  }

}
