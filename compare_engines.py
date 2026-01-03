#!/usr/bin/env python3

import subprocess
import numpy as np
from scipy import stats
import argparse
import sys

def start_engine(path):
    return subprocess.Popen(
        [path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

def run_games(proc, n, engine_args, label="Engine", verbose=False):
    arg_str = " ".join(str(x) for x in engine_args)
    proc.stdin.write(f"{n} {arg_str}\n")
    proc.stdin.flush()

    scores = []
    vps = []

    for i in range(n):
        line = proc.stdout.readline()
        if not line:
            print(f"[{label}] Unexpected EOF")
            break
        
        line = line.strip()
        if verbose:
            print(f"[{label} out]: {line}")

        parts = line.split()
        if len(parts) != 2:
            continue

        try:
            scores.append(float(parts[0]))
            vps.append(float(parts[1]))
        except ValueError:
            continue

    return scores, vps

def print_final(mean_old, mean_new, delta, lo, hi, n, vps_old, vps_new, confidence):
    print()
    print("Score comparison (Old -> New):")
    print(f"Score: {mean_old:.2f} -> {mean_new:.2f} (Δ = {delta:+.2f})")
    print(f"{int(confidence*100)}% CI: [{lo:.2f}, {hi:.2f}]")
    print(f"Games: {n}")
    print(f"Vps: {vps_old:.0f} -> {vps_new:.0f}")
    print()

def sequential_test(engine_old, engine_new, batch_size, engine_args, confidence, verbose):
    proc_a = start_engine(engine_old)
    proc_b = start_engine(engine_new)

    all_scores_old, all_scores_new = [], []
    all_vps_old, all_vps_new = [], []
    
    n, mean_old, mean_new, delta, lo, hi = 0, 0, 0, 0, 0, 0
    mvps_old, mvps_new = 0, 0
    
    # Track significance to print special alerts
    already_significant = False 

    try:
        while True:
            sa, va = run_games(proc_a, batch_size, engine_args, "Old (A)", verbose)
            sb, vb = run_games(proc_b, batch_size, engine_args, "New (B)", verbose)

            if len(sa) == 0 or len(sb) == 0:
                print("Error: One of the engines failed to return data.")
                break

            all_scores_old.extend(sa)
            all_scores_new.extend(sb)
            all_vps_old.extend(va)
            all_vps_new.extend(vb)

            n = len(all_scores_old)
            if n < 2: continue 

            mean_old, mean_new = np.mean(all_scores_old), np.mean(all_scores_new)
            var_old, var_new = np.var(all_scores_old, ddof=1), np.var(all_scores_new, ddof=1)
            mvps_old, mvps_new = np.mean(all_vps_old), np.mean(all_vps_new)
            
            delta = mean_new - mean_old
            se = np.sqrt(var_old/n + var_new/n)
            
            # Welch-Satterthwaite df
            num = (var_old/n + var_new/n)**2
            den = (var_old**2 / (n**2 * (n-1))) + (var_new**2 / (n**2 * (n-1)))
            df = num / den

            tcrit = stats.t.ppf((1 + confidence) / 2, df)
            lo, hi = delta - tcrit * se, delta + tcrit * se

            # Determine significance
            is_significant = (lo > 0 or hi < 0)
            sig_char = "*" if is_significant else " "

            print(
                f"N={n:5d} | "
                f"Old(A)={mean_old:7.2f} New(B)={mean_new:7.2f} | "
                f"Δ={delta:+6.2f} CI=[{lo:6.2f}, {hi:6.2f}] {sig_char} | "
                f"VPS Old={int(mvps_old):,} New={int(mvps_new):,}"
            )

            # Print an alert only when significance is first reached
            if is_significant and not already_significant:
                print(f"\n>>> STATISTICALLY SIGNIFICANT RESULT REACHED AT N={n} <<<")
                already_significant = True
            elif not is_significant:
                already_significant = False

    except KeyboardInterrupt:
        print("\nInterrupted by user.")
    finally:
        if n >= 2:
            print_final(mean_old, mean_new, delta, lo, hi, n, mvps_old, mvps_new, confidence)
        
        proc_a.terminate()
        proc_b.terminate()

def main():
    parser = argparse.ArgumentParser()
    # Updated Argument Order
    parser.add_argument("old_engine", help="Path to the baseline engine")
    parser.add_argument("new_engine", help="Path to the engine being tested")
    parser.add_argument("--batch-size", type=int, default=50)
    parser.add_argument("--confidence", type=float, default=0.95)
    parser.add_argument("--ms-per-move", type=int, default=10)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("-v", "--verbose", action="store_true", help="Show raw engine output")

    args = parser.parse_args()

    sequential_test(
        args.old_engine, args.new_engine, 
        args.batch_size, [args.ms_per_move, args.threads], 
        args.confidence, args.verbose
    )

if __name__ == "__main__":
    main()
