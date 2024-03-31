#include "BasicTokenizer.hpp"

using std::u8string;

// Take the BasicTokenizer for a test drive
int main(int argc, char *argv[]) {

  const u8string test_strings[] = {
    u8"", // empty string
    u8"?", // single character
    /* "Ｕｎｉｃｏｄｅ! 🅤🅝🅘🅒🅞🅓🅔‽ 🇺‌🇳‌🇮‌🇨‌🇴‌🇩‌🇪! 😄 The very name strikes fear and awe into the hearts of programmers worldwide. We all know we ought to “support Unicode” in our software (whatever that means—like using wchar_t for all the strings, right?). But Unicode can be abstruse, and diving into the thousand-page Unicode Standard plus its dozens of supplementary annexes, reports, and notes can be more than a little intimidating. I don’t blame programmers for still finding the whole thing mysterious, even 30 years after Unicode’s inception.", */
    u8"안 👋 hello", // from the youtube video
    u8"hello world!!!? (안녕하세요!) lol123 😉", // fun small string
    u8"FILE:taylorswift.txt", // FILE: is handled as a special string in unpack() TODO
  };

  BasicTokenizer bt;
  bt.train(test_strings[2], 266, true);
}
