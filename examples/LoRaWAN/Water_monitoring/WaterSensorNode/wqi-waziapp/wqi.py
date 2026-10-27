#!/usr/bin/env python3
"""
CCME Water Quality Index (CCME-WQI) from a time series.

The Canadian Council of Ministers of the Environment index compares every
measurement in a time window with an objective (a limit) per variable and
combines three factors:

    F1  scope      share of variables that missed their objective at least once
    F2  frequency  share of all measurements that missed their objective
    F3  amplitude  how far the failed measurements missed, scaled to 0..100

    WQI = 100 - sqrt(F1^2 + F2^2 + F3^2) / 1.732

    95-100 Excellent, 80-94 Good, 65-79 Fair, 45-64 Marginal, 0-44 Poor

The objectives are the index: the same data scores differently against
other limits. They live in objectives.json, one entry per variable, with a
"min", a "max", or both. Both must be greater than 0.

Use as a module (ccme_wqi) or from the command line on a CSV file whose
first column is the time and whose other columns are named like the
variables in objectives.json:

    python wqi.py data.csv [objectives.json]
"""

import csv
import json
import math
import os
import sys

CATEGORIES = [(95, "Excellent"), (80, "Good"), (65, "Fair"), (45, "Marginal"), (0, "Poor")]


def category(wqi):
    for floor, name in CATEGORIES:
        if wqi >= floor:
            return name
    return "Poor"


def to_number(v):
    """A measurement as float, or None for gaps, text and booleans."""
    if isinstance(v, bool) or v is None:
        return None
    try:
        f = float(v)
    except (TypeError, ValueError):
        return None
    return f if math.isfinite(f) else None


def excursion(value, lo=None, hi=None):
    """0 inside the objective, otherwise CCME's excursion.

    Below a minimum: objective / value - 1. A value of 0 would divide by
    zero, so it is held at a thousandth of the objective (excursion 999,
    which drives F3 close to 100, as a dead reading should).
    Above a maximum: value / objective - 1.
    """
    if lo is not None and value < lo:
        return lo / max(value, lo * 1e-3) - 1
    if hi is not None and value > hi:
        return value / hi - 1
    return 0.0


def ccme_wqi(series, objectives):
    """series: {variable: [values]}; objectives: {variable: {"min", "max"}}.

    Variables without a single usable value are left out, as CCME does.
    Returns None when nothing at all could be tested.
    """
    tested = failed_vars = tests = failed_tests = 0
    exc_sum = 0.0
    variables = {}

    for name, lim in objectives.items():
        lo, hi = lim.get("min"), lim.get("max")
        values = [x for x in (to_number(v) for v in series.get(name, [])) if x is not None]
        if not values:
            variables[name] = {"tests": 0, "failed": 0}
            continue
        exc = [excursion(v, lo, hi) for v in values]
        nf = sum(1 for e in exc if e > 0)
        tested += 1
        failed_vars += nf > 0
        tests += len(values)
        failed_tests += nf
        exc_sum += sum(exc)
        variables[name] = {"tests": len(values), "failed": nf,
                           "min": min(values), "max": max(values)}

    if tests == 0:
        return None

    f1 = 100.0 * failed_vars / tested
    f2 = 100.0 * failed_tests / tests
    nse = exc_sum / tests
    f3 = nse / (0.01 * nse + 0.01)
    wqi = max(0.0, 100.0 - math.sqrt(f1 * f1 + f2 * f2 + f3 * f3) / 1.732)

    return {
        "wqi": round(wqi, 1),
        "category": category(wqi),
        "F1": round(f1, 1), "F2": round(f2, 1), "F3": round(f3, 1),
        "variables_tested": tested,
        "tests": tests,
        # CCME asks for at least 4 variables, each sampled at least 4 times
        "reliable": tested >= 4 and all(v["tests"] >= 4 for v in variables.values() if v["tests"]),
        "variables": variables,
    }


def load_objectives(path):
    with open(path, encoding="utf-8") as f:
        cfg = json.load(f)
    for name, lim in cfg["variables"].items():
        if lim.get("min") is None and lim.get("max") is None:
            raise ValueError("%s: needs a min, a max or both" % name)
        for key in ("min", "max"):
            if lim.get(key) is not None and lim[key] <= 0:
                raise ValueError("%s: %s must be greater than 0" % (name, key))
    return cfg


def read_csv(path):
    """Wide CSV: time in the first column, one column per variable."""
    series = {}
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.reader(f)
        header = next(reader)
        for name in header[1:]:
            series[name.strip()] = []
        names = [h.strip() for h in header[1:]]
        for row in reader:
            for name, cell in zip(names, row[1:]):
                series[name].append(cell.strip() if cell.strip() else None)
    return series


def main(argv):
    if len(argv) < 2:
        print(__doc__.strip().split("\n\n")[-1])
        return 2
    here = os.path.dirname(os.path.abspath(__file__))
    cfg = load_objectives(argv[2] if len(argv) > 2 else os.path.join(here, "objectives.json"))
    series = read_csv(argv[1])
    unknown = [n for n in series if n not in cfg["variables"]]
    if unknown:
        print("ignored columns (not in objectives): " + ", ".join(unknown))
    r = ccme_wqi(series, cfg["variables"])
    if r is None:
        print("no usable values")
        return 1
    print("WQI %.1f  %s   (F1 %.1f  F2 %.1f  F3 %.1f, %d variables, %d measurements%s)"
          % (r["wqi"], r["category"], r["F1"], r["F2"], r["F3"], r["variables_tested"],
             r["tests"], "" if r["reliable"] else ", too few for a reliable index"))
    for name, v in r["variables"].items():
        if v["tests"]:
            print("  %-20s %4d of %4d outside   range %g .. %g"
                  % (name, v["failed"], v["tests"], v["min"], v["max"]))
        else:
            print("  %-20s no data" % name)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
