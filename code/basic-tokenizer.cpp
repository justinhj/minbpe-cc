#include "BasicTokenizer.hpp"

using std::u8string;

// Take the BasicTokenizer for a test drive
int main(int argc, char *argv[]) {

  const u8string test_strings[] = {
    u8"ababcdcdcdabcdef",
    u8"Ｕｎｉｃｏｄｅ! 🅤🅝🅘🅒🅞🅓🅔‽ 🇺‌🇳‌🇮‌🇨‌🇴‌🇩‌🇪! 😄 The very name strikes fear and awe into the hearts of programmers worldwide. We all know we ought to “support Unicode” in our software (whatever that means—like using wchar_t for all the strings, right?). But Unicode can be abstruse, and diving into the thousand-page Unicode Standard plus its dozens of supplementary annexes, reports, and notes can be more than a little intimidating. I don’t blame programmers for still finding the whole thing mysterious, even 30 years after Unicode’s inception.", // From Kaparthy's youtube video
    u8"", // empty string
    u8"?", // single character
    u8"안 👋 hello", // also from the youtube video
    u8"hello world!!!? (안녕하세요!) lol123 😉", // fun small string
    u8"FILE:taylorswift.txt", // FILE: is handled as a special string in unpack() TODO
    u8"But Unicode can be abstruse plus we know we out to be still finidng the whole thing mysterious",
  };

  auto verbose = true;

  BasicTokenizer bt;
  bt.train(test_strings[1], 256 + 20, verbose);
  auto encoded = bt.encode(test_strings[1], verbose);
  cout << "Original string length " << test_strings[1].size() << "\n";
  cout << "Encoded string length " << encoded.size() << "\n";
  auto decoded = bt.decode(encoded, false);

  cout << "Original string " << reinterpret_cast<const char *>(test_strings[1].data()) << "\n";
  cout << "Decoded string " << reinterpret_cast<const char *>(decoded.data()) << "\n";
}
