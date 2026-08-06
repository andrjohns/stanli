// Sexp parser for stanc3 --debug-transformed-mir output.
#include <stanli/sexp.hpp>

#include <cstdio>
#include <string>

static int failures = 0;
static void check(bool ok, const std::string& what) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", what.c_str());
  }
}

int main() {
  using stanli::sexp::Node;
  using stanli::sexp::parse;

  {
    Node n = parse("(a (b \"c d\") <opaque>)");
    check(!n.is_atom(), "root is list");
    check(n.size() == 3, "root size 3");
    check(n[0].is_atom() && n[0].atom == "a", "kid0 atom a");
    check(n[1].head_is("b"), "kid1 head b");
    check(n[1][1].atom == "c d", "quoted string contents");
    check(n[2].atom == "<opaque>", "opaque atom");
  }
  {
    // Nested + escapes + newlines, as stanc emits.
    Node n = parse("((x 1) (y -2.5)\n (s \"a\\\"b\"))");
    check(n.size() == 3, "nested size");
    check(n[1][1].atom == "-2.5", "numeric atom");
    check(n[2][1].atom == "a\"b", "escaped quote");
  }
  {
    bool threw = false;
    try {
      parse("(a (b)");
    } catch (const std::exception& e) {
      threw = std::string(e.what()).find("position") != std::string::npos;
    }
    check(threw, "unclosed list reports position");
  }
  {
    // Adjacent lists without spaces, comments not expected in stanc output.
    Node n = parse("(a(b c)d)");
    check(n.size() == 3 && n[1].head_is("b") && n[2].atom == "d",
          "tight adjacency");
  }

  if (failures == 0) std::printf("test_sexp OK\n");
  return failures == 0 ? 0 : 1;
}
