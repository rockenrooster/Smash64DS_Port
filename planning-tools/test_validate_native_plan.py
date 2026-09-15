#!/usr/bin/env python3
"""Host tests of the plan validator only, not Smash64DS."""
from __future__ import annotations
import copy
import json
import tempfile
import shutil
import unittest
import itertools
from collections import Counter
from coverage_counts import count_cases
from pathlib import Path
from validate_native_plan import validate, validate_graph

ROOT=Path(__file__).resolve().parents[1]
DATA=json.loads((ROOT/'docs/p2/native-optimization/tasks.json').read_text())

class PlanValidatorTests(unittest.TestCase):
    def test_real_plan(self):
        errors,s=validate(ROOT)
        self.assertEqual(errors,[])
        self.assertEqual(s['tasks'],81)
        self.assertEqual(s['documents'],18)
    def test_duplicate_id(self):
        tasks=copy.deepcopy(DATA['tasks']);tasks.append(copy.deepcopy(tasks[0]))
        self.assertTrue(any('duplicate' in e for e in validate_graph(tasks)[0]))
    def test_unknown_dependency(self):
        tasks=copy.deepcopy(DATA['tasks']);tasks[0]['dependencies']=['N99.99']
        self.assertTrue(any('unknown dependency' in e for e in validate_graph(tasks)[0]))
    def test_cycle(self):
        tasks=copy.deepcopy(DATA['tasks']);tasks[0]['dependencies']=[tasks[-1]['id']]
        self.assertTrue(any('cycle' in e for e in validate_graph(tasks)[0]))
    def test_missing_completion(self):
        tasks=copy.deepcopy(DATA['tasks']);tasks[0]['completion']=''
        self.assertTrue(any('missing completion' in e for e in validate_graph(tasks)[0]))
    def test_static_status(self):
        tasks=copy.deepcopy(DATA['tasks']);tasks[0]['state']='FIXED'
        self.assertTrue(any('static package state' in e for e in validate_graph(tasks)[0]))
    def test_malformed_task(self):
        self.assertTrue(validate_graph([None])[0])
    def test_topological_order(self):
        errors,order=validate_graph(DATA['tasks']);self.assertFalse(errors)
        pos={tid:i for i,tid in enumerate(order)}
        for t in DATA['tasks']:
            for dep in t['dependencies']:self.assertLess(pos[dep],pos[t['id']])
    def test_broken_link(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp)
            shutil.copytree(ROOT/'docs',root/'docs')
            p=root/'docs/p2/native-optimization/README.md'
            p.write_text(p.read_text()+'\n[Missing](not-a-real-file.md)\n')
            self.assertTrue(any('broken relative link' in e for e in validate(root)[0]))
    def test_unknown_source(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);shutil.copytree(ROOT/'docs',root/'docs')
            p=root/'docs/p2/native-optimization/README.md'
            p.write_text(p.read_text()+'\nUnsupported source [S99].\n')
            self.assertTrue(any('unknown source S99' in e for e in validate(root)[0]))


class RevisionCoverageTests(unittest.TestCase):
    def mutate_file(self, filename, callback):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);shutil.copytree(ROOT/'docs',root/'docs')
            p=root/'docs/p2/native-optimization'/filename
            callback(p)
            return validate(root)[0]

    def contract_change(self, key, value):
        def change(p):
            d=json.loads(p.read_text());d[key]=value;p.write_text(json.dumps(d))
        return self.mutate_file('coverage-contract.json',change)

    def test_sampling_cannot_replace_base_release(self):
        self.assertTrue(self.contract_change('pairwise_replaces_release_base_matrix',True))
    def test_duplicate_fighters_required(self):
        self.assertTrue(self.contract_change('include_repeated_fighter_kinds',False))
    def test_legal_slots_required(self):
        self.assertTrue(self.contract_change('include_legal_slot_assignments',False))
    def test_resource_proof_not_timing_proof(self):
        self.assertTrue(self.contract_change('resource_equivalence_implies_timing_equivalence',True))
    def test_safe_rejection_not_pass(self):
        self.assertTrue(self.contract_change('safe_legal_admission_refusal','PASS'))
    def test_pooled_performance_forbidden(self):
        self.assertTrue(self.contract_change('performance_aggregation','POOLED_P95'))
    def test_missing_evidence_not_pass(self):
        self.assertTrue(self.contract_change('missing_evidence','PASS'))
    def test_required_content_cannot_shrink(self):
        self.assertTrue(self.contract_change('unfinished_required_content','EXCLUDE'))
    def test_no_all_history_claim(self):
        self.assertTrue(self.contract_change('claims_all_possible_input_histories_proven',True))
    def test_rare_overrun_allowance_preserved(self):
        self.assertTrue(self.contract_change('existing_rare_overrun_allowance_preserved',False))
    def test_count_example_wrong(self):
        obj=copy.deepcopy(json.loads((ROOT/'docs/p2/native-optimization/coverage-contract.json').read_text())['example_full_catalogue'])
        obj['base_cases']=12284
        self.assertTrue(self.contract_change('example_full_catalogue',obj))
    def test_missing_release_dependency(self):
        def change(p):
            d=json.loads(p.read_text())
            next(t for t in d['tasks'] if t['id']=='N10.02')['dependencies'].remove('N10.08')
            p.write_text(json.dumps(d))
        self.assertTrue(any('universal dependency' in e for e in self.mutate_file('tasks.json',change)))
    def test_task_card_completion_drift(self):
        def change(p):p.write_text(p.read_text().replace('**Done:**','**Old done:**',1))
        self.assertTrue(any('Markdown completion' in e for e in self.mutate_file('02_BASELINE_AND_GATES.md',change)))
    def test_csv_completion_drift(self):
        def change(p):p.write_text(p.read_text().replace('One reproducible identity','Altered identity',1))
        self.assertTrue(any('CSV completion' in e for e in self.mutate_file('tasks.csv',change)))
    def test_dependency_graph_drift(self):
        def change(p):p.write_text(p.read_text().replace('  N10_08 --> N10_02\n',''))
        self.assertTrue(any('graph edges' in e for e in self.mutate_file('dependency-graph.mmd',change)))
    def test_missing_coverage_negative_fixture(self):
        def change(p):p.write_text(p.read_text().replace('| COV18 |','| REMOVED |'))
        self.assertTrue(any('missing COV18' in e for e in self.mutate_file('16_ALL_ROSTERS_ALL_STAGES.md',change)))
    def test_template_cannot_claim_runtime_pass(self):
        def change(p):
            d=json.loads(p.read_text());d['status']='PASS';p.write_text(json.dumps(d))
        self.assertTrue(any('template cannot claim' in e for e in self.mutate_file('templates/scenario-result.json',change)))

class CoverageCountTests(unittest.TestCase):
    def test_full_catalogue(self):
        counts=count_cases(12,9)
        self.assertEqual((counts['base_cases'],counts['ordered_cases']),(12285,186624))
    def test_single_fighter_single_stage(self):
        counts=count_cases(1,1)
        self.assertEqual((counts['base_cases'],counts['ordered_cases']),(1,1))
    def test_invalid_counts(self):
        for f,s in [(0,9),(12,0),(-1,9),(12,-1),(True,9),(12,False),(1.5,9),(12,'9')]:
            with self.subTest(f=f,s=s):
                with self.assertRaises(ValueError):count_cases(f,s)
    def test_independent_small_enumeration(self):
        for f in range(1,8):
            bases=set(itertools.combinations_with_replacement(range(f),4))
            orders=set(itertools.product(range(f),repeat=4))
            self.assertEqual({tuple(sorted(o)) for o in orders},bases)
            c=count_cases(f,3)
            self.assertEqual(len(bases)*3,c['base_cases'])
            self.assertEqual(len(orders)*3,c['ordered_cases'])
    def test_full_ordered_maps_to_full_base(self):
        base=set(itertools.combinations_with_replacement(range(12),4))
        mapped={tuple(sorted(o)) for o in itertools.product(range(12),repeat=4)}
        self.assertEqual(mapped,base)
        self.assertEqual(len(base),1365)
    def test_multiplicity_independent_counts(self):
        label={(4,):'AAAA',(3,1):'AAAB',(2,2):'AABB',(2,1,1):'AABC',(1,1,1,1):'ABCD'}
        observed=Counter()
        for r in itertools.combinations_with_replacement(range(12),4):
            observed[label[tuple(sorted(Counter(r).values(),reverse=True))]]+=1
        got=count_cases(12,9)['multiplicity_classes']
        self.assertEqual(dict(observed),{k:v['base_rosters'] for k,v in got.items()})
    def test_counts_make_no_runtime_claim(self):
        self.assertIn('NOT_RUNTIME_QUALIFICATION',count_cases(12,9)['nature'])

if __name__=='__main__':unittest.main(verbosity=2)
