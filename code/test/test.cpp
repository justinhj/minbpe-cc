#define CATCH_CONFIG_MAIN
#include "Catch2/catch2.hpp"

#include "Tokenizer.hpp"

unsigned int Factorial( unsigned int number ) {
    return number <= 1 ? number : Factorial(number-1)*number;
}

TEST_CASE( "Factorials are computed", "[factorial]" ) {
    REQUIRE( Factorial(1) == 1 );
    REQUIRE( Factorial(2) == 2 );
    REQUIRE( Factorial(3) == 6 );
    REQUIRE( Factorial(10) == 3628800 );
}

TEST_CASE("PairCount add and count", "[paircount]") {
    PairCount pc;
    REQUIRE( pc.get_count() == 0 );
    pc.increment_freq_count(make_tuple(1,2));
    REQUIRE( pc.get_count() == 1 );
    REQUIRE( pc.get_insert_order() == 1 );
}


