#ifndef __NANOM_H
#define __NANOM_H

#include <ctype.h>

#include <cstdint>
#include <stdexcept>

#include <string>
#include <vector>
#include <unordered_map>

#include <functional>

#include "metalan.h"


namespace nanom {

using metalan::Sym;
using metalan::symtab;

typedef int64_t Int;
typedef uint64_t UInt;
typedef double Real;

union Val {
    Int inte;
    UInt uint;
    Real real;

    Val() : inte(0) {}
    Val(Int i)  { inte = i; }
    Val(UInt i) { uint = i; }
    Val(Real i) { real = i; }
};

enum Type {
    NONE   = 0,
    BOOL   = 1,
    SYMBOL = 2,
    INT    = 3, 
    UINT   = 4,
    REAL   = 5,
    STRUCT = 6
};


struct Shape {
    struct typeinfo {
        Type type;
        Sym shape;
        size_t ix_from;
        size_t ix_to;

        typeinfo() : type(NONE), shape(0), ix_from(1), ix_to(0) {}
    };

    std::unordered_map<Sym, typeinfo> sym2field;
    std::vector<typeinfo> n2field;

    std::vector<Type> serialized;

    size_t nfields;

    Shape() : nfields(0) {}

    const typeinfo& get_type(Sym s) const {
        auto i = sym2field.find(s);

        if (i == sym2field.end()) {
            static typeinfo notype;
            return notype;
        }

        return i->second;
    }

    const typeinfo& get_nth_type(size_t n) const {

        if (n >= n2field.size()) {
            static typeinfo notype;
            return notype;
        }

        return n2field[n];
    }

    std::pair<size_t,size_t> get_index(Sym s) const {
        auto i = sym2field.find(s);
        if (i == sym2field.end())
            return std::make_pair(1,0);
        return std::make_pair(i->second.ix_from, i->second.ix_to);
    }

    void add_field(Sym s, Type t, Sym shape=0, size_t sh_size=0) {
        if (sym2field.count(s) != 0) {
            throw std::runtime_error("Cannot add duplicate field to shape.");
        }

        typeinfo& ti = sym2field[s];

        ti.type = t;
        ti.shape = shape;
        ti.ix_from = nfields;

        if (t == STRUCT) {
            nfields += sh_size;

        } else {
            ++nfields;
        }

        ti.ix_to = nfields;

        n2field.push_back(ti);
    }

    const typeinfo& get_type(const std::string& s) const { 
        return get_type(symtab().get(s)); 
    }

    std::pair<size_t,size_t> get_index(const std::string& s) const { 
        return get_index(symtab().get(s)); 
    }

    size_t size() const { return nfields; }

    bool is_type(size_t n, Type t) const {
        if (n >= serialized.size() || serialized[n] != t) return false;
        return true;
    }

    bool is_bool(size_t n) const { return is_type(n, BOOL); }
    bool is_sym(size_t n) const { return is_type(n, SYMBOL); }
    bool is_int(size_t n) const { return is_type(n, INT); }
    bool is_uint(size_t n) const { return is_type(n, UINT); }
    bool is_real(size_t n) const { return is_type(n, REAL); }
};

struct Shapes {
    std::unordered_map<Sym,Shape> shapes;

    Shapes() {}

    Shapes(const Shapes& s) : shapes(s.shapes) {}

    Shapes(Shapes&& s) : shapes(s.shapes) {}

    const Shape& get(Sym shapeid) const {
        auto i = shapes.find(shapeid);

        if (i == shapes.end())
            throw std::runtime_error("Unknown shape name: " + symtab().get(shapeid));

        return i->second;
    }

    bool has_shape(Sym shapeid) const {
        return (shapes.count(shapeid) != 0);
    }

    void add(Sym shapeid, const Shape& sh) {

        auto i = shapes.find(shapeid);

        if (i != shapes.end())
            throw std::runtime_error("Cannot redefine shape: " + symtab().get(shapeid));

        serialize(const_cast<Shape&>(sh));

        shapes.insert(i, std::make_pair(shapeid, sh));
    }

    const Shape& get(const std::string& s) const {
        return get(symtab().get(s));
    }

    void reset() {
        shapes.clear();
    }

private:
    
    void serialize(Shape& s) {

        s.serialized.resize(s.size());

        for (const auto& i : s.sym2field) {

            if (i.second.ix_from + 1 == i.second.ix_to) {
                s.serialized[i.second.ix_from] = i.second.type;

            } else {
                Shape& subs = const_cast<Shape&>(get(i.second.shape));
                if (subs.serialized.size() != subs.size()) {
                    serialize(subs);
                }

                std::copy(subs.serialized.begin(), subs.serialized.end(), s.serialized.begin() + i.second.ix_from);
            }
        }
    }

};


struct Struct {
    typedef std::vector<Val> value_type;
    value_type v;

    Struct(size_t n = 0) {
        v.resize(n);
    }

    Struct(const Struct& s) : v(s.v) {}

    Struct(Struct&& s) {
        v.swap(s.v);
    }

    Struct& operator=(const Struct& s) {
        v = s.v;
        return *this;
    }

    Struct& operator=(Struct&& s) {
        v.swap(s.v);
        return *this;
    }

    const Val& get_field(size_t i) const {
        return v.at(i);
    }

    void set_field(size_t i, Val val) {
        v[i] = val;
    }

    Struct substruct(size_t b, size_t i) const {
        Struct ret;
        ret.v.assign(v.begin() + b, v.begin() + i);
        return ret;
    }
};


}


namespace std {

template <>
struct hash<nanom::Struct> {
    size_t operator()(const nanom::Struct& v) const {
        size_t h = 0;
        for (const auto& i : v.v) {
            h += hash<nanom::UInt>()(i.uint);
        }
        return hash<size_t>()(h);
    }
};

template <>
struct equal_to<nanom::Struct> {

    bool operator()(const nanom::Struct& a, const nanom::Struct& b) const {
        auto ai = a.v.begin();
        auto bi = b.v.begin();
        auto ae = a.v.end();
        auto be = b.v.end();

        while (1) {
            if (ai == ae && bi == be) return true;
            if (ai == ae || bi == be) return false;
            if (ai->uint != bi->uint) return false;
            ++ai;
            ++bi;
        }
    }
};

}


namespace nanom {

enum op_t {
    NOOP = 0,

    PUSH,
    POP,
    SWAP,

    IF_FAIL,
    IF_NOT_FAIL,
    POP_FRAMEHEAD,
    POP_FRAMETAIL,
    DROP_FRAME,
    FAIL,
    IF,
    IF_NOT,

    CALL,
    SYSCALL,
    TAILCALL,
    CALL_LIGHT,
    EXIT,

    NEW_SHAPE,
    DEF_FIELD,
    DEF_STRUCT_FIELD,
    DEF_SHAPE,

    NEW_STRUCT,
    SET_FIELDS,
    GET_FIELDS,
    GET_FRAMEHEAD_FIELDS,

    /***/

    ADD_INT,
    SUB_INT,
    MUL_INT,
    DIV_INT,
    MOD_INT,
    NEG_INT,

    ADD_UINT,
    SUB_UINT,
    MUL_UINT,
    MOD_UINT,
    DIV_UINT,

    ADD_REAL,
    SUB_REAL,
    MUL_REAL,
    DIV_REAL,
    NEG_REAL,

    BAND,
    BOR,
    BNOT,
    BXOR,
    BSHL,
    BSHR,

    BOOL_NOT,

    EQ_INT,
    LT_INT,
    LTE_INT,
    GT_INT,
    GTE_INT,

    EQ_UINT,
    LT_UINT,
    LTE_UINT,
    GT_UINT,
    GTE_UINT,

    EQ_REAL,
    LT_REAL,
    LTE_REAL,
    GT_REAL,
    GTE_REAL,

    INT_TO_REAL,
    REAL_TO_INT,
    UINT_TO_REAL,
    REAL_TO_UINT
};


struct Opcode {
    op_t op;
    Val arg;

    Opcode(op_t o = NOOP, Val a = (UInt)0) : op(o), arg(a) {}
};


struct label_t {
    Sym name;
    Sym fromshape;
    Sym toshape;
    label_t(Sym a=0, Sym b=0, Sym c=0) : name(a), fromshape(b), toshape(c) {}
    
    bool operator==(const label_t& l) const {
        return (name == l.name && fromshape == l.fromshape && toshape == l.toshape);
    }

    std::string print() const {
        return symtab().get(name) + " " + symtab().get(fromshape) + "->" + symtab().get(toshape);
    }
};

}


namespace std {
template <>
struct hash<nanom::label_t> {
    size_t operator()(const nanom::label_t& l) const {
        return (hash<nanom::Sym>()(l.name) ^ 
                hash<nanom::Sym>()(l.fromshape) ^
                hash<nanom::Sym>()(l.toshape));
    }
};
}


namespace nanom {



typedef std::function<bool(const Shapes&, const Shape&, const Shape&, const Struct&, Struct&)> callback_t;

struct Timings {
    std::unordered_map<label_t, size_t> timings;
    
    struct scope {
        struct timeval b;
        size_t& counter;

        scope(Timings& t, const label_t& l) : counter(t.timings[l]) {
            gettimeofday(&b, NULL);
        }

        ~scope() {
            struct timeval e;
            gettimeofday(&e, NULL);
            size_t a = (e.tv_sec*1e6 + e.tv_usec);
            size_t q = (b.tv_sec*1e6 + b.tv_usec);
            counter += (a-q);
        }
    };
};



struct VmCode {
    typedef std::vector<Opcode> code_t;
     
    Timings timings;
    std::unordered_map<label_t, code_t> codes;

    Shapes shapes;

    std::unordered_map<label_t, callback_t> callbacks;

    VmCode() {}

    VmCode(const VmCode& vc) : codes(vc.codes), shapes(vc.shapes), callbacks(vc.callbacks) {}

    VmCode(VmCode&& vc) : codes(vc.codes), shapes(vc.shapes), callbacks(vc.callbacks) {}

    void register_callback(label_t s, callback_t cb) {

        if (callbacks.find(s) != callbacks.end()) {
            throw std::runtime_error("Callback registered twice: " + s.print());
        }

        callbacks[s] = cb;
    }

    static label_t toplevel_label() {
        Sym none = symtab().get("");
        return label_t(none, none, none);
    }
};



struct Vm {

    struct frame_t {
        label_t prev_label;
        size_t prev_ip;
        size_t stack_ix;
        size_t struct_size;

        frame_t() : prev_ip(0), stack_ix(0), struct_size(0) {}
        frame_t(const label_t& l, size_t i, size_t s, size_t ss) : 
            prev_label(l), prev_ip(i), stack_ix(s), struct_size(ss) {}
    };

    std::vector<Val> stack;
    std::vector<frame_t> frame;
    bool failbit;

    VmCode& code;
    Shapes& shapes;

    Shape tmp_shape;


    Vm(VmCode& c) : failbit(false), code(c), shapes(code.shapes) {

        stack.reserve(2048);
        frame.reserve(256);
    }

    Vm(VmCode& c, Shapes& s) : failbit(false), code(c), shapes(s) {}

    Val pop() {
        Val ret = stack.back();
        stack.pop_back();
        return ret;
    }

    void push(Val v) {
        stack.push_back(v);
    }

    void reset() {
        stack.clear();
        frame.clear();
        failbit = false;
    }

};

namespace {


struct _mapper {
    std::unordered_map<size_t,std::string> m;
    std::unordered_map<std::string,op_t> n;

    _mapper() {
        m[(size_t)NOOP] = "NOOP";
        m[(size_t)PUSH] = "PUSH";
        m[(size_t)POP] = "POP";
        m[(size_t)SWAP] = "SWAP";
        m[(size_t)IF] = "IF";
        m[(size_t)IF_NOT] = "IF_NOT";
        m[(size_t)IF_FAIL] = "IF_FAIL";
        m[(size_t)IF_NOT_FAIL] = "IF_NOT_FAIL";
        m[(size_t)POP_FRAMEHEAD] = "POP_FRAMEHEAD";
        m[(size_t)POP_FRAMETAIL] = "POP_FRAMETAIL";
        m[(size_t)DROP_FRAME] = "DROP_FRAME";
        m[(size_t)FAIL] = "FAIL";
        m[(size_t)CALL] = "CALL";
        m[(size_t)CALL_LIGHT] = "CALL_LIGHT";
        m[(size_t)SYSCALL] = "SYSCALL";
        m[(size_t)TAILCALL] = "TAILCALL";
        m[(size_t)EXIT] = "EXIT";
        m[(size_t)NEW_SHAPE] = "NEW_SHAPE";
        m[(size_t)DEF_FIELD] = "DEF_FIELD";
        m[(size_t)DEF_STRUCT_FIELD] = "DEF_STRUCT_FIELD";
        m[(size_t)DEF_SHAPE] = "DEF_SHAPE";
        m[(size_t)NEW_STRUCT] = "NEW_STRUCT";
        m[(size_t)SET_FIELDS] = "SET_FIELDS";
        m[(size_t)GET_FIELDS] = "GET_FIELDS";
        m[(size_t)GET_FRAMEHEAD_FIELDS] = "GET_FRAMEHEAD_FIELDS";
        m[(size_t)ADD_INT] = "ADD_INT";
        m[(size_t)SUB_INT] = "SUB_INT";
        m[(size_t)MUL_INT] = "MUL_INT";
        m[(size_t)DIV_INT] = "DIV_INT";
        m[(size_t)MOD_INT] = "MOD_INT";
        m[(size_t)NEG_INT] = "NEG_INT";
        m[(size_t)ADD_UINT] = "ADD_UINT";
        m[(size_t)SUB_UINT] = "SUB_UINT";
        m[(size_t)MUL_UINT] = "MUL_UINT";
        m[(size_t)DIV_UINT] = "DIV_UINT";
        m[(size_t)MOD_UINT] = "MOD_UINT";
        m[(size_t)ADD_REAL] = "ADD_REAL";
        m[(size_t)SUB_REAL] = "SUB_REAL";
        m[(size_t)MUL_REAL] = "MUL_REAL";
        m[(size_t)DIV_REAL] = "DIV_REAL";
        m[(size_t)NEG_REAL] = "NEG_REAL";
        m[(size_t)BAND] = "BAND";
        m[(size_t)BOR] = "BOR";
        m[(size_t)BNOT] = "BNOT";
        m[(size_t)BXOR] = "BXOR";
        m[(size_t)BSHL] = "BSHL";
        m[(size_t)BSHR] = "BSHR";
        m[(size_t)BOOL_NOT] = "BOOL_NOT";
        m[(size_t)EQ_INT] = "EQ_INT";
        m[(size_t)LT_INT] = "LT_INT";
        m[(size_t)LTE_INT] = "LTE_INT";
        m[(size_t)GT_INT] = "GT_INT";
        m[(size_t)GTE_INT] = "GTE_INT";
        m[(size_t)EQ_UINT] = "EQ_UINT";
        m[(size_t)LT_UINT] = "LT_UINT";
        m[(size_t)LTE_UINT] = "LTE_UINT";
        m[(size_t)GT_UINT] = "GT_UINT";
        m[(size_t)GTE_UINT] = "GTE_UINT";
        m[(size_t)EQ_REAL] = "EQ_REAL";
        m[(size_t)LT_REAL] = "LT_REAL";
        m[(size_t)LTE_REAL] = "LTE_REAL";
        m[(size_t)GT_REAL] = "GT_REAL";
        m[(size_t)GTE_REAL] = "GTE_REAL";
        m[(size_t)INT_TO_REAL] = "INT_TO_REAL";
        m[(size_t)REAL_TO_INT] = "REAL_TO_INT";
        m[(size_t)UINT_TO_REAL] = "UINT_TO_REAL";
        m[(size_t)REAL_TO_UINT] = "REAL_TO_UINT";
        
        n["NOOP"] = NOOP;
        n["PUSH"] = PUSH;
        n["POP"] = POP;
        n["SWAP"] = SWAP;
        n["IF"] = IF;
        n["IF_NOT"] = IF_NOT;
        n["IF_FAIL"] = IF_FAIL;
        n["IF_NOT_FAIL"] = IF_NOT_FAIL;
        n["POP_FRAMEHEAD"] = POP_FRAMEHEAD;
        n["POP_FRAMETAIL"] = POP_FRAMETAIL;
        n["DROP_FRAME"] = DROP_FRAME;
        n["FAIL"] = FAIL;
        n["CALL"] = CALL;
        n["CALL_LIGHT"] = CALL_LIGHT;
        n["SYSCALL"] = SYSCALL;
        n["TAILCALL"] = TAILCALL;
        n["EXIT"] = EXIT;
        n["NEW_SHAPE"] = NEW_SHAPE;
        n["DEF_FIELD"] = DEF_FIELD;
        n["DEF_STRUCT_FIELD"] = DEF_STRUCT_FIELD;
        n["DEF_SHAPE"] = DEF_SHAPE;
        n["NEW_STRUCT"] = NEW_STRUCT;
        n["SET_FIELDS"] = SET_FIELDS;
        n["GET_FIELDS"] = GET_FIELDS;
        n["GET_FRAMEHEAD_FIELDS"] = GET_FRAMEHEAD_FIELDS;
        n["ADD_INT"] = ADD_INT;
        n["SUB_INT"] = SUB_INT;
        n["MUL_INT"] = MUL_INT;
        n["DIV_INT"] = DIV_INT;
        n["NEG_INT"] = NEG_INT;
        n["ADD_UINT"] = ADD_UINT;
        n["SUB_UINT"] = SUB_UINT;
        n["MUL_UINT"] = MUL_UINT;
        n["DIV_UINT"] = DIV_UINT;
        n["ADD_REAL"] = ADD_REAL;
        n["SUB_REAL"] = SUB_REAL;
        n["MUL_REAL"] = MUL_REAL;
        n["DIV_REAL"] = DIV_REAL;
        n["NEG_REAL"] = NEG_REAL;
        n["BAND"] = BAND;
        n["BOR"] = BOR;
        n["BNOT"] = BNOT;
        n["BXOR"] = BXOR;
        n["BSHL"] = BSHL;
        n["BSHR"] = BSHR;
        n["BOOL_NOT"] = BOOL_NOT;
        n["EQ_INT"] = EQ_INT;
        n["LT_INT"] = LT_INT;
        n["LTE_INT"] = LTE_INT;
        n["GT_INT"] = GT_INT;
        n["GTE_INT"] = GTE_INT;
        n["EQ_UINT"] = EQ_UINT;
        n["LT_UINT"] = LT_UINT;
        n["LTE_UINT"] = LTE_UINT;
        n["GT_UINT"] = GT_UINT;
        n["GTE_UINT"] = GTE_UINT;
        n["EQ_REAL"] = EQ_REAL;
        n["LT_REAL"] = LT_REAL;
        n["LTE_REAL"] = LTE_REAL;
        n["GT_REAL"] = GT_REAL;
        n["GTE_REAL"] = GTE_REAL;
        n["INT_TO_REAL"] = INT_TO_REAL;
        n["REAL_TO_INT"] = REAL_TO_INT;
        n["UINT_TO_REAL"] = UINT_TO_REAL;
        n["REAL_TO_UINT"] = REAL_TO_UINT;
    }

    const std::string& name(op_t opc) const {
        auto i = m.find(opc);

        if (i == m.end()) {
            throw std::runtime_error("Invalid opcode requested");
        }

        return i->second;
    }

    const op_t code(const std::string& opc) const {
        auto i = n.find(opc);

        if (i == n.end()) {
            throw std::runtime_error("Invalid opcode requested: " + opc);
        }

        return i->second;
    }
};


const std::string& opcodename(op_t opc) {
    static const _mapper m;
    return m.name(opc);
}


const op_t opcodecode(const std::string& opc) {
    static const _mapper m;
    return m.code(opc);
}

}


inline void vm_run(Vm& vm, 
                   label_t label = VmCode::toplevel_label(), 
                   size_t ip = 0, 
                   bool verbose = false) {

    size_t topframe = vm.frame.size();

    VmCode::code_t* code = &(vm.code.codes[label]);
    
    if (verbose) {
        std::cout << ">>> " << label.print() << " " << ip << std::endl;
    }

    while (1) {

        if (ip >= code->size()) {
            throw std::runtime_error("Sanity error: instruction pointer out of bounds.");
        }

        Opcode& c = (*code)[ip];

        if (verbose) {

            std::string pref;
            for (size_t n = 0; n < vm.frame.size(); ++n) {
                pref += "  ";
            }
            switch (c.op) {
            case CALL:
                std::cout << pref << "CALL " << symtab().get((vm.stack.rbegin()+2)->uint) << " "
                          << symtab().get((vm.stack.rbegin()+1)->uint) << " "
                          << symtab().get(vm.stack.rbegin()->uint) << std::endl;
                break;
            case TAILCALL:
                std::cout << pref << "CALL " << symtab().get((vm.stack.rbegin()+2)->uint) << " "
                          << symtab().get((vm.stack.rbegin()+1)->uint) << " "
                          << symtab().get(vm.stack.rbegin()->uint) << std::endl;
                break;
            case SYSCALL:
                break;
            case CALL_LIGHT:
                std::cout << pref << "CALL_LIGHT " << symtab().get(vm.stack.rbegin()->uint) << std::endl;
                break;
            case EXIT:
                std::cout << pref << "EXIT" << std::endl;
                break;
            case FAIL:
                std::cout << pref << "FAIL" << std::endl;
                break;
            default:
                break;
            }                

            /*
            std::cout << ">" << ip << " " << opcodename(c.op) << "(" << c.arg.inte << ") "
                      << vm.failbit << " ||\t\t\t";
            for (const auto& ii : vm.stack) {
                std::cout << " " << ii.inte << ":" << symtab().get(ii.uint);
            }
            std::cout << " [" << vm.stack.size() << "]  " << topframe;
            for (const auto& ii : vm.frame) {
                std::cout << "\n\t\t" << ii.prev_ip << "/" << ii.stack_ix << "," << ii.struct_size 
                          << "," << symtab().get(ii.prev_label.name)
                          << "," << symtab().get(ii.prev_label.fromshape)
                          << "," << symtab().get(ii.prev_label.toshape) << " ";
            }
            std::cout << std::endl;
            */
        }

        switch (c.op) {
        case NOOP:
            break;
            
        case PUSH:
            vm.stack.push_back(c.arg);
            break;

        case POP:
            vm.stack.pop_back();
            break;

        case SWAP: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v2);
            vm.stack.push_back(v1);
            break;
        }

        case IF: {
            Val v = vm.pop();
            if (v.uint) {
                ip += c.arg.inte;
                continue;
            }
            break;
        }

        case IF_NOT: {
            Val v = vm.pop();
            if (!v.uint) {
                ip += c.arg.inte;
                continue;
            }
            break;
        }

        case IF_FAIL: 
            if (vm.failbit) {
                ip += c.arg.inte;
                continue;
            }
            break;

        case IF_NOT_FAIL:
            if (!vm.failbit) {
                ip += c.arg.inte;
                continue;
            }
            break;

        case POP_FRAMEHEAD: {
            const auto& fp = vm.frame.back();
            auto sb = vm.stack.begin() + fp.stack_ix;
            auto se = sb + fp.struct_size;
            vm.stack.erase(sb, se);
            break;
        }

        case POP_FRAMETAIL: {
            const auto& fp = vm.frame.back();
            auto sb = vm.stack.begin() + fp.stack_ix + fp.struct_size;
            vm.stack.erase(sb, vm.stack.end());
            break;
        }

        case DROP_FRAME:
            vm.frame.pop_back();
            break;
            
        case FAIL: {
            vm.failbit = true;

            if (vm.frame.size() == topframe) {
                vm.frame.pop_back();
                return;

            } else {
                const auto& fp = vm.frame.back();
                label = fp.prev_label;
                ip = fp.prev_ip;
                code = &(vm.code.codes[label]);
                vm.frame.pop_back();
                continue;
            }
        }

        case EXIT: {
            vm.failbit = false;

            if (vm.frame.size() == topframe) {
                vm.frame.pop_back();
                return;

            } else {
                const auto& fp = vm.frame.back();
                label = fp.prev_label;
                ip = fp.prev_ip;
                code = &(vm.code.codes[label]);
                vm.frame.pop_back();
                continue;
            }
        }

        case TAILCALL: {
            Val totype = vm.pop();
            Val fromtype = vm.pop();
            Val name = vm.pop();

            auto& fp = vm.frame.back();
            auto sb = vm.stack.begin() + fp.stack_ix;
            auto se = sb + fp.struct_size;
            vm.stack.erase(sb, se);

            const Shape& shape = vm.shapes.get(fromtype.uint);
            
            label_t l(name.uint, fromtype.uint, totype.uint);

            fp.stack_ix = vm.stack.size() - shape.size();
            fp.struct_size = shape.size();

            vm.failbit = false;
            label = l;
            code = &(vm.code.codes[label]);
            ip = 0;
            continue;
        }

        case CALL: {
            Val totype = vm.pop();
            Val fromtype = vm.pop();
            Val name = vm.pop();

            const Shape& shape = vm.shapes.get(fromtype.uint);
            
            label_t l(name.uint, fromtype.uint, totype.uint);

            vm.frame.emplace_back(label, ip+1, vm.stack.size() - shape.size(), shape.size());

            vm.failbit = false;
            label = l;
            code = &(vm.code.codes[label]);
            ip = 0;
            continue;
        }

        case SYSCALL: {
            Val totype = vm.pop();
            Val fromtype = vm.pop();
            Val name = vm.pop();

            const Shape& shape = vm.shapes.get(fromtype.uint);

            label_t l(name.uint, fromtype.uint, totype.uint);

            Struct tmp;
            auto tope = vm.stack.end();
            auto topb = tope - shape.size();
            tmp.v.assign(topb, tope);
            vm.stack.resize(vm.stack.size() - shape.size());

            Struct ret;

            auto j = vm.code.callbacks.find(l);

            if (j == vm.code.callbacks.end()) {
                throw std::runtime_error("Callback '" + l.print() + "' undefined");
            }

            Timings::scope(vm.code.timings, l);
            vm.failbit = !(j->second)(vm.shapes, shape, vm.shapes.get(totype.uint), tmp, ret);

            vm.stack.insert(vm.stack.end(), ret.v.begin(), ret.v.end());
            break;
        }

        case CALL_LIGHT: {
            Val name = vm.pop();

            const auto& fr = vm.frame.back();

            vm.frame.emplace_back(label, ip+1, fr.stack_ix, fr.struct_size);

            vm.failbit = false;
            label.name = name.uint;
            code = &(vm.code.codes[label]);
            ip = 0;
            continue;
        }

        case NEW_SHAPE: {
            vm.tmp_shape = Shape();
            break;
        }

        case DEF_FIELD: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.tmp_shape.add_field(v1.uint, (Type)v2.uint);
            break;
        }

        case DEF_STRUCT_FIELD: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            const Shape& sh = vm.shapes.get(v2.uint);
            vm.tmp_shape.add_field(v1.uint, STRUCT, v2.uint, sh.size());
            break;
        }

        case DEF_SHAPE: {
            Val v = vm.pop();
            vm.shapes.add(v.uint, vm.tmp_shape);
            break;
        }

        case NEW_STRUCT: {
            Val v = vm.pop();
            vm.stack.insert(vm.stack.end(), v.uint, Val());
            break;
        }

        case SET_FIELDS: {
            Val strusize = vm.pop();
            Val offs_end = vm.pop();
            Val offs_beg = vm.pop();

            size_t topsize = (offs_end.uint - offs_beg.uint);
            auto tope = vm.stack.end();
            auto topi = tope - topsize;
            auto stri = topi - strusize.uint + offs_beg.uint;

            for (auto i = topi; i != tope; ++i, ++stri) {
                *stri = *i;
            }

            vm.stack.resize(vm.stack.size() - topsize);
            break;
        } 

        case GET_FIELDS: {
            Val strusize = vm.pop();
            Val offs_end = vm.pop();
            Val offs_beg = vm.pop();

            vm.stack.reserve(vm.stack.size() + (offs_end.uint - offs_beg.uint));

            auto stbeg = vm.stack.end();
            stbeg = stbeg - strusize.uint;
            auto fb = stbeg + offs_beg.uint;
            auto fe = stbeg + offs_end.uint;

            for (; fb != fe; ++fb) {
                vm.stack.push_back(*fb);
            }
            
            vm.stack.erase(stbeg, stbeg + strusize.uint);
            break;
        }

        case GET_FRAMEHEAD_FIELDS: {
            Val offs_end = vm.pop();
            Val offs_beg = vm.pop();

            vm.stack.reserve(vm.stack.size() + (offs_end.uint - offs_beg.uint));

            const auto& fp = vm.frame.back();
            auto sb = vm.stack.begin() + fp.stack_ix;
            auto se = sb + offs_end.uint;
            sb += offs_beg.uint;

            vm.stack.insert(vm.stack.end(), sb, se);
            break;
        }
            
        case ADD_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.inte + v2.inte);
            break;
        }

        case SUB_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.inte - v2.inte);
            break;
        }

        case MUL_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.inte * v2.inte);
            break;
        }

        case DIV_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.inte / v2.inte);
            break;
        }

        case MOD_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.inte % v2.inte);
            break;
        }

        case NEG_INT: {
            Val v = vm.pop();
            vm.stack.push_back(-v.inte);
            break;
        }

        case ADD_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint + v2.uint);
            break;
        }

        case SUB_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint - v2.uint);
            break;
        }

        case MUL_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint * v2.uint);
            break;
        }

        case DIV_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint / v2.uint);
            break;
        }

        case MOD_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint % v2.uint);
            break;
        }

        case ADD_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.real + v2.real);
            break;
        }

        case SUB_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.real - v2.real);
            break;
        }

        case MUL_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.real * v2.real);
            break;
        }

        case DIV_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.real / v2.real);
            break;
        }

        case NEG_REAL: {
            Val v = vm.pop();
            vm.stack.push_back(-v.real);
            break;
        }

        case BAND: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint & v2.uint);
            break;
        }

        case BOR: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint | v2.uint);
            break;
        }

        case BNOT: {
            Val v1 = vm.pop();
            vm.stack.push_back(~v1.uint);
            break;
        }

        case BXOR: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint ^ v2.uint);
            break;
        }

        case BSHL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint << v2.uint);
            break;
        }

        case BSHR: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(v1.uint >> v2.uint);
            break;
        }

        case BOOL_NOT: {
            Val v = vm.pop();
            vm.stack.push_back((UInt)!(v.uint));
            break;
        }

        case EQ_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.inte == v2.inte));
            break;
        }

        case LT_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.inte < v2.inte));
            break;
        }

        case LTE_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.inte <= v2.inte));
            break;
        }

        case GT_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.inte > v2.inte));
            break;
        }

        case GTE_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.inte >= v2.inte));
            break;
        }

        case EQ_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.uint == v2.uint));
            break;
        }

        case LT_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.uint < v2.uint));
            break;
        }

        case LTE_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.uint <= v2.uint));
            break;
        }

        case GT_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.uint > v2.uint));
            break;
        }

        case GTE_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.uint >= v2.uint));
            break;
        }

        case EQ_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.real == v2.real));
            break;
        }

        case LT_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.real < v2.real));
            break;
        }

        case LTE_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.real <= v2.real));
            break;
        }

        case GT_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.real > v2.real));
            break;
        }

        case GTE_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back((Int)(v1.real >= v2.real));
            break;
        }

        case INT_TO_REAL: {
            Val v = vm.pop();
            Val ret = (Real)v.inte;
            vm.stack.push_back(ret);
            break;
        }

        case REAL_TO_INT: {
            Val v = vm.pop();
            Val ret = (Int)v.real;
            vm.stack.push_back(ret);
            break;
        }

        case UINT_TO_REAL: {
            Val v = vm.pop();
            Val ret = (Real)v.uint;
            vm.stack.push_back(ret);
            break;
        }

        case REAL_TO_UINT: {
            Val v = vm.pop();
            Val ret = (UInt)v.real;
            vm.stack.push_back(ret);
            break;
        }

        }

        ++ip;
    }
}

}


#endif


