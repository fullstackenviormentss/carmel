#ifndef FILEARGS_HPP
#define FILEARGS_HPP

#include <string>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>

typedef boost::shared_ptr<std::istream> Infile;
typedef boost::shared_ptr<std::ostream> Outfile;

struct null_deleter {
    void operator()(void*) {}
};

static Infile default_in(&cin,null_deleter());
static Outfile default_log(&cerr,null_deleter());
static Outfile default_out(&cout,null_deleter());
static Infile default_none;

//using namespace boost;
//using namespace boost::program_options;
namespace po=boost::program_options;
using boost::shared_ptr;

//using namespace std;


/* Overload the 'validate' function for shared_ptr<std::istream>. We use shared ptr
   to properly kill the stream when it's no longer used.
*/
namespace boost {    namespace program_options {
void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::istream>* target_type, int)
{
    using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = validators::get_single_string(values);

    if (s == "-") {
        boost::shared_ptr<std::istream> r(&std::cin, null_deleter());
        v = boost::any(r);
    } else {
        boost::shared_ptr<ifstream> r(new ifstream(s.c_str()));
        if (!*r) {
            throw std::logic_error(std::string("Could not open input file ").append(s));
        }
        boost::shared_ptr<std::istream> r2(r);
        v = boost::any(r2);
    }
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              boost::shared_ptr<std::ostream>* target_type, int)
{
    using namespace boost::program_options;

    // Make sure no previous assignment to 'a' was made.
    validators::check_first_occurrence(v);
    // Extract the first std::string from 'values'. If there is more than
    // one std::string, it's an error, and exception will be thrown.
    const std::string& s = validators::get_single_string(values);

    if (s == "-") {
        boost::shared_ptr<std::ostream> w(&std::cout, null_deleter());
        v = boost::any(w);
    } else {
        boost::shared_ptr<ofstream> r(new ofstream(s.c_str()));
        if (!*r) {
            throw std::logic_error(std::string("Could not create output file ").append(s));
        }
        boost::shared_ptr<std::ostream> w2(r);
        v = boost::any(w2);
    }
}

}}


#endif
