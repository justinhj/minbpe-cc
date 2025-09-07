# Plan context

The plan is to try boost intrusive list to see if it improves performance over forward list in my tokenizer trainer.

The code in @code/include/Tokenizer.h has a number of references to

forward_list<Token>

It is used also in the test file at @code/tests/test.cpp

Fill in the check boxes at each stage once the program compiles and the tests run.

# Plan steps

[*] Add the include to Tokenizer.h and build with `zig build` to verify that the boost library is present.

#include <boost/intrusive/list.hpp>

[*] Create a data type for intrusive list since it will no longer just contain the primitive. This should live in the Tokenizer.h file

Most likely this would work

    struct TokenNode {
        Token value;
        boost::intrusive::list_member_hook<> hook;
    };
    using TokenList = boost::intrusive::list<
        TokenNode,
        boost::intrusive::member_hook<TokenNode, boost::intrusive::list_member_hook<>, &TokenNode::hook>
    >;

[*] Create a simple test in test.cpp that does the following.

Create a vector of tokens
Create an intrusive list from it
Compare the contents of the vector of token with the intrusive list of tokennode.

Fixed the test with proper memory management to avoid destructor issues
