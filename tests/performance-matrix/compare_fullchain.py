#!/usr/bin/env python3
"""Paired V1.0.0 vs V1.0.1 full DSP-chain audit.

Both executables are built from the same test/performance_matrix.cpp file
with the same compiler and optimization flags in one Windows Actions job.
Inputs, block sizes and sample rates are therefore identical.
"""
import csv
import json
import math
import pathlib
import statistics
import sys
from collections import defaultdict


def matrix(path):
    with open(path, newline="", encoding="utf-8") as stream:
        values = list(csv.DictReader(stream))
    if not values:
        raise AssertionError(f"Missing rows in {path}")
    return {
        (row["scenario"], row["sample_rate"], row["block_size"]): row
        for row in values
    }


def check_renders(path_300, path_301):
    samples = defaultdict(lambda: [0.0, 0.0, 0.0, 0])
    absolute_peak = defaultdict(float)
    with open(path_300, newline="", encoding="utf-8") as stream_a, open(
        path_301, newline="", encoding="utf-8"
    ) as stream_b:
        a, b = csv.DictReader(stream_a), csv.DictReader(stream_b)
        if a.fieldnames != b.fieldnames:
            raise AssertionError("Render headers differ")
        for old, new in zip(a, b):
            if (old["scenario"], old["sample"]) != (new["scenario"], new["sample"]):
                raise AssertionError("Output sample alignment differs")
            scenario = old["scenario"]
            stat = samples[scenario]
            for key in ("in_left", "in_right"):
                if old[key] != new[key]:
                    raise AssertionError("Generated input was not identical")
            for key in ("out_left", "out_right"):
                reference = float(old[key])
                optimized = float(new[key])
                if not math.isfinite(reference) or not math.isfinite(optimized):
                    raise AssertionError("Non-finite output")
                error = optimized - reference
                stat[0] += error * error
                stat[1] += reference * reference
                stat[2] = max(stat[2], abs(error))
                stat[3] += 1
        if next(a, None) is not None or next(b, None) is not None:
            raise AssertionError("One render has extra audio samples")

    if len(samples) < 18:
        raise AssertionError("Incomplete module audio scenario coverage")

    summary = {}
    for scenario, (err2, ref2, peak, frames) in sorted(samples.items()):
        relative_rms = math.sqrt(err2 / max(ref2, 1e-20))
        summary[scenario] = {
            "relative_null_rms": relative_rms,
            "peak_absolute_difference": peak,
            "channels_times_frames": frames
        }
        # The matcher itself must be almost perfectly sample-identical.
        # Full-chain tolerance allows only very small floating-point
        # differences from 1.0.1 coefficient caches.
        limit_rms = 1e-8 if scenario.startswith("MATCH_") else 1e-5
        limit_peak = 1e-8 if scenario.startswith("MATCH_") else 1e-4
        if relative_rms > limit_rms or peak > limit_peak:
            raise AssertionError(
                f"Audio regression in {scenario}: RMS={relative_rms:.5g} "
                f"peak={peak:.5g}, allowed {limit_rms:g} / {limit_peak:g}"
            )

    return summary


def paired_cpu(path_300, path_301):
    a = matrix(path_300)
    b = matrix(path_301)
    if set(a) != set(b):
        raise AssertionError("Baseline/current performance matrix differs")

    summary = {}
    for key in sorted(a):
        old, new = a[key], b[key]
        before = float(old["mean_us"])
        after = float(new["mean_us"])
        if before <= 0 or after <= 0:
            raise AssertionError(f"Invalid CPU data for {key}")
        speedup = before / after
        scenario, rate, block = key
        label = f"{scenario}|{rate}|{block}"
        summary[label] = {
            "baseline_mean_us": before,
            "v101_mean_us": after,
            "speedup": speedup,
            "baseline_p99_us": float(old["p99_us"]),
            "v101_p99_us": float(new["p99_us"]),
            "baseline_max_us": float(old["max_us"]),
            "v101_max_us": float(new["max_us"]),
            "deadline_us": float(new["deadline_us"]),
            "baseline_overruns": int(old["overruns"]),
            "v101_overruns": int(new["overruns"])
        }
        print(
            f"FULLCHAIN {scenario:25} {float(rate):7.0f}Hz "
            f"{int(block):3} buffer | 1.0.0={before:9.3f}us "
            f"1.0.1={after:9.3f}us speedup={speedup:6.2f}x "
            f"p99={float(old['p99_us']):7.2f}/{float(new['p99_us']):7.2f}us"
        )

    def check_gate(scenario, minimum_speedup=None, max_regression=1.20):
        label = f"{scenario}|48000|64"
        result = summary[label]
        if minimum_speedup is not None:
            if result["speedup"] < minimum_speedup:
                raise AssertionError(
                    f"{scenario}: full-chain CPU speedup "
                    f"{result['speedup']:.3f}x < {minimum_speedup:.3f}x"
                )
        else:
            if result["speedup"] < 1.0 / max_regression:
                raise AssertionError(
                    f"{scenario}: CPU regression {1/result['speedup']:.3f}x "
                    f"> {max_regression:.3f}x"
                )

    # Real whole-plugin thresholds: cannot ship based only on a MATCH kernel.
    check_gate("ALL_100", minimum_speedup=1.25)
    check_gate("ALL_50", minimum_speedup=1.20)
    check_gate("MUSICAL_CHAIN", minimum_speedup=1.25)
    check_gate("MATCH_100", minimum_speedup=2.0)
    check_gate("NEUTRAL")
    check_gate("ALL_EXCEPT_MATCH")
    for module in ("FINISH", "MASS", "LOW_CUT"):
        check_gate(module + "_100")
    return summary


def main():
    if len(sys.argv) != 6:
        raise SystemExit(
            "Usage: compare_fullchain.py baseline-cpu.csv new-cpu.csv "
            "baseline-audio.csv new-audio.csv result.json"
        )
    old_cpu, new_cpu, old_audio, new_audio, path_output = sys.argv[1:]
    sound = check_renders(old_audio, new_audio)
    performance = paired_cpu(old_cpu, new_cpu)
    result = {
        "baseline": "V1.0.0 (engineering)",
        "candidate": "V1.0.1",
        "status": "PASS",
        "audio": sound,
        "cpu": performance,
        "notes": (
            "Paired Windows CI comparison. Neither CI performance nor "
            "three-second audio vectors replace Studio One load tests."
        )
    }
    pathlib.Path(path_output).write_text(
        json.dumps(result, indent=2), encoding="utf-8"
    )
    print(
        f"FULL CHAIN COMPARISON PASS: {len(sound)} sound scenarios, "
        f"{len(performance)} CPU scenarios; baseline and candidate matched."
    )


if __name__ == "__main__":
    main()
