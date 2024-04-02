#include "BasicTokenizer.hpp"

using std::string;

int char_to_int(char8_t c) {
  return c < 0 ? c + 256 : c; 
}
// Take the BasicTokenizer for a test drive
int main(int argc, char *argv[]) {

  const string test_strings[] = {
    "ababcdcdcdabcdef",
    "Ｕｎｉｃｏｄｅ! 🅤🅝🅘🅒🅞🅓🅔‽ 🇺‌🇳‌🇮‌🇨‌🇴‌🇩‌🇪! 😄 The very name strikes fear and awe into the hearts of programmers worldwide. We all know we ought to “support Unicode” in our software (whatever that means—like using wchar_t for all the strings, right?). But Unicode can be abstruse, and diving into the thousand-page Unicode Standard plus its dozens of supplementary annexes, reports, and notes can be more than a little intimidating. I don’t blame programmers for still finding the whole thing mysterious, even 30 years after Unicode’s inception.", // From Kaparthy's youtube video
    "", // empty string
    "?", // single character
    "안 👋 hello", // also from the youtube video
    "hello world!!!? (안녕하세요!) lol123 😉", // fun small string
    "FILE:taylorswift.txt", // FILE: is handled as a special string in unpack() TODO
    "But Unicode can be abstruse plus we know we out to be still finidng the whole thing mysterious",
  };

  auto verbose = true;

  BasicTokenizer bt;
  bt.train(test_strings[1], 256 + 20, verbose);
  auto encoded = bt.encode(test_strings[1], verbose);
  cout << "Original string length " << test_strings[1].size() << "\n";
  cout << "Encoded string length " << encoded.size() << "\n";
  auto decoded = bt.decode(encoded, false);

  cout << "Original string " << test_strings[1] << "\n";
  cout << "Decoded string " << decoded << "\n";

  string text = "안 👋 hello";
  vector<int> codes;
  for(int c : text) {
    cout << char_to_int(c) <<  ' ';
    codes.push_back(char_to_int(c));
  }
  cout << "\n";
  cout << text << "\n";

  string text2;
  for(int c : codes) {
    cout << c <<  ' ';
    text2.push_back(c);
  }
  cout << "\n";

  cout << text2 << "\n";
}
