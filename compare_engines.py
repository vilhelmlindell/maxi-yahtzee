#!/usr/bin/env python3

import subprocess
import numpy as np
from scipy import stats
import sys


def start_engine(path):
    return subprocess.Popen(
        [path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        text=True,
        bufsize=1
    )


def run_games(proc, n):
    """
    Send n to engine stdin and read n lines of:
        <score> <vps>
    """
    proc.stdin.write(f"{n}\n")
    proc.stdin.flush()

    scores = []
    vps = []

    for _ in range(n):
        line = proc.stdout.readline()
        if not line:
            break

        parts = line.strip().split()
        if len(parts) != 2:
            continue

        try:
            s = float(parts[0])
            v = float(parts[1])
        except ValueError:
            continue

        scores.append(s)
        vps.append(v)

    return scores, vps


def sequential_test(engine_a, engine_b, batch_size=50, confidence=0.95):
    """
    Engine A is always the new version.
    Engine B is always the old version.
    All statistics are reported as A - B.
    """
    proc_a = start_engine(engine_a)
    proc_b = start_engine(engine_b)

    scores_a, scores_b = [], []
    vps_a, vps_b = [], []

    while True:
        sa, va = run_games(proc_a, batch_size)
        sb, vb = run_games(proc_b, batch_size)

        if not sa or not sb:
            print("Error: engine returned no data")
            return

        scores_a.extend(sa)
        scores_b.extend(sb)
        vps_a.extend(va)
        vps_b.extend(vb)

        n = len(scores_a)

        mean_a = np.mean(scores_a)
        mean_b = np.mean(scores_b)
        mean_vps_a = np.mean(vps_a)
        mean_vps_b = np.mean(vps_b)

        var_a = np.var(scores_a, ddof=1)
        var_b = np.var(scores_b, ddof=1)

        delta = mean_a - mean_b
        se = np.sqrt(var_a / n + var_b / n)

        df = (var_a / n + var_b / n) ** 2 / (
            (var_a ** 2) / (n ** 2 * (n - 1)) +
            (var_b ** 2) / (n ** 2 * (n - 1))
        )

        tcrit = stats.t.ppf(confidence, df)
        lo = delta - tcrit * se
        hi = delta + tcrit * se

        print(
            f"N={n:6d} | "
            f"A(new)={mean_a:7.2f}  B(old)={mean_b:7.2f} | "
            f"Δ(A−B)={delta:6.2f}  CI=[{lo:6.2f}, {hi:6.2f}] | "
            f"VPS A={mean_vps_a:,.0f}  B={mean_vps_b:,.0f}"
        )

        # Stop once the CI excludes zero (either direction)
        if lo > 0 or hi < 0:
            print_final(
                mean_a=mean_a,
                mean_b=mean_b,
                delta=delta,
                lo=lo,
                hi=hi,
                n=n,
                vps_a=mean_vps_a,
                vps_b=mean_vps_b,
                confidence=confidence,
            )
            return


def print_final(mean_a, mean_b, delta, lo, hi, n, vps_a, vps_b, confidence):
    print()
    print("Score comparison:")
    print(f"Score: {mean_b:.2f} -> {mean_a:.2f} (Δ = {delta:+.2f})")
    print(f"{int(confidence*100)}% CI: [{lo:.2f}, {hi:.2f}]")
    print(f"Games: {n}")
    print(f"Vps: {vps_b:.0f} -> {vps_a:.0f}")
    print()

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: compare_engines.py <new_engine> <old_engine>")
        sys.exit(1)

    sequential_test(sys.argv[2], sys.argv[1])
