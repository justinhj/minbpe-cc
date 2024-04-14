#include "BasicTokenizer.hpp"
#include "RegexTokenizer.hpp"
#include <fstream>
#include <iostream>
/* #include "../opt/CLI11/CLI11.hpp" */
using std::string;
/* using std::optional; */

// Xcode 14.3 14E222b optional monadic
// same for zip views
// but I have Xcode-15.3?
// Apple clang version 14.0.3 (clang-1403.0.22.14.1)

const string test_strings[] = {
  "with 20 and 201 and 2001 what is happen?",
  "FILE:tests/sample.txt",
  "FILE:tests/taylorswift.txt",
  "FILE:tests/bible.txt",
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
      cout << "Loading input file: " << file_path << "\n";
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
  /* CLI::App app{"App description"}; */
  /* argv = app.ensure_utf8(argv); */

  /* optional<string> filename; */
  /* app.add_option("-f,--file", filename, "A help string"); */

  /* CLI11_PARSE(app, argc, argv); */

  /* if(filename.has_value()) { */
  /*   cout << "filename " << filename.value() << "\n"; */
  /* } */

    const string text = R"(RCA Republic Big Machine
Website	www.taylorswift.com Edit this at Wikidata
Signature
Taylor Alison Swift (born December 13, 1989) is an American singer-songwriter. Her versatile artistry, songwriting, and entrepreneurship have influenced the music industry, popular culture, and politics, and her life is a subject of widespread media coverage.
안 👋 hello
)";

  /* RegexTokenizer rt = RegexTokenizer(); */
  /* RegexTokenizer rt = RegexTokenizer(RegexTokenizer::GPT2_SPLIT_PATTERN); */
  /* rt.train(text, 256 + 256, true); */
  
  /* return 0; */

  using std::chrono::high_resolution_clock;
  using std::chrono::duration_cast;
  using std::chrono::milliseconds;

  auto t1 = high_resolution_clock::now(); // Record start time
  auto verbose = true;
  auto input = getTestString(3);

  /* BasicTokenizer bt; */
  /* bt.train(input, 256 + 256, verbose); */

  RegexTokenizer rt = RegexTokenizer(RegexTokenizer::GPT4_SPLIT_PATTERN);
  rt.train(input, 256 + 256, verbose);

  auto t2 = high_resolution_clock::now(); // Record end time
  auto duration = t2 - t1;
  auto ms_int = duration_cast<milliseconds>(duration).count();

  std::cout << "Execution time: " << ms_int / 1000.0 << " seconds" << std::endl;

  /* auto encoded = bt.encode(input, verbose); */
  /* cout << "Original string length " << input.size() << "\n"; */
  /* cout << "Encoded string length " << encoded.size() << "\n"; */
  /* auto decoded = bt.decode(encoded, false); */

  /* cout << "Original string -- " << input << "\n"; */
  /* cout << "Decoded string  -- " << decoded << "\n"; */
}
