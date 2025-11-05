#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

TEST_CASE("sanity") {
    CHECK(1 + 1 == 2);
}

#if 0
int main(int argc, char** argv) {
    return doctest::Context(argc, argv).run(); 
}
#endif
