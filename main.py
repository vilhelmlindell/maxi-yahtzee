#!/usr/bin/env python3
import subprocess
import numpy as np
from scipy import stats

def run_games(executable, n):
    """
    Run `n` games using the engine executable.
    Returns a list of integer scores.
    """
    # Launch the process
    proc = subprocess.Popen([executable], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    
    # Send n to stdin
    stdout, _ = proc.communicate(input=str(n))
    
    # Parse output lines as integers
    scores = [int(line) for line in stdout.strip().splitlines() if line.strip().isdigit()]
    return scores

def sequential_test(engine_a, engine_b, batch_size=10, confidence=0.95, max_batches=1000):
    scores_a, scores_b = [], []
    alpha = 1 - confidence
    
    for batch_num in range(max_batches):
        # Run a new batch
        new_a = run_games(engine_a, batch_size)
        new_b = run_games(engine_b, batch_size)
        scores_a.extend(new_a)
        scores_b.extend(new_b)
        
        n_a = len(scores_a)
        n_b = len(scores_b)
        
        mean_a = np.mean(scores_a)
        mean_b = np.mean(scores_b)
        var_a = np.var(scores_a, ddof=1)
        var_b = np.var(scores_b, ddof=1)
        
        # Welch-Satterthwaite degrees of freedom
        se = np.sqrt(var_a / n_a + var_b / n_b)
        df = (var_a / n_a + var_b / n_b)**2 / ((var_a**2 / (n_a**2 * (n_a - 1))) + (var_b**2 / (n_b**2 * (n_b - 1))))
        
        # t critical value for one-sided CI
        t_crit = stats.t.ppf(confidence, df)
        delta_mean = mean_a - mean_b
        ci_lower = delta_mean - t_crit * se
        ci_upper = delta_mean + t_crit * se
        
        print(f"Batch {batch_num+1}: mean_a={mean_a:.2f}, mean_b={mean_b:.2f}, delta={delta_mean:.2f}, CI=({ci_lower:.2f}, {ci_upper:.2f})")
        
        # Check stopping condition
        if ci_lower > 0:
            print(f"\nEngine A is better with {confidence*100:.1f}% confidence!")
            print(f"Estimated improvement: {delta_mean:.2f} points (95% CI: {ci_lower:.2f}, {ci_upper:.2f})")
            return
        elif ci_upper < 0:
            print(f"\nEngine B is better with {confidence*100:.1f}% confidence!")
            print(f"Estimated improvement: {-delta_mean:.2f} points (95% CI: {-ci_upper:.2f}, {-ci_lower:.2f})")
            return
    
    print("\nMax batches reached, no engine reaches 95% confidence.")
    print(f"Current estimate: delta={delta_mean:.2f} points (CI: {ci_lower:.2f}, {ci_upper:.2f})")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python compare_yahtzee.py /path/to/engineA /path/to/engineB")
        sys.exit(1)
    
    engine_a = sys.argv[1]
    engine_b = sys.argv[2]
    
    sequential_test(engine_a, engine_b)
