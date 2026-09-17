#!/usr/bin/env python3
"""Validate this STATIC planning package, not the game or its performance.

Usage: python planning-tools/validate_native_plan.py [--root REPOSITORY_ROOT]
Python 3.10+, standard library only. Does not fetch files, modify the plan, or
invoke any game tools. Nonzero exit indicates inconsistent planning material.
"""
from __future__ import annotations
import argparse
import csv
import json
import re
import sys
from pathlib import Path
from urllib.parse import unquote
from coverage_counts import count_cases

TASK_RE = re.compile(r'^N\d{2}\.\d{2}$')
CARD_RE = re.compile(r'^### (N\d{2}\.\d{2}) — ', re.M)
SOURCE_RE = re.compile(r'\b(?:S\d{2}[A-Z]?|H\d{2})\b')
LINK_RE = re.compile(r'\[[^\]\n]+\]\(([^)\n]+)\)')
REQUIRED_TEXT = ('title','package','validation','work_to_retire','completion','stop_or_rollback')


def validate_graph(tasks: list[dict]) -> tuple[list[str], list[str]]:
    """Return errors and a stable topological order. Handles malformed input."""
    errors: list[str] = []
    by_id: dict[str,dict] = {}
    for pos,t in enumerate(tasks):
        if not isinstance(t,dict):
            errors.append(f'task[{pos}] must be an object'); continue
        tid=t.get('id')
        if not isinstance(tid,str) or not TASK_RE.fullmatch(tid):
            errors.append(f'task[{pos}] has invalid id {tid!r}');continue
        if tid in by_id: errors.append(f'duplicate task id: {tid}');continue
        by_id[tid]=t
        for field in REQUIRED_TEXT:
            if not isinstance(t.get(field),str) or not t[field].strip():
                errors.append(f'{tid}: missing {field}')
        for field in ('source_or_edit_targets','implementation_steps'):
            value=t.get(field)
            if not isinstance(value,list) or not value or not all(isinstance(x,str) and x.strip() for x in value):
                errors.append(f'{tid}: invalid {field}')
        if not isinstance(t.get('dependencies'),list) or not all(isinstance(x,str) for x in t.get('dependencies',[])):
            errors.append(f'{tid}: invalid dependencies')
        if t.get('state')!='PLANNED':
            errors.append(f'{tid}: static package state must remain PLANNED; use existing project board for live status')
        if t.get('parent')!='P2-2p8': errors.append(f'{tid}: wrong parent')
    for tid,t in by_id.items():
        for dep in t.get('dependencies',[]) if isinstance(t.get('dependencies'),list) else []:
            if not isinstance(dep,str) or dep not in by_id: errors.append(f'{tid}: unknown dependency {dep!r}')
    seen: dict[str,int]={}; order: list[str]=[]
    def visit(tid: str, stack: list[str]) -> None:
        if seen.get(tid)==2: return
        if seen.get(tid)==1:
            errors.append('dependency cycle: '+' -> '.join(stack+[tid]));return
        seen[tid]=1
        deps=by_id[tid].get('dependencies',[])
        if isinstance(deps,list):
            for dep in deps:
                if isinstance(dep,str) and dep in by_id: visit(dep,stack+[tid])
        seen[tid]=2;order.append(tid)
    for tid in sorted(by_id):visit(tid,[])
    return errors,order



def validate_coverage_contract(d: Path, tasks: list[dict]) -> list[str]:
    """Check static universal-scope promises, NOT evidence of runtime support."""
    errors: list[str] = []
    try:
        c=json.loads((d/'coverage-contract.json').read_text(encoding='utf-8'))
        if not isinstance(c,dict):return ['coverage contract must be an object']
    except (OSError,ValueError) as exc:return [f'coverage contract: {exc}']
    expected = {
        'status':'PLANNED',
        'support':'EVERY_LEGAL_FOUR_FIGHTER_LINEUP_ON_EVERY_SELECTABLE_VS_STAGE',
        'source_catalogue_required':True,
        'unfinished_required_content':'BLOCKED_NOT_EXCLUDED',
        'include_repeated_fighter_kinds':True,
        'include_legal_slot_assignments':True,
        'release_profile':'RELEASE_EXHAUSTIVE',
        'release_base_requirement':'FULL_SCORED_SOURCE_NORMAL_BATTLE_EVERY_BASE_CASE',
        'slot_reuse':'PROPERTY_SCOPED_EXPLICIT_PROOF_OR_EXECUTE',
        'resource_equivalence_implies_timing_equivalence':False,
        'performance_aggregation':'PER_REQUIRED_RUN_AND_CASE_THEN_LOGICAL_AND',
        'missing_evidence':'UNKNOWN_NOT_PASS',
        'safe_legal_admission_refusal':'FAIL_NOT_PASS',
        'known_legal_failure_blocks_universal_closure':True,
        'pairwise_replaces_release_base_matrix':False,
        'claims_all_possible_input_histories_proven':False,
        'existing_rare_overrun_allowance_preserved':True,
    }
    for key,value in expected.items():
        if type(c.get(key)) is not type(value) or c.get(key)!=value:
            errors.append(f'coverage contract: invalid {key}')
    example=c.get('example_full_catalogue',{})
    try:
        result=count_cases(example.get('fighters'),example.get('stages'))
        for key in ('base_cases','ordered_cases'):
            if type(example.get(key)) is not int or example[key]!=result[key]:
                errors.append(f'coverage contract: wrong example {key}')
        if example.get('is_runtime_coverage') is not False:
            errors.append('coverage contract: count example cannot claim runtime coverage')
    except (AttributeError,ValueError,TypeError) as exc:
        errors.append(f'coverage contract: bad count example: {exc}')
    required={'N00.06','N02.07','N03.11','N10.01','N10.02','N10.07','N10.08','N10.09'}
    if set(c.get('required_scope_tasks',[]))!=required:
        errors.append('coverage contract: missing scope task declarations')
    by={t['id']:t for t in tasks if isinstance(t,dict) and isinstance(t.get('id'),str)}
    for tid in required:
        if tid not in by:errors.append(f'coverage contract: missing task {tid}')
    required_edges={
        'N02.01':{'N00.06'}, 'N02.06':{'N02.07'},
        'N03.09':{'N03.11'}, 'N10.01':{'N00.06'},
        'N10.02':{'N10.07','N10.08','N10.09'},
        'N10.05':{'N10.08'},
    }
    for tid,needed in required_edges.items():
        if not needed.issubset(set(by.get(tid,{}).get('dependencies',[]))):
            errors.append(f'coverage contract: missing universal dependency for {tid}')
    try:
        spec=(d/'16_ALL_ROSTERS_ALL_STAGES.md').read_text(encoding='utf-8')
        for i in range(1,19):
            if not re.search(r'\| COV'+f'{i:02d}'+r' \|',spec):
                errors.append(f'coverage specification: missing COV{i:02d} fixture')
    except OSError as exc:errors.append(f'coverage specification: {exc}')
    return errors

def validate(root: Path) -> tuple[list[str],dict]:
    errors: list[str]=[]
    d=root/'docs/p2/native-optimization'
    try: payload=json.loads((d/'tasks.json').read_text(encoding='utf-8'))
    except (OSError,ValueError) as exc:return [f'cannot read tasks.json: {exc}'],{}
    tasks=payload.get('tasks')
    if not isinstance(tasks,list):return ['tasks must be a list'],{}
    ge,order=validate_graph(tasks); errors.extend(ge)
    errors.extend(validate_coverage_contract(d,tasks))
    if payload.get('task_count')!=len(tasks):errors.append('task_count disagrees with tasks')
    md_files=sorted(d.glob('*.md'))
    card_count: dict[str,int]={}
    cards: dict[str,str]={}
    try:index=(d/'15_SOURCE_INDEX.md').read_text(encoding='utf-8')
    except OSError as exc:return errors+[str(exc)],{}
    known_sources=set(re.findall(r'^## (S\d{2}[A-Z]?|H\d{2}) — ',index,re.M))
    for path in md_files:
        text=path.read_text(encoding='utf-8')
        if text.count('```')%2:errors.append(f'{path.name}: unbalanced code fences')
        for tid in CARD_RE.findall(text):card_count[tid]=card_count.get(tid,0)+1
        for m in re.finditer(r'^### (N\d{2}\.\d{2}) — [^\n]*\n.*?(?=^#{1,3} |\Z)',text,re.M|re.S):
            cards[m[1]]=m[0]
        for ref in set(SOURCE_RE.findall(text)):
            if ref not in known_sources:errors.append(f'{path.name}: unknown source {ref}')
        for link in LINK_RE.findall(text):
            if '://' in link or link.startswith(('mailto:','#')):continue
            target=unquote(link.split('#',1)[0])
            if target and not (path.parent/target).exists():errors.append(f'{path.name}: broken relative link {link}')
    task_ids={t['id'] for t in tasks if isinstance(t,dict) and isinstance(t.get('id'),str)}
    for tid in task_ids:
        if card_count.get(tid,0)!=1:errors.append(f'{tid}: expected one Markdown task card, got {card_count.get(tid,0)}')
    for tid in card_count:
        if tid not in task_ids:errors.append(f'unknown Markdown task card: {tid}')
    for t in tasks:
        if not isinstance(t,dict) or t.get('id') not in cards:continue
        body=cards[t['id']]
        checks={
            'title':f"### {t['id']} — {t.get('title','')}\n",
            'dependencies':'**Depends on:** '+(', '.join(t.get('dependencies',[])) if t.get('dependencies') else 'None')+'\n',
            'validation':'**Required tests/evidence:** '+t.get('validation',''),
            'completion':'**Done:** '+t.get('completion',''),
            'work_to_retire':'**Work or dependency retired:** '+t.get('work_to_retire',''),
            'stop_or_rollback':'**Stop/revert:** '+t.get('stop_or_rollback',''),
        }
        for key,content in checks.items():
            if content not in body:errors.append(f"{t['id']}: Markdown {key} disagrees with JSON")
        for i,content in enumerate(t.get('implementation_steps',[]),1):
            if f'{i}. {content}' not in body:errors.append(f"{t['id']}: Markdown step {i} disagrees with JSON")
    try:
        with (d/'tasks.csv').open(newline='',encoding='utf-8') as f:rows=list(csv.DictReader(f))
        if [r.get('id') for r in rows]!=[t.get('id') for t in tasks if isinstance(t,dict)]:errors.append('CSV task IDs/order disagree with JSON')
        by={t['id']:t for t in tasks if isinstance(t,dict) and isinstance(t.get('id'),str)}
        for r in rows:
            t=by.get(r.get('id',''))
            if t and r.get('dependencies')!='; '.join(t.get('dependencies',[])):errors.append(f"{t['id']}: CSV dependencies disagree")
            if t:
                for field in ('package','title','optional','state','completion','work_to_retire','stop_or_rollback'):
                    if r.get(field)!=str(t.get(field)):errors.append(f"{t['id']}: CSV {field} disagrees")
    except (OSError,ValueError,TypeError) as exc:errors.append(f'CSV error: {exc}')
    for path in sorted((d/'templates').glob('*.json')):
        try:
            value=json.loads(path.read_text(encoding='utf-8'))
            if value.get('status') not in ('UNMEASURED','PLANNED'):errors.append(f'{path.name}: template cannot claim a result')
        except (OSError,ValueError) as exc:errors.append(f'{path.name}: {exc}')
    try:
        order_file=(d/'dependency-order.txt').read_text(encoding='utf-8')
        actual=re.findall(r'^(N\d{2}\.\d{2}) \| ',order_file,re.M)
        if actual!=order:errors.append('dependency-order.txt is stale or inconsistent')
        graph=(d/'dependency-graph.mmd').read_text(encoding='utf-8')
        graph_nodes=set(re.findall(r'^  (N\d{2}_\d{2})\[',graph,re.M))
        expected_nodes={tid.replace('.','_') for tid in task_ids}
        if graph_nodes!=expected_nodes:errors.append('dependency graph nodes disagree')
        graph_edges=set(re.findall(r'^  (N\d{2}_\d{2}) --> (N\d{2}_\d{2})$',graph,re.M))
        expected_edges={(dep.replace('.','_'),t['id'].replace('.','_')) for t in tasks if isinstance(t,dict) for dep in t.get('dependencies',[]) if isinstance(dep,str)}
        if graph_edges!=expected_edges:errors.append('dependency graph edges disagree')
    except OSError as exc:errors.append(f'dependency view error: {exc}')
    return errors,{'documents':len(md_files),'tasks':len(task_ids),'packages':len({t.get('package') for t in tasks if isinstance(t,dict)}),'source_entries':len(known_sources),'topological_order':order}


def main() -> int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--root',type=Path,default=Path(__file__).resolve().parents[1])
    args=ap.parse_args()
    errors,summary=validate(args.root.resolve())
    if errors:
        for e in errors:print('ERROR:',e,file=sys.stderr)
        return 1
    print(f"PASS: {summary['documents']} planning documents; {summary['tasks']} task cards; {summary['packages']} packages; {summary['source_entries']} source entries.")
    print('Dependency graph acyclic; task references/CSV/Markdown links/templates consistent.')
    print('This validates the plan only. No game build, correctness test, or benchmark ran.')
    return 0

if __name__=='__main__':raise SystemExit(main())
