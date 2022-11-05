"""Generate the main interpreter switch."""

# Write the cases to generated_cases.c.h, which is #included in ceval.c.

# TODO: Reuse C generation framework from deepfreeze.py?

import argparse
import io
import os
import re
import sys

import parser
from parser import InstDef

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-i", "--input", type=str, default="Python/bytecodes.c")
arg_parser.add_argument("-o", "--output", type=str, default="Python/generated_cases.c.h")
arg_parser.add_argument("-c", "--compare", action="store_true")
arg_parser.add_argument("-q", "--quiet", action="store_true")


def eopen(filename: str, mode: str = "r"):
    if filename == "-":
        if "r" in mode:
            return sys.stdin
        else:
            return sys.stdout
    return open(filename, mode)


def parse_cases(src: str, filename: str|None = None) -> tuple[list[InstDef], list[parser.Family]]:
    psr = parser.Parser(src, filename=filename)
    instrs: list[InstDef] = []
    families: list[parser.Family] = []
    while not psr.eof():
        if inst := psr.inst_def():
            assert inst.block
            instrs.append(InstDef(inst.name, inst.inputs, inst.outputs, inst.block))
        elif fam := psr.family_def():
            families.append(fam)
        else:
            raise psr.make_syntax_error(f"Unexpected token")
    return instrs, families


def always_exits(block: parser.Block) -> bool:
    text = block.text
    lines = text.splitlines()
    while lines and not lines[-1].strip():
        lines.pop()
    if not lines or lines[-1].strip() != "}":
        return False
    lines.pop()
    if not lines:
        return False
    line = lines.pop().rstrip()
    # Indent must match exactly (TODO: Do something better)
    if line[:12] != " "*12:
        return False
    line = line[12:]
    return line.startswith(("goto ", "return ", "DISPATCH", "GO_TO_", "Py_UNREACHABLE()"))


def write_cases(f: io.TextIOBase, instrs: list[InstDef]):
    predictions = set()
    for inst in instrs:
        for target in re.findall(r"(?:PREDICT|GO_TO_INSTRUCTION)\((\w+)\)", inst.block.text):
            predictions.add(target)
    indent = "        "
    f.write(f"// This file is generated by {os.path.relpath(__file__)}\n")
    f.write("// Do not edit!\n")
    for instr in instrs:
        assert isinstance(instr, InstDef)
        f.write(f"\n{indent}TARGET({instr.name}) {{\n")
        if instr.name in predictions:
            f.write(f"{indent}    PREDICTED({instr.name});\n")
        # input = ", ".join(instr.inputs)
        # output = ", ".join(instr.outputs)
        # f.write(f"{indent}    // {input} -- {output}\n")
        assert instr.block
        blocklines = instr.block.text.splitlines(True)
        # Remove blank lines from ends
        while blocklines and not blocklines[0].strip():
            blocklines.pop(0)
        while blocklines and not blocklines[-1].strip():
            blocklines.pop()
        # Remove leading '{' and trailing '}'
        assert blocklines and blocklines[0].strip() == "{"
        assert blocklines and blocklines[-1].strip() == "}"
        blocklines.pop()
        blocklines.pop(0)
        # Remove trailing blank lines
        while blocklines and not blocklines[-1].strip():
            blocklines.pop()
        # Write the body
        for line in blocklines:
            f.write(line)
        assert instr.block
        if not always_exits(instr.block):
            f.write(f"{indent}    DISPATCH();\n")
        # Write trailing '}'
        f.write(f"{indent}}}\n")


def main():
    args = arg_parser.parse_args()
    with eopen(args.input) as f:
        srclines = f.read().splitlines()
    begin = srclines.index("// BEGIN BYTECODES //")
    end = srclines.index("// END BYTECODES //")
    src = "\n".join(srclines[begin+1 : end])
    instrs, families = parse_cases(src, filename=args.input)
    ninstrs = nfamilies = 0
    if not args.quiet:
        ninstrs = len(instrs)
        nfamilies = len(families)
        print(
            f"Read {ninstrs} instructions "
            f"and {nfamilies} families from {args.input}",
            file=sys.stderr,
        )
    with eopen(args.output, "w") as f:
        write_cases(f, instrs)
    if not args.quiet:
        print(
            f"Wrote {ninstrs} instructions to {args.output}",
            file=sys.stderr,
        )


if __name__ == "__main__":
    main()
