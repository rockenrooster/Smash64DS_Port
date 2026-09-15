#!/usr/bin/env python3
"""E01: offline alpha-isoband tessellation experiment, NOT wired into the game.

Clips a source triangle along lines of constant interpolated SHADE alpha instead
of assigning one average alpha to the whole triangle. Exact rational intersections
preserve the source plane, RGB, UVs and shared-edge intersection positions.

Only use after proving the effective source alpha reads SHADE and input RGB is
unlit color, not normal bytes. Object-space interpolation is not a proof of
source screen-space interpolation under varying clip W. This does not
replace texture alpha or simulate arbitrary combiners. Cross-matrix triangles
are rejected; blending, polygon IDs, depth writes and packet budgets remain the
native integration owner's job. No generated or decomp files are changed here.
"""
from __future__ import annotations
import argparse
from dataclasses import dataclass
from fractions import Fraction as F
import json
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class Vertex:
    xyz: tuple[F, F, F]
    st: tuple[F, F]
    rgba: tuple[F, F, F, F]
    matrix_binding: int = 0

    @classmethod
    def from_dict(cls, row: dict) -> 'Vertex':
        def values(name: str, count: int) -> tuple[F, ...]:
            result = tuple(F(str(x)) for x in row[name])
            if len(result) != count:
                raise ValueError(f'{name} must have {count} components')
            return result
        rgba = values('rgba', 4)
        if any(x < 0 or x > 255 for x in rgba):
            raise ValueError('RGBA outside 0..255')
        return cls(values('xyz', 3), values('st', 2), rgba,
                   int(row.get('matrix_binding', 0)))


@dataclass(frozen=True)
class Polygon:
    vertices: tuple[Vertex, Vertex, Vertex]
    polygon_alpha: int


def interpolate(a: Vertex, b: Vertex, t: F) -> Vertex:
    if a.matrix_binding != b.matrix_binding:
        raise ValueError('Cross-matrix interpolation requires an explicit transform owner')
    if not 0 <= t <= 1:
        raise ValueError('Interpolation outside source edge')
    def lerp(x: tuple, y: tuple) -> tuple:
        return tuple(xi + (yi-xi)*t for xi, yi in zip(x, y))
    return Vertex(lerp(a.xyz,b.xyz), lerp(a.st,b.st), lerp(a.rgba,b.rgba), a.matrix_binding)


def clip(poly: list[Vertex], limit: F, keep_above: bool) -> list[Vertex]:
    if not poly:
        return []
    def inside(v: Vertex) -> bool:
        return v.rgba[3] >= limit if keep_above else v.rgba[3] <= limit
    output: list[Vertex] = []
    prev = poly[-1]
    for cur in poly:
        prev_in, cur_in = inside(prev), inside(cur)
        if prev_in != cur_in:
            t = (limit-prev.rgba[3])/(cur.rgba[3]-prev.rgba[3])
            output.append(interpolate(prev,cur,t))
        if cur_in:
            output.append(cur)
        prev=cur
    # Clipping at an existing vertex can emit it twice. Keep order, remove
    # only exact adjacent duplicates, including the closing duplicate.
    result=[]
    for v in output:
        if not result or v != result[-1]:
            result.append(v)
    if len(result)>1 and result[0]==result[-1]:
        result.pop()
    return result


def cross(a: Vertex, b: Vertex, c: Vertex) -> tuple[F,F,F]:
    u=tuple(y-x for x,y in zip(a.xyz,b.xyz))
    v=tuple(y-x for x,y in zip(a.xyz,c.xyz))
    return (u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])


def tessellate(triangles: Iterable[tuple[Vertex,Vertex,Vertex]], *,
               levels: int = 8, max_output_triangles: int = 256) -> list[Polygon]:
    if not 2 <= levels <= 32:
        raise ValueError('levels must be in 2..32')
    if max_output_triangles <= 0:
        raise ValueError('A positive explicit polygon budget is required')
    output: list[Polygon] = []
    for tri in triangles:
        if len(tri)!=3:
            raise ValueError('Expected a triangle')
        if len({v.matrix_binding for v in tri}) != 1:
            raise ValueError('Cross-matrix triangle is not supported by E01')
        if any(x < 0 or x > 255 for v in tri for x in v.rgba):
            raise ValueError('RGBA outside 0..255')
        if cross(*tri)==(0,0,0):
            continue
        alphas=[v.rgba[3] for v in tri]
        if max(alphas)==0:
            # Exactly transparent source SHADE-alpha area has no pixels.
            continue
        bands=range(levels) if min(alphas)!=max(alphas) else [None]
        for band in bands:
            poly=list(tri)
            if band is not None:
                low,high=F(255*band,levels),F(255*(band+1),levels)
                poly=clip(clip(poly,low,True),high,False)
            if len(poly)<3:
                continue
            for i in range(1,len(poly)-1):
                small=(poly[0],poly[i],poly[i+1])
                if cross(*small)==(0,0,0):
                    continue
                mean=sum(v.rgba[3] for v in small)/3
                # Alpha 0 means wireframe on DS; never submit it as transparency.
                # Nonzero but sub-quantum source alpha rounds up to one quantum.
                scaled=mean*31/255
                alpha=max(1,min(31,int(scaled+F(1,2))))
                if len(output)>=max_output_triangles:
                    raise ValueError('AOT polygon budget exceeded; no partial output is returned')
                output.append(Polygon(small,alpha))
    return output


def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('input',type=Path,help='JSON list of triangles, each containing 3 vertex dictionaries')
    ap.add_argument('output',type=Path)
    ap.add_argument('--levels',type=int,default=8)
    ap.add_argument('--max-output-triangles',type=int,required=True)
    args=ap.parse_args()
    source=json.loads(args.input.read_text())
    rows=tessellate([tuple(Vertex.from_dict(v) for v in tri) for tri in source],
                    levels=args.levels,max_output_triangles=args.max_output_triangles)
    # Preserve exact fractions. Integration must quantize positions/UVs once,
    # deduplicate shared edges, and run the target range/capacity certificates.
    def encode(v: Vertex) -> dict:
        return {k:[str(x) for x in getattr(v,k)] for k in ('xyz','st','rgba')} | {'matrix_binding':v.matrix_binding}
    result={'experimental':True,'linked_into_rom':False,
            'triangles':[{'polygon_alpha':p.polygon_alpha,'vertices':[encode(v) for v in p.vertices]} for p in rows]}
    if args.input.resolve()==args.output.resolve():
        raise ValueError('Input and output must be different files')
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(f'{len(source)} source triangles -> {len(rows)} experimental triangles')
    return 0


if __name__=='__main__':
    try:
        raise SystemExit(main())
    except (OSError,KeyError,TypeError,ValueError) as exc:
        raise SystemExit(f'E01: {exc}')
