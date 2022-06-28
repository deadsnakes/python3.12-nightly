"""Print a summary of specialization stats for all files in the
default stats folders.
"""

import collections
import os.path
import opcode
from datetime import date
import itertools
import sys

if os.name == "nt":
    DEFAULT_DIR = "c:\\temp\\py_stats\\"
else:
    DEFAULT_DIR = "/tmp/py_stats/"

#Create list of all instruction names
specialized = iter(opcode._specialized_instructions)
opname = ["<0>"]
for name in opcode.opname[1:]:
    if name.startswith("<"):
        try:
            name = next(specialized)
        except StopIteration:
            pass
    opname.append(name)

# opcode_name --> opcode
# Sort alphabetically.
opmap = {name: i for i, name in enumerate(opname)}
opmap = dict(sorted(opmap.items()))

TOTAL = "specialization.deferred", "specialization.hit", "specialization.miss", "execution_count"

def print_specialization_stats(name, family_stats, defines):
    if "specializable" not in family_stats:
        return
    total = sum(family_stats.get(kind, 0) for kind in TOTAL)
    if total == 0:
        return
    with Section(name, 3, f"specialization stats for {name} family"):
        rows = []
        for key in sorted(family_stats):
            if key.startswith("specialization.failure_kinds"):
                continue
            if key in ("specialization.hit", "specialization.miss"):
                label = key[len("specialization."):]
            elif key == "execution_count":
                label = "unquickened"
            elif key in ("specialization.success",  "specialization.failure", "specializable"):
                continue
            elif key.startswith("pair"):
                continue
            else:
                label = key
            rows.append((f"{label:>12}", f"{family_stats[key]:>12}", f"{100*family_stats[key]/total:0.1f}%"))
        emit_table(("Kind", "Count", "Ratio"), rows)
        print_title("Specialization attempts", 4)
        total_attempts = 0
        for key in ("specialization.success",  "specialization.failure"):
            total_attempts += family_stats.get(key, 0)
        rows = []
        if total_attempts:
            for key in ("specialization.success",  "specialization.failure"):
                label = key[len("specialization."):]
                label = label[0].upper() + label[1:]
                val = family_stats.get(key, 0)
                rows.append((label, val, f"{100*val/total_attempts:0.1f}%"))
            emit_table(("", "Count:", "Ratio:"), rows)
        total_failures = family_stats.get("specialization.failure", 0)
        failure_kinds = [ 0 ] * 30
        for key in family_stats:
            if not key.startswith("specialization.failure_kind"):
                continue
            _, index = key[:-1].split("[")
            index =  int(index)
            failure_kinds[index] = family_stats[key]
        failures = [(value, index) for (index, value) in enumerate(failure_kinds)]
        failures.sort(reverse=True)
        rows = []
        for value, index in failures:
            if not value:
                continue
            rows.append((kind_to_text(index, defines, name), value, f"{100*value/total_failures:0.1f}%"))
        emit_table(("Failure kind", "Count:", "Ratio:"), rows)

def gather_stats():
    stats = collections.Counter()
    for filename in os.listdir(DEFAULT_DIR):
        with open(os.path.join(DEFAULT_DIR, filename)) as fd:
            for line in fd:
                try:
                    key, value = line.split(":")
                except ValueError:
                    print (f"Unparsable line: '{line.strip()}' in  {filename}", file=sys.stderr)
                    continue
                key = key.strip()
                value = int(value)
                stats[key] += value
    return stats

def extract_opcode_stats(stats):
    opcode_stats = [ {} for _ in range(256) ]
    for key, value in stats.items():
        if not key.startswith("opcode"):
            continue
        n, _, rest = key[7:].partition("]")
        opcode_stats[int(n)][rest.strip(".")] = value
    return opcode_stats

def parse_kinds(spec_src, prefix="SPEC_FAIL"):
    defines = collections.defaultdict(list)
    start = "#define " + prefix + "_"
    for line in spec_src:
        line = line.strip()
        if not line.startswith(start):
            continue
        line = line[len(start):]
        name, val = line.split()
        defines[int(val.strip())].append(name.strip())
    return defines

def pretty(defname):
    return defname.replace("_", " ").lower()

def kind_to_text(kind, defines, opname):
    if kind < 7:
        return pretty(defines[kind][0])
    if opname.endswith("ATTR"):
        opname = "ATTR"
    if opname.endswith("SUBSCR"):
        opname = "SUBSCR"
    for name in defines[kind]:
        if name.startswith(opname):
            return pretty(name[len(opname)+1:])
    return "kind " + str(kind)

def categorized_counts(opcode_stats):
    basic = 0
    specialized = 0
    not_specialized = 0
    specialized_instructions = {
        op for op in opcode._specialized_instructions
        if "__" not in op and "ADAPTIVE" not in op}
    adaptive_instructions = {
        op for op in opcode._specialized_instructions
        if "ADAPTIVE" in op}
    for i, opcode_stat in enumerate(opcode_stats):
        if "execution_count" not in opcode_stat:
            continue
        count = opcode_stat['execution_count']
        name = opname[i]
        if "specializable" in opcode_stat:
            not_specialized += count
        elif name in adaptive_instructions:
            not_specialized += count
        elif name in specialized_instructions:
            miss = opcode_stat.get("specialization.miss", 0)
            not_specialized += miss
            specialized += count - miss
        else:
            basic += count
    return basic, not_specialized, specialized

def print_title(name, level=2):
    print("#"*level, name)
    print()

class Section:

    def __init__(self, title, level=2, summary=None):
        self.title = title
        self.level = level
        if summary is None:
            self.summary = title.lower()
        else:
            self.summary = summary

    def __enter__(self):
        print_title(self.title, self.level)
        print("<details>")
        print("<summary>", self.summary, "</summary>")
        print()
        return self

    def __exit__(*args):
        print()
        print("</details>")
        print()

def to_str(x):
    if isinstance(x, int):
        return format(x, ",d")
    else:
        return str(x)

def emit_table(header, rows):
    width = len(header)
    header_line = "|"
    under_line = "|"
    for item in header:
        under = "---"
        if item.endswith(":"):
            item = item[:-1]
            under += ":"
        header_line += item + " | "
        under_line += under + "|"
    print(header_line)
    print(under_line)
    for row in rows:
        if width is not None and len(row) != width:
            raise ValueError("Wrong number of elements in row '" + str(row) + "'")
        print("|", " | ".join(to_str(i) for i in row), "|")
    print()

def emit_execution_counts(opcode_stats, total):
    with Section("Execution counts", summary="execution counts for all instructions"):
        counts = []
        for i, opcode_stat in enumerate(opcode_stats):
            if "execution_count" in opcode_stat:
                count = opcode_stat['execution_count']
                miss = 0
                if "specializable" not in opcode_stat:
                    miss = opcode_stat.get("specialization.miss")
                counts.append((count, opname[i], miss))
        counts.sort(reverse=True)
        cumulative = 0
        rows = []
        for (count, name, miss) in counts:
            cumulative += count
            if miss:
                miss =  f"{100*miss/count:0.1f}%"
            else:
                miss = ""
            rows.append((name, count, f"{100*count/total:0.1f}%",
                        f"{100*cumulative/total:0.1f}%", miss))
        emit_table(
            ("Name", "Count:", "Self:", "Cumulative:", "Miss ratio:"),
            rows
        )


def emit_specialization_stats(opcode_stats):
    spec_path = os.path.join(os.path.dirname(__file__), "../../Python/specialize.c")
    with open(spec_path) as spec_src:
        defines = parse_kinds(spec_src)
    with Section("Specialization stats", summary="specialization stats by family"):
        for i, opcode_stat in enumerate(opcode_stats):
            name = opname[i]
            print_specialization_stats(name, opcode_stat, defines)

def emit_specialization_overview(opcode_stats, total):
    basic, not_specialized, specialized = categorized_counts(opcode_stats)
    with Section("Specialization effectiveness"):
        emit_table(("Instructions", "Count:", "Ratio:"), (
            ("Basic", basic, f"{basic*100/total:0.1f}%"),
            ("Not specialized", not_specialized, f"{not_specialized*100/total:0.1f}%"),
            ("Specialized", specialized, f"{specialized*100/total:0.1f}%"),
        ))
        for title, field in (("Deferred", "specialization.deferred"), ("Misses", "specialization.miss")):
            total = 0
            counts = []
            for i, opcode_stat in enumerate(opcode_stats):
                value = opcode_stat.get(field, 0)
                counts.append((value, opname[i]))
                total += value
            counts.sort(reverse=True)
            if total:
                with Section(f"{title} by instruction", 3):
                    rows = [ (name, count, f"{100*count/total:0.1f}%") for (count, name) in counts[:10] ]
                    emit_table(("Name", "Count:", "Ratio:"), rows)

def emit_call_stats(stats):
    stats_path = os.path.join(os.path.dirname(__file__), "../../Include/pystats.h")
    with open(stats_path) as stats_src:
        defines = parse_kinds(stats_src, prefix="EVAL_CALL")
    with Section("Call stats", summary="Inlined calls and frame stats"):
        total = 0
        for key, value in stats.items():
            if "Calls to" in key:
                total += value
        rows = []
        for key, value in stats.items():
            if "Calls to" in key:
                rows.append((key, value, f"{100*value/total:0.1f}%"))
            elif key.startswith("Calls "):
                name, index = key[:-1].split("[")
                index =  int(index)
                label = name + " (" + pretty(defines[index][0]) + ")"
                rows.append((label, value, f"{100*value/total:0.1f}%"))
        for key, value in stats.items():
            if key.startswith("Frame"):
                rows.append((key, value, f"{100*value/total:0.1f}%"))
        emit_table(("", "Count:", "Ratio:"), rows)

def emit_object_stats(stats):
    with Section("Object stats", summary="allocations, frees and dict materializatons"):
        total_materializations = stats.get("Object new values")
        total_allocations = stats.get("Object allocations") + stats.get("Object allocations from freelist")
        total_increfs = stats.get("Object interpreter increfs") + stats.get("Object increfs")
        total_decrefs = stats.get("Object interpreter decrefs") + stats.get("Object decrefs")
        rows = []
        for key, value in stats.items():
            if key.startswith("Object"):
                if "materialize" in key:
                    ratio = f"{100*value/total_materializations:0.1f}%"
                elif "allocations" in key:
                    ratio = f"{100*value/total_allocations:0.1f}%"
                elif "increfs"     in key:
                    ratio = f"{100*value/total_increfs:0.1f}%"
                elif "decrefs"     in key:
                    ratio = f"{100*value/total_decrefs:0.1f}%"
                else:
                    ratio = ""
                label = key[6:].strip()
                label = label[0].upper() + label[1:]
                rows.append((label, value, ratio))
        emit_table(("",  "Count:", "Ratio:"), rows)

def get_total(opcode_stats):
    total = 0
    for opcode_stat in opcode_stats:
        if "execution_count" in opcode_stat:
            total += opcode_stat['execution_count']
    return total

def emit_pair_counts(opcode_stats, total):
    pair_counts = []
    for i, opcode_stat in enumerate(opcode_stats):
        if i == 0:
            continue
        for key, value in opcode_stat.items():
            if key.startswith("pair_count"):
                x, _, _ = key[11:].partition("]")
                if value:
                    pair_counts.append((value, (i, int(x))))
    with Section("Pair counts", summary="Pair counts for top 100 pairs"):
        pair_counts.sort(reverse=True)
        cumulative = 0
        rows = []
        for (count, pair) in itertools.islice(pair_counts, 100):
            i, j = pair
            cumulative += count
            rows.append((opname[i] + " " + opname[j], count, f"{100*count/total:0.1f}%",
                        f"{100*cumulative/total:0.1f}%"))
        emit_table(("Pair", "Count:", "Self:", "Cumulative:"),
            rows
        )
    with Section("Predecessor/Successor Pairs", summary="Top 5 predecessors and successors of each opcode"):
        predecessors = collections.defaultdict(collections.Counter)
        successors = collections.defaultdict(collections.Counter)
        total_predecessors = collections.Counter()
        total_successors = collections.Counter()
        for count, (first, second) in pair_counts:
            if count:
                predecessors[second][first] = count
                successors[first][second] = count
                total_predecessors[second] += count
                total_successors[first] += count
        for name, i in opmap.items():
            total1 = total_predecessors[i]
            total2 = total_successors[i]
            if total1 == 0 and total2 == 0:
                continue
            pred_rows = succ_rows = ()
            if total1:
                pred_rows = [(opname[pred], count, f"{count/total1:.1%}")
                             for (pred, count) in predecessors[i].most_common(5)]
            if total2:
                succ_rows = [(opname[succ], count, f"{count/total2:.1%}")
                             for (succ, count) in successors[i].most_common(5)]
            with Section(name, 3, f"Successors and predecessors for {name}"):
                emit_table(("Predecessors", "Count:", "Percentage:"),
                    pred_rows
                )
                emit_table(("Successors", "Count:", "Percentage:"),
                    succ_rows
                )

def main():
    stats = gather_stats()
    opcode_stats = extract_opcode_stats(stats)
    total = get_total(opcode_stats)
    emit_execution_counts(opcode_stats, total)
    emit_pair_counts(opcode_stats, total)
    emit_specialization_stats(opcode_stats)
    emit_specialization_overview(opcode_stats, total)
    emit_call_stats(stats)
    emit_object_stats(stats)
    print("---")
    print("Stats gathered on:", date.today())

if __name__ == "__main__":
    main()
