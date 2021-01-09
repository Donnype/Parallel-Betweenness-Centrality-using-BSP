def read(file):
    data = {
        'p1': [],
        'p2': [],
        'p3': [],
        'p4': [],
        'p5': [],
        'p6': [],
        'p7': [],
        'p8': [],
    }

    ns = []
    ps = []
    mss = []

    with open(file, 'r') as f:
            f.readline()

            for line in f:
                n, p, ms = line.split("\t")
                ns.append(n)
                ps.append(p)
                mss.append(ms)

    for i, m in enumerate(mss):
        key = 'p' + ps[i]
        data[key].append(float(m.strip()))

    print_table(data)


def print_table(data):
    lines = ""
    i = 3

    for n in [f"$10^{i}$" for i in range(3, 10)]:
        line = f"                {n} "

        for t in data.values():
            line += f"& ${t[i - 3]}$ "

        line += """ \\\\
"""

        lines += line
        i += 1

    table = """
        \\begin{tabular}{ |c|c c c c c c c c| }
            \hline
            & 1 & 2 & 3 & 4 & 5 & 6 & 7 & 8 \\\\
            \hline
"""
    table += lines
    table += """            \hline
        \end{tabular}
"""

    print(table)


def main():
    for file in ['parallel_benchmark.txt', 'log_parallel_benchmark.txt', 'fast_parallel_benchmark.txt']:
        print(file)
        read(file)


if __name__ == '__main__':
    main()

