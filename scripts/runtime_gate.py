#!/usr/bin/env python3
"""Honest gate report. --require-game fails while the actual title build is blocked."""
import argparse,json,pathlib,sys
R=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--require-game',action='store_true');a=p.parse_args()
state={'game_status':'REQUIRES_REVERSE_ENGINEERING','g1':'deterministic generation and private Linux research execution; full title integration not qualified','missing':['qualified compatibility of supplied non-reference XBE variant','remaining HLE/ABI/callback and mission validation','Android Vulkan/SDL3 runtime and device tests'],'current_research':'original intro/title/menu/difficulty on Linux; recorded indexed draws submitted; XDK linkage corrected and real visibility reports completed; tree insertion stalls during scene loading; Windows optional','arm64':'baseline and continuation-aware guest archives compiled (129 AArch64 objects each); no linked Android game or APK','burner':'adaptations not incorporated; license unresolved','historical_diagnostic':'not a port','synthetic_smoke':'does not qualify any game gate'}
print(json.dumps(state,indent=2));sys.exit(78 if a.require_game else 0)
