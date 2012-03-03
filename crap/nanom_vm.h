#ifndef __NANOM_VM_H
#define __NANOM_VM_H

#include <ctype.h>

#include <cstdint>
#include <stdexcept>

#include <vector>
#include <unordered_map>

namespace nanom {

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

enum op_t {
    NOOP = 0,

    PUSH,

    TO_HEAP,
    COPY_TO_HEAP,
    FROM_HEAP,
    SIZE_HEAP,

    POP,
    SWAP,
    DUP,

    CMP_INT,
    CMP_UINT,
    CMP_REAL,
    
    CALL,
    CALL_IF,
    RET,
    JUMP,
    JUMP_IF,
    JUMP_CHECK_IF,
    RET_IF,

    SYSCALL,
    EXIT,

    INT_TO_REAL,
    REAL_TO_INT,

    ADD_INT,
    SUB_INT,
    MUL_INT,
    DIV_INT,
    NEG_INT,

    ADD_UINT,
    SUB_UINT,
    MUL_UINT,
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
    BSHR
};


struct Opcode {
    op_t op;
    Val arg;
};

struct Vm;

typedef void (*syscall_callback_t)(Vm&);

struct Vm {

    std::vector<Val> stack;
    std::vector<size_t> frame;
    std::unordered_map<size_t, std::vector<Val> > heap;

    std::unordered_map<size_t, std::string> strtab;

    std::unordered_map<size_t, syscall_callback_t> syscalls;

    std::vector<Opcode> code;


    Vm() {}

    void register_syscall(size_t n, syscall_callback_t c) {
        syscalls[n] = c;
    }

    void syscall(size_t n) {
        auto i = syscalls.find(n);

        if (i == syscalls.end())
            throw std::runtime_error("Unknown syscall index.");

        i->second(*this);
    }

    Val pop() {
        Val ret = stack.back();
        stack.pop_back();
        return ret;
    }

    void push(Val v) {
        stack.push_back(v);
    }

};

namespace {

template <typename T>
Int _cmp(T a, T b) {
    if (a < b) return -1;
    else if (a > b) return 1;
    return 0;
}

template <typename T>
T _add(T a, T b) {
    return (a + b);
}

template <typename T>
T _sub(T a, T b) {
    return (a - b);
}

template <typename T>
T _mul(T a, T b) {
    return (a * b);
}

template <typename T>
T _div(T a, T b) {
    return (a / b);
}

struct _mapper {
    std::unordered_map<int,std::string> m;
    _mapper() {
        m[NOOP] = "NOOP";
        m[PUSH] = "PUSH";
        m[TO_HEAP] = "TO_HEAP";
        m[COPY_TO_HEAP] = "COPY_TO_HEAP";
        m[FROM_HEAP] = "FROM_HEAP";
        m[SIZE_HEAP] = "SIZE_HEAP";
        m[POP] = "POP";
        m[SWAP] = "SWAP";
        m[DUP] = "DUP";
        m[CMP_INT] = "CMP_INT";
        m[CMP_UINT] = "CMP_UINT";
        m[CMP_REAL] = "CMP_REAL";
        m[CALL] = "CALL";
        m[CALL_IF] = "CALL_IF";
        m[RET] = "RET";
        m[JUMP] = "JUMP";
        m[JUMP_IF] = "JUMP_IF";
        m[JUMP_CHECK_IF] = "JUMP_CHECK_IF";
        m[RET_IF] = "RET_IF";
        m[SYSCALL] = "SYSCALL";
        m[EXIT] = "EXIT";
        m[INT_TO_REAL] = "INT_TO_REAL";
        m[REAL_TO_INT] = "REAL_TO_INT";
        m[ADD_INT] = "ADD_INT";
        m[SUB_INT] = "SUB_INT";
        m[MUL_INT] = "MUL_INT";
        m[DIV_INT] = "DIV_INT";
        m[NEG_INT] = "NEG_INT";
        m[ADD_UINT] = "ADD_UINT";
        m[SUB_UINT] = "SUB_UINT";
        m[MUL_UINT] = "MUL_UINT";
        m[DIV_UINT] = "DIV_UINT";
        m[ADD_REAL] = "ADD_REAL";
        m[SUB_REAL] = "SUB_REAL";
        m[MUL_REAL] = "MUL_REAL";
        m[DIV_REAL] = "DIV_REAL";
        m[NEG_REAL] = "NEG_REAL";
        m[BAND] = "BAND";
        m[BOR] = "BOR";
        m[BNOT] = "BNOT";
        m[BXOR] = "BXOR";
        m[BSHL] = "BSHL";
        m[BSHR] = "BSHR";
    }
};

const std::string& opcodename(int opc) {
    static _mapper m;
    return m.m[opc];
}

}


inline void vm_run(Vm& vm, size_t ip) {

    bool done = false;

    while (!done) {

        if (ip >= vm.code.size()) {
            throw std::runtime_error("Sanity error: instruction pointer out of bounds.");
        }

        Opcode& c = vm.code[ip];


        std::cout << "/" << ip << " " << opcodename(c.op) << "(" << c.arg.inte << ") "
                  << " ||\t\t\t";
        for (const auto& ii : vm.stack) {
            std::cout << " " << ii.inte;
        }
        std::cout << std::endl;


        switch (c.op) {
        case NOOP:
            break;
            
        case PUSH:
            vm.stack.push_back(c.arg);
            break;

        case SIZE_HEAP: {
            Val cell = vm.pop();

            if (c.arg.uint == 0) {
                vm.heap.erase(cell.uint);
            } else {
                std::vector<Val>& h = vm.heap[cell.uint];

                if (h.size() < c.arg.uint) {
                    h.resize(c.arg.uint);
                }
            }
            break;
        }

        case TO_HEAP: {
            Val cell = vm.pop();
            Val v = vm.pop();

            vm.heap[cell.uint][c.arg.uint] = v;
            break;
        }

        case COPY_TO_HEAP: {
            Val cell = vm.pop();

            vm.heap[cell.uint][c.arg.uint] = vm.stack.back();
            break;
        }

        case FROM_HEAP: {

            Val cell = vm.pop();

            vm.stack.push_back(vm.heap[cell.uint][c.arg.uint]);
            break;
        }

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

        case DUP: {
            Val v = vm.pop();
            vm.stack.push_back(v);
            vm.stack.push_back(v);
            break;
        }

        case CMP_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_cmp(v1.inte, v2.inte));
            break;
        }

        case CMP_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_cmp(v1.uint, v2.uint));
            break;
        }

        case CMP_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_cmp(v1.real, v2.real));
            break;
        }

        case CALL:
            vm.frame.push_back((UInt)ip + 1);
            ip = c.arg.uint;
            continue;

        case CALL_IF: {
            Val v = vm.pop();
            if (v.inte != 0) {
                vm.frame.push_back((UInt)ip + 1);
                ip = c.arg.uint;
                continue;
            }
            break;
        }

        case JUMP:
            ip = c.arg.uint;
            continue;

        case JUMP_IF: {
            Val v = vm.pop();
            if (v.inte != 0) {
                ip = c.arg.uint;
                continue;
            }
            break;
        }

        case JUMP_CHECK_IF: {
            Val v = vm.stack.back();
            if (v.inte != 0) {
                ip = c.arg.uint;
                continue;
            }
            break;
        }
         
        case RET: {
            size_t nip = vm.frame.back();
            vm.frame.pop_back();
            ip = nip;
            continue;
        }

        case RET_IF: {
            Val v = vm.pop();
            if (v.inte != 0) {
                size_t nip = vm.frame.back();
                vm.frame.pop_back();
                ip = nip;
                continue;
            }
            break;
        }

        case SYSCALL: {
            vm.syscall(c.arg.uint);
            break;
        }

        case EXIT:
            done = true;
            break;

        case INT_TO_REAL:
            vm.stack.back().real = (Real)vm.stack.back().inte;
            break;

        case REAL_TO_INT:
            vm.stack.back().inte = (Int)vm.stack.back().real;
            break;

        case ADD_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_add(v1.inte, v2.inte));
            break;
        }

        case SUB_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_sub(v1.inte, v2.inte));
            break;
        }

        case MUL_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_mul(v1.inte, v2.inte));
            break;
        }

        case DIV_INT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_div(v1.inte, v2.inte));
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
            vm.stack.push_back(_add(v1.uint, v2.uint));
            break;
        }

        case SUB_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_sub(v1.uint, v2.uint));
            break;
        }

        case MUL_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_mul(v1.uint, v2.uint));
            break;
        }

        case DIV_UINT: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_div(v1.uint, v2.uint));
            break;
        }

        case ADD_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_add(v1.real, v2.real));
            break;
        }

        case SUB_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_sub(v1.real, v2.real));
            break;
        }

        case MUL_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_mul(v1.real, v2.real));
            break;
        }

        case DIV_REAL: {
            Val v2 = vm.pop();
            Val v1 = vm.pop();
            vm.stack.push_back(_div(v1.real, v2.real));
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

        }

        ++ip;
    }
}

}

#endif

