# This script generates the opcode.h header file.

import sys
import tokenize

SCRIPT_NAME = "Tools/build/generate_opcode_h.py"
PYTHON_OPCODE = "Lib/opcode.py"

header = f"""
// Auto-generated by {SCRIPT_NAME} from {PYTHON_OPCODE}

#ifndef Py_OPCODE_H
#define Py_OPCODE_H
#ifdef __cplusplus
extern "C" {{
#endif


/* Instruction opcodes for compiled code */
""".lstrip()

footer = """

#define IS_PSEUDO_OPCODE(op) (((op) >= MIN_PSEUDO_OPCODE) && ((op) <= MAX_PSEUDO_OPCODE))

#ifdef __cplusplus
}
#endif
#endif /* !Py_OPCODE_H */
"""

internal_header = f"""
// Auto-generated by {SCRIPT_NAME} from {PYTHON_OPCODE}

#ifndef Py_INTERNAL_OPCODE_H
#define Py_INTERNAL_OPCODE_H
#ifdef __cplusplus
extern "C" {{
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "opcode.h"
""".lstrip()

internal_footer = """
#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_OPCODE_H
"""

DEFINE = "#define {:<38} {:>3}\n"

UINT32_MASK = (1<<32)-1

def write_int_array_from_ops(name, ops, out):
    bits = 0
    for op in ops:
        bits |= 1<<op
    out.write(f"const uint32_t {name}[9] = {{\n")
    for i in range(9):
        out.write(f"    {bits & UINT32_MASK}U,\n")
        bits >>= 32
    assert bits == 0
    out.write(f"}};\n")

def main(opcode_py, outfile='Include/opcode.h', internaloutfile='Include/internal/pycore_opcode.h'):
    opcode = {}
    if hasattr(tokenize, 'open'):
        fp = tokenize.open(opcode_py)   # Python 3.2+
    else:
        fp = open(opcode_py)            # Python 2.7
    with fp:
        code = fp.read()
    exec(code, opcode)
    opmap = opcode['opmap']
    opname = opcode['opname']
    hasarg = opcode['hasarg']
    hasconst = opcode['hasconst']
    hasjrel = opcode['hasjrel']
    hasjabs = opcode['hasjabs']
    is_pseudo = opcode['is_pseudo']
    _pseudo_ops = opcode['_pseudo_ops']

    ENABLE_SPECIALIZATION = opcode["ENABLE_SPECIALIZATION"]
    HAVE_ARGUMENT = opcode["HAVE_ARGUMENT"]
    MIN_PSEUDO_OPCODE = opcode["MIN_PSEUDO_OPCODE"]
    MAX_PSEUDO_OPCODE = opcode["MAX_PSEUDO_OPCODE"]

    NUM_OPCODES = len(opname)
    used = [ False ] * len(opname)
    next_op = 1

    for name, op in opmap.items():
        used[op] = True

    specialized_opmap = {}
    opname_including_specialized = opname.copy()
    for name in opcode['_specialized_instructions']:
        while used[next_op]:
            next_op += 1
        specialized_opmap[name] = next_op
        opname_including_specialized[next_op] = name
        used[next_op] = True
    specialized_opmap['DO_TRACING'] = 255
    opname_including_specialized[255] = 'DO_TRACING'
    used[255] = True

    with open(outfile, 'w') as fobj, open(internaloutfile, 'w') as iobj:
        fobj.write(header)
        iobj.write(internal_header)

        for name in opname:
            if name in opmap:
                op = opmap[name]
                if op == HAVE_ARGUMENT:
                    fobj.write(DEFINE.format("HAVE_ARGUMENT", HAVE_ARGUMENT))
                if op == MIN_PSEUDO_OPCODE:
                    fobj.write(DEFINE.format("MIN_PSEUDO_OPCODE", MIN_PSEUDO_OPCODE))

                fobj.write(DEFINE.format(name, op))

                if op == MAX_PSEUDO_OPCODE:
                    fobj.write(DEFINE.format("MAX_PSEUDO_OPCODE", MAX_PSEUDO_OPCODE))


        for name, op in specialized_opmap.items():
            fobj.write(DEFINE.format(name, op))

        iobj.write("\nextern const uint32_t _PyOpcode_Jump[9];\n")
        iobj.write("\nextern const uint8_t _PyOpcode_Caches[256];\n")
        iobj.write("\nextern const uint8_t _PyOpcode_Deopt[256];\n")
        iobj.write("\n#ifdef NEED_OPCODE_TABLES\n")
        write_int_array_from_ops("_PyOpcode_Jump", opcode['hasjrel'] + opcode['hasjabs'], iobj)

        iobj.write("\nconst uint8_t _PyOpcode_Caches[256] = {\n")
        for i, entries in enumerate(opcode["_inline_cache_entries"]):
            if entries:
                iobj.write(f"    [{opname[i]}] = {entries},\n")
        iobj.write("};\n")

        deoptcodes = {}
        for basic, op in opmap.items():
            if not is_pseudo(op):
                deoptcodes[basic] = basic
        for basic, family in opcode["_specializations"].items():
            for specialized in family:
                deoptcodes[specialized] = basic
        iobj.write("\nconst uint8_t _PyOpcode_Deopt[256] = {\n")
        for opt, deopt in sorted(deoptcodes.items()):
            iobj.write(f"    [{opt}] = {deopt},\n")
        iobj.write("};\n")
        iobj.write("#endif   // NEED_OPCODE_TABLES\n")

        fobj.write("\n")
        fobj.write("#define HAS_ARG(op) ((((op) >= HAVE_ARGUMENT) && (!IS_PSEUDO_OPCODE(op)))\\")
        for op in _pseudo_ops:
            if opmap[op] in hasarg:
                fobj.write(f"\n    || ((op) == {op}) \\")
        fobj.write("\n    )\n")

        fobj.write("\n")
        fobj.write("#define HAS_CONST(op) (false\\")
        for op in hasconst:
            fobj.write(f"\n    || ((op) == {opname[op]}) \\")
        fobj.write("\n    )\n")

        fobj.write("\n")
        for i, (op, _) in enumerate(opcode["_nb_ops"]):
            fobj.write(DEFINE.format(op, i))

        fobj.write("\n")
        fobj.write("/* Defined in Lib/opcode.py */\n")
        fobj.write(f"#define ENABLE_SPECIALIZATION {int(ENABLE_SPECIALIZATION)}")

        iobj.write("\n")
        iobj.write("#ifdef Py_DEBUG\n")
        iobj.write(f"static const char *const _PyOpcode_OpName[{NUM_OPCODES}] = {{\n")
        for op, name in enumerate(opname_including_specialized):
            if name[0] != "<":
                op = name
            iobj.write(f'''    [{op}] = "{name}",\n''')
        iobj.write("};\n")
        iobj.write("#endif\n")

        iobj.write("\n")
        iobj.write("#define EXTRA_CASES \\\n")
        for i, flag in enumerate(used):
            if not flag:
                iobj.write(f"    case {i}: \\\n")
        iobj.write("        ;\n")

        fobj.write(footer)
        iobj.write(internal_footer)


    print(f"{outfile} regenerated from {opcode_py}")


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3])
