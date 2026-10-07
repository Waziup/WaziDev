# -*- coding: utf-8 -*-
"""Turn an SVG into filled KiCad silkscreen polygons.

   KiCad's gr_poly has no notion of a hole, so a glyph counter - the gap in an
   A, a P, a U - cannot simply be a second contour. Each hole is spliced into
   its outer contour through a hairline bridge at their closest pair of points,
   which is the standard way to express a hole in a single self-touching
   outline. Without that the letters fill in solid and the logo is a blob.
"""
from __future__ import print_function
import io, re, sys, math, codecs
if __name__ == '__main__':
    sys.stdout = codecs.getwriter('ascii')(sys.stdout.buffer, 'replace')

FLAT = 0.25          # curve flattening, in SVG units


def tokens(d):
    return re.findall(r'[MmLlHhVvCcSsQqTtAaZz]|-?\d*\.?\d+(?:[eE][-+]?\d+)?', d)


def bez3(p0, p1, p2, p3, n):
    out = []
    for i in range(1, n + 1):
        t = float(i) / n
        u = 1 - t
        out.append((u*u*u*p0[0] + 3*u*u*t*p1[0] + 3*u*t*t*p2[0] + t*t*t*p3[0],
                    u*u*u*p0[1] + 3*u*u*t*p1[1] + 3*u*t*t*p2[1] + t*t*t*p3[1]))
    return out


def steps(a, b):
    return max(3, min(24, int(math.hypot(b[0]-a[0], b[1]-a[1]) / FLAT) + 3))


def parse(d):
    """-> list of closed contours, each a list of (x, y)"""
    t = tokens(d)
    i, cur, start, contours, pts, prev_c = 0, (0.0, 0.0), (0.0, 0.0), [], [], None
    cmd = None
    while i < len(t):
        if re.match(r'[A-Za-z]', t[i]):
            cmd = t[i]; i += 1
        if cmd in 'Zz':
            if len(pts) > 2:
                contours.append(pts)
            pts = []; cur = start; prev_c = None
            continue
        f = lambda k: float(t[i + k])
        rel = cmd.islower()
        if cmd in 'Mm':
            p = (f(0), f(1)); i += 2
            cur = (cur[0] + p[0], cur[1] + p[1]) if rel else p
            if len(pts) > 2:
                contours.append(pts)
            pts = [cur]; start = cur; prev_c = None
            cmd = 'l' if rel else 'L'
        elif cmd in 'Ll':
            p = (f(0), f(1)); i += 2
            cur = (cur[0] + p[0], cur[1] + p[1]) if rel else p
            pts.append(cur); prev_c = None
        elif cmd in 'Hh':
            x = f(0); i += 1
            cur = (cur[0] + x, cur[1]) if rel else (x, cur[1])
            pts.append(cur); prev_c = None
        elif cmd in 'Vv':
            y = f(0); i += 1
            cur = (cur[0], cur[1] + y) if rel else (cur[0], y)
            pts.append(cur); prev_c = None
        elif cmd in 'CcSs':
            if cmd in 'Cc':
                a = (f(0), f(1)); b = (f(2), f(3)); p = (f(4), f(5)); i += 6
                if rel:
                    a = (cur[0]+a[0], cur[1]+a[1]); b = (cur[0]+b[0], cur[1]+b[1])
                    p = (cur[0]+p[0], cur[1]+p[1])
            else:
                b = (f(0), f(1)); p = (f(2), f(3)); i += 4
                if rel:
                    b = (cur[0]+b[0], cur[1]+b[1]); p = (cur[0]+p[0], cur[1]+p[1])
                a = (2*cur[0]-prev_c[0], 2*cur[1]-prev_c[1]) if prev_c else cur
            pts += bez3(cur, a, b, p, steps(cur, p))
            prev_c = b; cur = p
        elif cmd in 'QqTt':
            if cmd in 'Qq':
                a = (f(0), f(1)); p = (f(2), f(3)); i += 4
                if rel:
                    a = (cur[0]+a[0], cur[1]+a[1]); p = (cur[0]+p[0], cur[1]+p[1])
            else:
                p = (f(0), f(1)); i += 2
                if rel:
                    p = (cur[0]+p[0], cur[1]+p[1])
                a = (2*cur[0]-prev_c[0], 2*cur[1]-prev_c[1]) if prev_c else cur
            c1 = (cur[0] + 2.0/3*(a[0]-cur[0]), cur[1] + 2.0/3*(a[1]-cur[1]))
            c2 = (p[0] + 2.0/3*(a[0]-p[0]), p[1] + 2.0/3*(a[1]-p[1]))
            pts += bez3(cur, c1, c2, p, steps(cur, p))
            prev_c = a; cur = p
        elif cmd in 'Aa':
            i += 7                      # no arcs in this logo; skip the params
            p = (float(t[i-2]), float(t[i-1]))
            cur = (cur[0]+p[0], cur[1]+p[1]) if rel else p
            pts.append(cur); prev_c = None
        else:
            i += 1
    if len(pts) > 2:
        contours.append(pts)
    return contours


def area(c):
    s = 0.0
    for i in range(len(c)):
        x1, y1 = c[i]; x2, y2 = c[(i + 1) % len(c)]
        s += x1 * y2 - x2 * y1
    return s / 2.0


def inside(p, c):
    x, y = p; n = len(c); hit = False
    for i in range(n):
        x1, y1 = c[i]; x2, y2 = c[(i + 1) % n]
        if (y1 > y) != (y2 > y) and x < (x2 - x1) * (y - y1) / (y2 - y1) + x1:
            hit = not hit
    return hit


def splice(outer, hole):
    """Join a hole into its outer contour through the shortest bridge."""
    best, bi, bj = 1e18, 0, 0
    for i, a in enumerate(outer):
        for j, b in enumerate(hole):
            d = (a[0]-b[0])**2 + (a[1]-b[1])**2
            if d < best:
                best, bi, bj = d, i, j
    h = hole[bj:] + hole[:bj + 1]
    return outer[:bi + 1] + h + outer[bi:]
