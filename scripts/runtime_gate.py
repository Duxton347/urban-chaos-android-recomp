#!/usr/bin/env python3
"""Honest gate report. --require-game fails while the actual title build is blocked."""
import argparse,json,pathlib,sys
R=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--require-game',action='store_true');a=p.parse_args()
state={'game_status':'BLOCKED','g1':'not validated','missing':['legitimate candidate Xbox Default.xbe','reviewed reproducible Urban Chaos configuration/ABI/hooks','documented license for Burner adaptations','native title integration and real PC execution evidence'],'historical_diagnostic':'not a port','synthetic_smoke':'does not qualify any game gate'}
print(json.dumps(state,indent=2));sys.exit(78 if a.require_game else 0)
