#ifndef __METALAN_PRIME_H
#define __METALAN_PRIME_H

#include <map>

#include "metalan.h"
#include "nanom.h"

#include "nanom_stringlib.h"
#include "nanom_symtablib.h"
#include "nanom_stdio.h"


namespace metalan {

struct fmter {
    std::string& str;
    fmter(std::string& s) : str(s) {}

    fmter& operator<<(const char* s) {
        str += s;
        return *this;
    }

    fmter& operator<<(const std::string& s) {
        str += s;
        return *this;
    }

    fmter& operator<<(nanom::Int v) {
        str += nanom::int_to_string(v);
        return *this;
    }

    fmter& operator<<(nanom::UInt v) {
        str += nanom::uint_to_string(v);
        return *this;
    }

    fmter& operator<<(nanom::Real v) {
        str += nanom::real_to_string(v);
        return *this;
    }

    fmter& operator<<(int v) {
        str += nanom::int_to_string((nanom::Int)v);
        return *this;
    }

    fmter& operator<<(unsigned int v) {
        str += nanom::uint_to_string((nanom::Int)v);
        return *this;
    }
};


struct MetalanPrime {

    static const size_t strtab_base = 0xFF0000;

    nanom::Vm vm;
    nanom::Assembler vm_as;

    Parser parser;

    MetalanPrime() : vm_as(vm) 
        {
            nanom::register_stringlib(vm_as, 0);
            nanom::register_symtablib(vm_as, 100);
            nanom::register_stdio(vm_as, 200);

            vm_as.register_const("port", (nanom::UInt)0xFF0000);
            vm_as.register_const("in", (nanom::UInt)0);
            vm_as.register_const("out", (nanom::UInt)1);
        }

    void parse(const std::string& code, const std::string& inp) {

        Outlist out;
        std::string unprocessed;
        bool ok = parser.parse(code, inp, out, unprocessed);

        if (!ok) {
            throw std::runtime_error("Parse failed. Unconsumed input: " + unprocessed);
        }

        std::string asmprog;

        std::map<size_t,std::string> result;

        fmter f(asmprog);

        for (const auto& n : out) {

            size_t inputid = strtab_base + result.size() * 2;
            size_t outputid = inputid + 1;
            std::string& respart = result[outputid];

            if (n.type == Outnode::CODE) {

                f << "\n.string '";

                for (unsigned char c : n.capture) {
                    if (c == '\'') asmprog += "\\'";
                    else asmprog += c;
                }

                f << "' " << inputid << "\n"
                  << "PUSH(" << inputid << ")\n"
                  << "PUSH($port)\n"
                  << "TO_HEAP($in)\n"
                  << "PUSH(" << outputid << ")\n"
                  << "PUSH($port)\n"
                  << "TO_HEAP($out)\n";

                asmprog += n.str;

            } else {
                respart = n.str;
            }
        }

        f << ".label __init_start_compile\n"
          << "PUSH($port)\n"
          << "SIZE_HEAP(2)\n"
          << "CALL(main)\n"
          << "EXIT\n";
        

        std::cout << asmprog << std::endl << std::endl;
        std::cout << "-----------------------------" << std::endl;

        vm_as.assemble(asmprog);
        vm_as.vm_run("__init_start_compile");

        for (auto& c : result) {

            auto i = vm.strtab.find(c.first);

            if (i != vm.strtab.end()) {
                std::cout << i->second;
            } else {
                std::cout << c.second;
            }
        }

        std::cout << std::endl;
    }
};

}

#endif
