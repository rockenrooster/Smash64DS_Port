#!/usr/bin/env python3
"""Analytic geometry tests; no original game asset or DS rasterizer is used."""
from fractions import Fraction as F
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'experiments'))
from alpha_isobands import Vertex,cross,tessellate


def v(x,y,z,a,bind=0):
    return Vertex((F(x),F(y),F(z)),(F(x),F(z)),(F(255),F(255),F(255),F(a)),bind)


class AlphaTests(unittest.TestCase):
    def setUp(self):
        self.tri=(v(0,0,0,0),v(120,0,0,255),v(0,0,120,255))

    def test_plane_area_and_winding_preserved(self):
        polys=tessellate([self.tri],levels=8)
        orig=cross(*self.tri)
        sums=tuple(sum(cross(*p.vertices)[i] for p in polys) for i in range(3))
        self.assertEqual(sums,orig)
        for p in polys:
            self.assertTrue(all(a*b>=0 for a,b in zip(cross(*p.vertices),orig)))
            self.assertTrue(all(q.xyz[1]==0 for q in p.vertices))

    def test_uvs_remain_source_affine(self):
        for p in tessellate([self.tri]):
            for q in p.vertices:
                self.assertEqual(q.st,(q.xyz[0],q.xyz[2]))
                self.assertEqual(q.rgba[3],F(255,120)*(q.xyz[0]+q.xyz[2]))

    def test_never_emits_wireframe_and_has_multiple_levels(self):
        alphas={p.polygon_alpha for p in tessellate([self.tri])}
        self.assertGreater(len(alphas),4)
        self.assertTrue(all(1<=a<=31 for a in alphas))

    def test_exact_transparency_and_uniform_alpha(self):
        clear=tuple(v(q.xyz[0],q.xyz[1],q.xyz[2],0) for q in self.tri)
        solid=tuple(v(q.xyz[0],q.xyz[1],q.xyz[2],255) for q in self.tri)
        self.assertEqual(tessellate([clear]),[])
        self.assertEqual(len(tessellate([solid])),1)
        self.assertEqual(tessellate([solid])[0].polygon_alpha,31)

    def test_budget_exhaustion_and_invalid_cross_matrix_are_errors(self):
        with self.assertRaisesRegex(ValueError,'budget'):
            tessellate([self.tri],max_output_triangles=1)
        mixed=(self.tri[0],self.tri[1],v(0,0,120,255,1))
        with self.assertRaisesRegex(ValueError,'Cross-matrix'):
            tessellate([mixed])

    def test_reversed_winding_reverses_output_area(self):
        reverse=(self.tri[2],self.tri[1],self.tri[0])
        area=sum(cross(*p.vertices)[1] for p in tessellate([reverse]))
        self.assertEqual(area,-cross(*self.tri)[1])

    def test_shared_edges_have_identical_intersections(self):
        a,b=v(0,0,0,0),v(120,0,0,255)
        left=tessellate([(a,b,v(0,0,120,255))])
        right=tessellate([(b,a,v(0,0,-120,255))])
        def shared(polys):
            return {q.xyz for p in polys for q in p.vertices if q.xyz[2]==0}
        self.assertEqual(shared(left),shared(right))

    def test_bad_budget_levels_and_vertices(self):
        for level in (0,1,33):
            with self.assertRaises(ValueError): tessellate([self.tri],levels=level)
        with self.assertRaises(ValueError): tessellate([self.tri],max_output_triangles=0)
        with self.assertRaises(ValueError): Vertex.from_dict({'xyz':[0,0,0],'st':[0,0],'rgba':[255,0,0,256]})


if __name__=='__main__': unittest.main(verbosity=2)
