# Urban Chaos Android Recomp

## Mission and scope

Port the original Xbox Urban Chaos: Riot Response to Android ARM64 by static recompilation. Preserve original logic and content. Do not implement a replacement game, a PS2 emulation solution or a visual simulation presented as gameplay. The target is Xiaomi Pad 6 / HyperOS 2.0.16.0; device behavior requires measured tests.

User-facing delivery is an ISO-based Android launcher with the real recompiled runtime, matching the Black workflow: select/import a user disc, identify its XBE revision, mount read-only and launch. APK is the Android installation format; it does not bundle original disc assets or imply on-device recompilation. Prioritize direct ISO mounting over mandatory full extraction. Preserve the existing runtime work and keep experimental revision warnings distinct from validated compatibility. See docs/LAUNCHER_DESIGN.md.

## Evidence and gates

Read docs/BUILD_STATUS.md and docs/ARCHITECTURE.md before changes. Keep distinct: tools compile; generated synthetic fixture executes; game compiles; game boots; original menu; playable mission; campaign verified. Never equate an APK or launcher with a working game. The historical android/ module is diagnostic-only and is not the port. Do not expand diagnostic UI as a substitute for recompilation.

Pin every reused revision in dependencies.lock.json. Preserve vendor sources and notices. Burner title adaptations have no clarified general license in the pinned source: inspect them, record seams, and do not distribute/adapt their implementation until applicable permission is documented. Do not infer compatible ABI from the existence of similar code in Black. Never reuse Black title addresses, game hooks or encrypted executables.

## Data and publication

No commercial ISO, XBE, generated title units, derived fixtures, game saves, encrypted copies, credentials or signing keys in Git, public logs, caches or artifacts. Private game work occurs only in ignored private/ and build/ or an owner-controlled cloud runner outside public PRs. Users import their own legitimate data through SAF or an owner-controlled cloud volume; never require uploading game files through chat. Do not execute game-data jobs from untrusted pull requests.

Publish only to Duxton347/urban-chaos-android-recomp after confirming access. Do not create substitute repositories. Use small commits and PRs; do not merge merely to make checks green. Keep logs honest and record failing commands. Public artifact publication uses explicit allowlists, not directory wildcards.

## Validation

Run python3 -m unittest discover -s tests -v for importer/inspector changes; build the real host tools for recompilation checks. scripts/native_smoke.py is a synthetic machine-code fixture test, not a game result. scripts/build_portability.sh compiles CPU contracts for AArch64, not a game runtime. Run scripts/publication_check.py before publishing. Workflows use pinned actions and toolchains. Release jobs must fail closed while the game gates are incomplete.

Do not introduce full-device storage permission. Keep game, saves, configuration, shader cache and logs separate. Action IDs and guest input mappings need source evidence before adding game HUD buttons. Do not promise frame rate or resolution without tablet measurements. Ask for delegation only when explicitly requested; no autonomous sub-agent spawning.
