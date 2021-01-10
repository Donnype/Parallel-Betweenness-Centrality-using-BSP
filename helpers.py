import matplotlib.pyplot as plt
import numpy as np


def tables():
    datas = get_data()

    lines = ""

    for i in range(8):
        line = f'{i + 1} '
        for s, data in datas.items():
            data = data[:, 1]
            line += f'& ${round(data[i], 2)}$ '

        line += """ \\\\
"""
        lines += line

    print(lines)


def get_data():
    datas = {}

    files = ['data/f_10_s_3_n_8400_sparse.txt', 'data/s_30_n_8400_sparse.txt']
    labels = ['s = 3, f = 10', 's = 30, f = 1']
    for label, file in zip(labels, files):
    # for s in [2, 5, 10, 100]:
        # file = f'data/8400_{s}_sparse.bfs'
        print(file)
        data = []

        with open(file, 'r') as f:
            for line in f:
                data.append(line.strip().split("\t"))

        data = np.array(data).astype(float)
        # datas[s] = data
        datas[label] = data

    return datas


def figures():
    for label, data in get_data().items():
        plt.plot(data[:, 0], data[:, 1], label=f'{label}')

    plt.xlabel('Number of processors')
    plt.ylabel('Time (ms)')
    plt.grid()
    plt.legend(loc='upper right')
    name = f'figures/long-or-not.png'
    plt.savefig(name, dpi=500)
    plt.close()


def main():
    tables()
    # figures()


if __name__ == '__main__':
    main()

