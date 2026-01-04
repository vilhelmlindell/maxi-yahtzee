#!/usr/bin/env python3

import subprocess
import numpy as np
from scipy import stats
import argparse
import shlex

def start_engine(cmd_string):
    # Splits the string into a list, e.g., './engine -d' becomes ['./engine', '-d']
    args = shlex.split(cmd_string)
    return subprocess.Popen(
        args,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

def run_games(proc, n, label="Engine", verbose=False):
    # The engine now only receives the game count per batch via stdin
    arg_str = f"-g {n}"
    
    if verbose:
        print(f"[{label} in]: {arg_str}")
        
    proc.stdin.write(f"{arg_str}\n")
    proc.stdin.flush()

    scores = []
    vps = []

    for i in range(n):
        line = proc.stdout.readline()
        if not line:
            break
        
        line = line.strip()
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
    print("\nScore comparison (Old -> New):")
    print(f"Score: {mean_old:.2f} -> {mean_new:.2f} (Δ = {delta:+.2f})")
    print(f"{int(confidence*100)}% CI: [{lo:.2f}, {hi:.2f}]")
    print(f"Games: {n}")
    print(f"Vps: {vps_old:.0f} -> {vps_new:.0f}\n")

def sequential_test(engine_old_cmd, engine_new_cmd, batch_size, confidence, verbose):
    proc_a = start_engine(engine_old_cmd)
    proc_b = start_engine(engine_new_cmd)

    all_scores_old, all_scores_new = [], []
    all_vps_old, all_vps_new = [], []
    
    n, mean_old, mean_new, delta, lo, hi = 0, 0, 0, 0, 0, 0
    mvps_old, mvps_new = 0, 0
    already_significant = False 

    try:
        while True:
            sa, va = run_games(proc_a, batch_size, "Old (A)", verbose)
            sb, vb = run_games(proc_b, batch_size, "New (B)", verbose)

            if len(sa) == 0 or len(sb) == 0:
                print("Error: One of the engines failed to return data. Check commands.")
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
            
            num = (var_old/n + var_new/n)**2
            den = (var_old**2 / (n**2 * (n-1))) + (var_new**2 / (n**2 * (n-1)))
            df = num / den

            tcrit = stats.t.ppf((1 + confidence) / 2, df)
            lo, hi = delta - tcrit * se, delta + tcrit * se

            is_significant = (lo > 0 or hi < 0)
            sig_char = "*" if is_significant else " "

            print(
                f"N={n:5d} | "
                f"Old(A)={mean_old:7.2f} New(B)={mean_new:7.2f} | "
                f"Δ={delta:+6.2f} CI=[{lo:6.2f}, {hi:6.2f}] {sig_char} | "
                f"VPS Old={int(mvps_old):,} New={int(mvps_new):,}"
            )

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
    parser.add_argument("old_engine", help="Command to run old engine (e.g. './engine -t 10')")
    parser.add_argument("new_engine", help="Command to run new engine (e.g. './engine -t 10 -d')")
    parser.add_argument("--batch-size", type=int, default=50)
    parser.add_argument("--confidence", type=float, default=0.95)
    parser.add_argument("-v", "--verbose", action="store_true")

    args = parser.parse_args()

    sequential_test(
        args.old_engine, args.new_engine, 
        args.batch_size, args.confidence, args.verbose
    )

if __name__ == "__main__":
    main()
