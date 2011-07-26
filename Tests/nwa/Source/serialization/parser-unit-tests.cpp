#include <sstream>

#include "gtest/gtest.h"

#include "wali/nwa/NWA.hpp"
#include "wali/nwa/NWAParser.hpp"

#include "Tests/nwa/Source/fixtures.hpp"
#include "Tests/nwa/Source/class-NWA/supporting.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>
#include <exception>
#include <cstdlib>
#include <ctime>
#include <cctype>

namespace wali {
  namespace nwa {
    namespace parser {
      namespace details {

extern
void
discardws(std::istream & is)
;

void
test_discardws()
{
  std::stringstream ss("  helllo");

  assert(ss.peek() == ' '); // mostly just testing .peek does what I think
  discardws(ss);
  assert(ss.peek() == 'h'); // make sure we discard up to the h
  discardws(ss);
  assert(ss.peek() == 'h'); // make sure we discard nothing if there's no WS
}

extern
std::string
read_lit(std::istream & is, std::string const & lit)
  ;

struct CharactersDifferException : std::exception {
  std::string lit;
  int differing_pos;
  int read_char;
  const char * message;
  int line;

  CharactersDifferException(std::string const & l, int p, int c, int li)
    : lit(l)
    , differing_pos(p)
    , read_char(c)
    , line(li)
  {
    assert(c!=-1);
    message = err_msg();
  }

  const char * what() const throw()
  {
    return message;
  }

  const char * err_msg() const
  {
    try {
      std::stringstream ss;
      ss << "CharactersDifferException when reading literal [" << lit << "]\n";
      ss << "               Read " << (char)read_char
         << " instead of " << lit[differing_pos]
         << " at position " << differing_pos
         << " of the literal; line " << line << "\n";

      std::string s = ss.str();
      char * pm = new char[s.size()];
            
      std::copy(s.begin(), s.end(), pm);
      return pm;
    }
    catch (std::exception const & e) {
      (void) e;
      return "[CharactersDifferException: cannot allocate memory]\n";
    }
  }

  ~CharactersDifferException() throw()
  {
    if (message[0] != '[') {
      // We allocated this in err_msg
      delete message;
    }
  }
};

struct StreamTooShortException   : std::exception {};


void
test_read_lit()
{
  std::stringstream empty("");
  std::stringstream short_("he");
  std::stringstream exact("hello");
  std::stringstream long_("hello world");
  std::stringstream differ("goodbye world");

  std::string query = "hello";

  // Make sure if we're too short we get an exception
  bool exn = false;
  try {
    read_lit(empty, query);
  }
  catch(StreamTooShortException const & e) {
    (void) e;
    exn = true;
  }
  assert(exn);

  exn = false;
  try {
    read_lit(short_, query);
  }
  catch(StreamTooShortException const & e) {
    (void) e;
    exn = true;
  }
  assert(exn);

  // Make sure if we start with the query we get it right
  assert(read_lit(exact, query) == query);
  assert(read_lit(long_, query) == query);
  assert(long_.peek() == 'w'); // did it skip over WS?

  // Make sure if we start with stuff other than the query we get an
  // exception
  exn = false;
  try {
    read_lit(differ, query);
  }
  catch(CharactersDifferException const & e) {
    (void) e;
    exn = true;
  }
  assert(exn);
}

        
extern
std::string
read_name(std::istream & is)
  ;        

void
test_read_name()
{
  // Test several that look like they are a non-final name in a list
  std::stringstream ss1(" hello, world");
  std::stringstream ss2(" hello , world");
  std::stringstream ss3(" hello (ignore), world");
  std::stringstream ss4(" hello (ignore) , world");
  std::stringstream ss5(" hello (<.,h.439[0oetnh>), world");
  std::stringstream ss6(" hello (ighonetohaoey489) , world");

  // In every one of those, "hello" should be considered the key
  assert(read_name(ss1) == "hello");
  assert(read_name(ss2) == "hello");
  assert(read_name(ss3) == "hello");
  assert(read_name(ss4) == "hello");
  assert(read_name(ss5) == "hello");
  assert(read_name(ss6) == "hello");

  // And it should always remove up to but not including the comma
  assert(ss1.peek() == ',');
  assert(ss2.peek() == ',');
  assert(ss3.peek() == ',');
  assert(ss4.peek() == ',');
  assert(ss5.peek() == ',');
  assert(ss6.peek() == ',');

  // Test several that look like a final-one in a list
  std::stringstream ss7(" hello)");
  std::stringstream ss8(" hello (o,nthent9r34<>))");
  std::stringstream ss9(" hello (o,nthent9r34<>) )");
  std::stringstream ss10(" hello}");
  std::stringstream ss11(" hello (o,nthent9r34<>)}");
  std::stringstream ss12(" hello (o,nthent9r34<>)\n}");

  assert(read_name(ss7) == "hello");
  assert(read_name(ss8) == "hello");
  assert(read_name(ss9) == "hello");
  assert(read_name(ss10) == "hello");
  assert(read_name(ss11) == "hello");
  assert(read_name(ss12) == "hello");

  // Make sure it stops at the appropriate delimiter
  assert(ss7.peek() == ')');
  assert(ss8.peek() == ')');
  assert(ss9.peek() == ')');
  assert(ss10.peek() == '}');
  assert(ss11.peek() == '}');
  assert(ss12.peek() == '}');

  // Now test a hard case: parens as the key
  std::stringstream ss13("(key1,2) bar");
  std::stringstream ss14("{key1,2} bar");
  std::stringstream ss15("(<key1,2>) bar");
  std::stringstream ss16("({key1,2}) bar");
  std::stringstream ss17("({key1,2)) bar");
    
  assert(read_name(ss13) == "(key1,2)");
  assert(read_name(ss14) == "{key1,2}");
  assert(read_name(ss15) == "(<key1,2>)");
  assert(read_name(ss16) == "({key1,2})");
  assert(read_name(ss17) == "({key1,2))");
}

        
extern
KeyTriple
read_triple(std::istream & is)
  ;        

extern        
KeyQuad
read_quad(std::istream & is)
  ;
        
void
test_read_triple_quad()
{
  std::stringstream ss1("(a, b, c)");
  std::stringstream ss2("(a, b, c)z");
  std::stringstream ss3("(a,b, c)(d, e, f)");

  KeyTriple ans3(getKey("a"), getKey("b"), getKey("c"));
    
  assert(read_triple(ss1) == ans3);
  assert(read_triple(ss2) == ans3);
  assert(read_triple(ss3) == ans3);
    
  std::stringstream ss4("(a (antohe), b (tnoehn), c (otnhento))z");
  std::stringstream ss5("(a (jab3,9030), b,\nc)\n}");

  assert(read_triple(ss4) == ans3);
  assert(read_triple(ss5) == ans3);
    
  std::stringstream ss6("(a, b, c, d)");
  std::stringstream ss7("(a, b, c, d)z");
  std::stringstream ss8("(a,b, c, d)(h, e, f)");

  KeyQuad ans4(getKey("a"), getKey("b"), getKey("c"), getKey("d"));

  assert(read_quad(ss6) == ans4);
  assert(read_quad(ss7) == ans4);
  assert(read_quad(ss8) == ans4);
        
  std::stringstream ss9("(a (antohe), b (tnoehn), c (otnhento), d (otnheont349))z");
  std::stringstream ss10("(a (jab3,9030), b,\nc, d)\n}");

  assert(read_quad(ss9)  == ans4);
  assert(read_quad(ss10) == ans4);
}


extern
std::vector<std::string>
read_name_list(std::istream & is)
  ;

extern
std::vector<KeyTriple>
read_triple_list(std::istream & is)
  ;

extern  
std::vector<KeyQuad>
read_quad_list(std::istream & is)
  ;


void
test_read_lists()
{
  // These should return a zero list for all three kinds of lists.  When we
  // do the corresponding asserts below, make sure that they (really, ss2)
  // is unchanged
  std::stringstream ss1("");
  std::stringstream ss2("}");

  assert(read_name_list(ss1).size() == 0);
  assert(read_name_list(ss2).size() == 0);
  ss1.str(""); // reset ss1 because C++ is dumb and 'peek()' modifes goodbit
  assert(ss2.peek() == '}');
  assert(read_triple_list(ss1).size() == 0);
  assert(read_triple_list(ss2).size() == 0);
  ss1.str("");
  assert(ss2.peek() == '}');
  assert(read_quad_list(ss1).size() == 0);
  assert(read_quad_list(ss2).size() == 0);
  ss1.str("");
  assert(ss2.peek() == '}');

  // Test read_name_list. We only test the sizes on the first four tests,
  // then test all the elements on the later ones.
  std::stringstream ss3("name1");
  std::stringstream ss4("name1 }");
  std::stringstream ss5("name1, name2");
  std::stringstream ss6("name1, name2 }");
  std::stringstream ss7("name1, name2, name3, name4");
  std::stringstream ss8("name1, name2, name3, name4 }");

  assert(read_name_list(ss3).size() == 1);
  assert(read_name_list(ss4).size() == 1);
  assert(read_name_list(ss5).size() == 2);
  assert(read_name_list(ss6).size() == 2);
  std::vector<std::string> v7 = read_name_list(ss7);
  assert(v7.size() == 4);
  assert(v7[0] == "name1");
  assert(v7[1] == "name2");
  assert(v7[2] == "name3");
  assert(v7[3] == "name4");
  std::vector<std::string> v8 = read_name_list(ss8);
  assert(v8.size() == 4);
  assert(v8[0] == "name1");
  assert(v8[1] == "name2");
  assert(v8[2] == "name3");
  assert(v8[3] == "name4");

  // (I skip ss9 for easy of typing)

  // Test read_triple_list. We only test a couple of these here, and only
  // sizes; other problems should have been shaken out by now.
  std::stringstream ss10("(a,b, c)");
  std::stringstream ss11("(a,b, c) }");
  std::stringstream ss12("(a,b, c), (d, e, f)");
  std::stringstream ss13("(a,b, c), (d, e, f), (g, h, i) }");

  assert(read_triple_list(ss10).size() == 1);
  assert(read_triple_list(ss11).size() == 1);
  assert(read_triple_list(ss12).size() == 2);
  assert(read_triple_list(ss13).size() == 3);
    
  // Test read_quad_list. Again fairly light on purpose
  std::stringstream ss14("(a,b, c,d)");
  std::stringstream ss15("(a,b, c,d) }");
  std::stringstream ss16("(a,b,c,d),(d,e,f,g), (oth, 97go, n.hy429,<0932g>)");
  std::stringstream ss17("(a,b, c,g), (d, e, f,gc) }");

  assert(read_quad_list(ss14).size() == 1);
  assert(read_quad_list(ss15).size() == 1);
  assert(read_quad_list(ss16).size() == 3);
  assert(read_quad_list(ss17).size() == 2);
}


extern        
void
read_sigma_block(std::istream & is, NWARefPtr nwa)
  ;

extern        
void
read_state_block(std::istream & is, NWARefPtr nwa)
  ;

extern
void
read_delta_block(std::istream & is, NWARefPtr nwa)
  ;
        


void
test_read_blocks()
{
  NWARefPtr nwa = new wali::nwa::NWA();
    
  // These tests call read_*_block (implicitly making sure there are no
  // exceptions or assertions), then make sure they read everything they
  // should. For the first few tests, that means reading to the
  // end-of-string; later it means reading until the Z.
  std::stringstream ss1("Q: { }");
  std::stringstream ss2("Q: ");
  std::stringstream ss3("Q: { a }");
  std::stringstream ss4("Q: { a, b }");
  std::stringstream ss5("Q: a, b ");

  read_state_block(ss1, nwa);
  read_state_block(ss2, nwa);
  read_state_block(ss3, nwa);
  read_state_block(ss4, nwa);
  read_state_block(ss5, nwa);

  assert(ss1.peek() == -1);
  assert(ss2.peek() == -1);
  assert(ss3.peek() == -1);
  assert(ss4.peek() == -1);
  assert(ss5.peek() == -1);
    
  std::stringstream ss6("Q: a, b Z");
  std::stringstream ss7("Qf: a, b Z");
  std::stringstream ss8("Q0: a, b Z");
  std::stringstream ss9("Sigma: a Z");

  read_state_block(ss6, nwa);
  read_state_block(ss7, nwa);
  read_state_block(ss8, nwa);
  read_sigma_block(ss9, nwa);

  assert(ss6.peek() == 'Z');
  assert(ss7.peek() == 'Z');
  assert(ss8.peek() == 'Z');
  assert(ss9.peek() == 'Z');

  std::stringstream ss10("Delta_i: (a, b, c), (a, b, c) Z");
  std::stringstream ss11("Delta_c: (a, b, c), (a, b, c) Z");
  std::stringstream ss12("Delta_r: (a, b, c, d), (a, b, c, d) Z");

  read_delta_block(ss10, nwa);
  read_delta_block(ss11, nwa);
  read_delta_block(ss12, nwa);

  assert(ss10.peek() == 'Z');
  assert(ss11.peek() == 'Z');
  assert(ss12.peek() == 'Z');
}




}}}}

////////////////////////////////////////////////////////////////////////////////
/// READ_NWA
///
/// nwa-description  ::= ('nwa' name? ':'?)? '{'? block+ '}'?
///   with an opening brace required if 'nwa' is present and name is not
///   (and yes, there must be a space between the name and either : or {)
///
/// Reads in an nwa-description

namespace wali {
  namespace nwa {
    using namespace parser::details;

    void
    test_read_map()
    {
      std::string nwas =
        "nwa main : { Q0: start_m Qf: end_m }\n"
        "nwa foo    { Q0: start_f Qf: end_f Q: node1 }\n"
        "nwa bar    { Q0: start_r Qf: end_r Q: node1, node2 }\n"
        "nwa baz    { Q0: start_z Qf: end_z Q: node1, node2, node3 }\n"
        ;

      std::stringstream ss(nwas);

      ProcedureMap m = read_nwa_proc_set(ss);

      assert(m.size() == 4);

      assert(m["main"]->sizeStates() == 2);
      assert(m["foo"]->sizeStates() == 3);
      assert(m["bar"]->sizeStates() == 4);
      assert(m["baz"]->sizeStates() == 5);
    }

        
    void
    test_read_nwa()
    {
      std::string mattsNwa =
        "Q: {" "\n"
        "  stuck (=2)," "\n"
        "  (key#6,3) (=16)," "\n"
        "  (key#6,4) (=17)," "\n"
        "  (key#7,3) (=18)," "\n"
        "  (key#4,3) (=19)," "\n"
        "  (key#3,3) (=20)," "\n"
        "  (key#5,3) (=21)," "\n"
        "  (key#9,3) (=22)," "\n"
        "  (key#5,4) (=23)," "\n"
        "  (key#9,4) (=24)," "\n"
        "  (key#10,3) (=25)," "\n"
        "  (key#10,4) (=26)}" "\n"
        "Q0: {" "\n"
        "  (key#6,3) (=16)," "\n"
        "  (key#6,4) (=17)}" "\n"
        "Qf: {" "\n"
        "  (key#5,4) (=23)," "\n"
        "  (key#9,4) (=24)," "\n"
        "  (key#10,4) (=26)}" "\n"
        "Sigma: {" "\n"
        "  <start;0;expression>," "\n"
        "  <start;1;body;0>," "\n"
        "  <start;2;expression;1>," "\n"
        "  <{#fun634}>," "\n"
        "  <start;2;expression>}" "\n"
        "Delta_c: {" "\n"
        "  ((key#4,3) (=19) ,<{#fun634}>, (key#3,3) (=20) )" "\n"
        "}" "\n"
        "Delta_i:  {" "\n"
        "  ((key#6,3) (=16) ,<start;0;expression>, (key#7,3) (=18) )," "\n"
        "  ((key#6,4) (=17) ,<start;0;expression>, (key#7,3) (=18) )," "\n"
        "  ((key#7,3) (=18) ,<start;2;expression;1>, (key#4,3) (=19) )," "\n"
        "  ((key#9,3) (=22) ,<start;2;expression>, (key#10,3) (=25) )," "\n"
        "  ((key#9,3) (=22) ,<start;2;expression>, (key#10,4) (=26) )," "\n"
        "  ((key#9,4) (=24) ,<start;2;expression>, (key#10,3) (=25) )," "\n"
        "  ((key#9,4) (=24) ,<start;2;expression>, (key#10,4) (=26) )" "\n"
        "}" "\n"
        "Delta_r: {" "\n"
        "  ((key#3,3) (=20) , (key#4,3) (=19) ,<start;1;body;0>, (key#5,3) (=21) )," "\n"
        "  ((key#3,3) (=20) , (key#4,3) (=19) ,<start;1;body;0>, (key#9,3) (=22) )," "\n"
        "  ((key#3,3) (=20) , (key#4,3) (=19) ,<start;1;body;0>, (key#5,4) (=23) )," "\n"
        "  ((key#3,3) (=20) , (key#4,3) (=19) ,<start;1;body;0>, (key#9,4) (=24) )" "\n"
        "}" "\n";

      std::stringstream ss1(mattsNwa);
      std::stringstream ss2("nwa: " + mattsNwa);
      std::stringstream ss3("nwa somename  { " + mattsNwa + " } nwa: " + mattsNwa);

      read_nwa(ss1);
      read_nwa(ss2);
      read_nwa(ss3);
      read_nwa(ss3);

      assert(ss1.peek() == -1);
      assert(ss2.peek() == -1);
      assert(ss3.peek() == -1);

    
      std::string duplicates =
        "Q: { a }" "\n"
        "Q: b" "\n"
        "Delta_i: (a, b (c), c)" "\n"
        "Delta_r: (a, b, c, d)" "\n"
        "Delta_c: {}" "\n"
        "Delta_i: (a, b, d)" "\n";

      std::stringstream ss4(duplicates);
      read_nwa(ss4);
      assert(ss4.peek() == -1);
    }
    
  } // namespace nwa
} // namespace wali

// Yo, Emacs!
// Local Variables:
//   c-file-style: "ellemtel"
//   c-basic-offset: 2
// End: