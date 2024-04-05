#include "BasicTokenizer.hpp"
#include <fstream>
#include <iostream>
using std::string;

int char_to_int(char8_t c) {
  return c < 0 ? c + 256 : c; 
}
const string test_strings[] = {
  "abab",
  "FILE:tests/sample.txt",
  "FILE:tests/taylorswift.txt",
  "Ｕｎｉｃｏｄｅ! 🅤🅝🅘🅒🅞🅓🅔‽ 🇺‌🇳‌🇮‌🇨‌🇴‌🇩‌🇪! 😄 The very name strikes fear and awe into the hearts of programmers worldwide. We all know we ought to “support Unicode” in our software (whatever that means—like using wchar_t for all the strings, right?). But Unicode can be abstruse, and diving into the thousand-page Unicode Standard plus its dozens of supplementary annexes, reports, and notes can be more than a little intimidating. I don’t blame programmers for still finding the whole thing mysterious, even 30 years after Unicode’s inception.",
  "", // empty string
  "?", // single character
  "안 👋 hello", // also from the youtube video
  "hello world!!!? (안녕하세요!) lol123 😉", // fun small string
  "FILE:taylorswift.txt", // FILE: is handled as a special string in unpack() TODO
  "But Unicode can be abstruse plus we know we out to be still finidng the whole thing mysterious",
};
string getTestString(int index) {
  if (index < 0 || index >= sizeof(test_strings) / sizeof(test_strings[0])) {
    std::cerr << "Index out of bounds." << std::endl;
    return "";
  }
  string str = test_strings[index];
  const string fileIndicator = "FILE:";
  if (str.find(fileIndicator) != str.npos) {
    std::string file_path = str.substr(fileIndicator.length());
    std::ifstream file(file_path);
    std::string file_content;

    if (file) {
      file_content.assign((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
      file.close();
      return file_content;
    } else {
      std::cerr << "Could not open file: " << file_path << std::endl;
      return "";
    }
  }
  cout << str << " does not begin with " << fileIndicator << "\n";
  return str;
}
// Take the BasicTokenizer for a test drive
int main(int argc, char *argv[]) {
  auto verbose = true;
  auto input = getTestString(3);

  BasicTokenizer bt;
  bt.train(input, 256 + 20, verbose);
  auto encoded = bt.encode(input, verbose);
  cout << "Original string length " << input.size() << "\n";
  cout << "Encoded string length " << encoded.size() << "\n";
  auto decoded = bt.decode(encoded, false);

  /* cout << "Original string -- " << input << "\n"; */
  /* cout << "Decoded string  -- " << decoded << "\n"; */
}
