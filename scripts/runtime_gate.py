#!/usr/bin/env python3
"""Honest gate report. --require-game fails while the actual title build is blocked."""
import argparse,json,pathlib,sys
R=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--require-game',action='store_true');a=p.parse_args()
state={'game_status':'BLOCKED','g1':'raw generation/compile succeeded; original Burner integration not validated','missing':['qualified compatibility of the supplied XBE variant (reference hash differs)','reviewed reproducible Urban Chaos configuration/ABI/hooks','documented license for Burner adaptations','D3D device/shader integration and verified native boot/menu'],'current_research':'experimental audio references pass; next SIGSEGV in D3D 0x003E7F70; Windows optional','historical_diagnostic':'not a port','synthetic_smoke':'does not qualify any game gate'}
print(json.dumps(state,indent=2));sys.exit(78 if a.require_game else 0)
