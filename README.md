# Urban Chaos Android Recomp

Entrega definida: **launcher Android para importar e executar a ISO original**, com runtime recompilado e recursos do disco externos. [Como o Black faz isso e como será aplicado aqui](docs/LAUNCHER_DESIGN.md).


Port nativo Android ARM64 de **Urban Chaos: Riot Response (Xbox original)** por recompilação estática. A meta é preservar o jogo original; o alvo prioritário é Xiaomi Pad 6 / HyperOS 2.0.16.0.

**Estado: menu e entrada lenta na missão relatados pelo proprietário com 0.3; 0.4 voltou a não abrir a missão no teste enviado. O candidato privado 0.5 corrige a cobertura das legendas da abertura e acrescenta cache opcional e diagnóstico. Biblioteca ARM64 e APK compilados/assinados; missão jogável e campanha ainda não verificadas.** [Investigação 0.5 e limites](docs/LOADING_05_INVESTIGATION.md). [Primeiro teste](docs/FIRST_TABLET_TEST.md) · [Receita Android](docs/ANDROID_BUILD.md). O diagnóstico histórico em `android/` permanece separado.

## Primeira entrega e rota

- [Progresso do runtime: menu original, correções e testes](docs/RUNTIME_PROGRESS.md)
- [Reaproveitamento PC](docs/PC_REUSE_MATRIX.md) e [contratos de runtime e receita de testes](docs/RUNTIME_CONTRACTS.md)
- [Retornos não locais e callbacks do recompilador](docs/CONTEXT_SWITCHING.md)
- [Investigação de áudio, testes posteriores e rota sem Windows obrigatório](docs/AUDIO_INVESTIGATION.md)
- [Imagem fornecida: extração, geração e falha de execução](docs/USER_IMAGE_INSPECTION.md)
- [Recuperação da compilação PC: tentativas e resultados](docs/PC_BUILD_RECOVERY.md)
- [Build status e evidências](docs/BUILD_STATUS.md)
- [Arquitetura](docs/ARCHITECTURE.md)
- [Recompilação e menor caminho para primeiro executável](docs/RECOMPILATION.md)
- [Disco: Redump, XISO, pasta, hash, Title ID e versão](docs/DISC_IMPORT.md)
- [Android: runtime, Vulkan, áudio, lifecycle e saves](docs/ANDROID_PORT.md)
- [HUD: ações reais, multitouch e editor](docs/HUD_SPEC.md)
- [Publicação e CI](docs/PUBLICATION.md)

Referências obrigatórias auditadas: [Burner](https://github.com/linkssy2/UrbanChaosRiotResponseBurnerEdition), [XboxXBERecomp](https://github.com/TGP482/XboxXBERecomp), [Black Android](https://github.com/RanggaCasper/Black-Android) e [XboxKit](https://github.com/Deterous/XboxKit). Commits, hashes e papéis estão em `dependencies.lock.json`.

## Verificação na nuvem

Linux x86_64 com Python 3.12+, Git, Make e compilador C++20:

```bash
python3 scripts/ci_bootstrap.py --component audit
python3 scripts/audit_references.py
python3 scripts/ci_bootstrap.py --component host
scripts/build_host.sh
python3 -m unittest discover -s tests -v
python3 scripts/native_smoke.py
python3 scripts/publication_check.py
```

O smoke gera e executa somente uma função x86 **sintética**; não é jogo ou boot. Para os checks Java, usar JDK 17 e `scripts/test.sh`. Para o contrato Android, instalar NDK `28.2.13676358` e CMake `3.31.6` no SDK local da nuvem, exportar `ANDROID_HOME` e executar `scripts/build_portability.sh`. Não produz APK.

Os workflows em `.github/workflows/` separam auditoria, ferramentas/runtime smoke, compilação ARM64, testes, artefatos de desenvolvimento e gate de releases. Actions são fixadas por SHA, ferramentas por versão/checksum e caches contêm apenas dependências públicas. A tentativa de reprodução PC executa o builder original no Windows e preserva falhas. O gate de release falha enquanto o jogo não estiver qualificado. `AGENTS.md` orienta mudanças futuras.

## Próximo bloqueio

A execução chegou aos menus e à primeira cena da abertura de Hope Street. A investigação segue no carregamento da primeira missão: foram corrigidos retornos não locais, chamadas de áudio, linkage de buffers XDK e reports reais de visibilidade. A corrupção da árvore foi rastreada até ownership de superfícies e corrigida; cone e fatores espaciais também receberam integração. [Evidências e limitações](docs/RUNTIME_PROGRESS.md). Ainda faltam missão jogável, fidelidade de áudio, save/load, backend Android e testes no tablet. O hash da variante difere da referência Burner; sua integração privada original continua não reproduzida, e as adaptações próprias da Burner não foram incorporadas sem licença esclarecida.

Não são exigidos PC local nem upload de jogo pelo chat. Futuras entradas legítimas ficam em volume privado na nuvem ou são selecionadas no tablet via SAF. `private/`, `generated/`, imagens, executáveis, saves e keystores nunca entram no Git ou nos artefatos públicos. Não serão usados executáveis criptografados de projetos de referência.

Importadores e inspetor existentes:

```bash
python3 tools/inspect_xbe.py private/Default.xbe
python3 tools/prepare_game.py private/own-disc.iso private/imported --require-candidate
python3 tools/recompile_xbe.py private/Default.xbe --config private/config/urban-chaos.toml --out build/title-attempt-001
```

O último comando só é aplicável após obter os dados legítimos e revisar a configuração. Importação ou hash correto não significam compatibilidade Android. O wrapper não aprova G1 automaticamente.

Licenças/avisos das dependências reutilizadas ficam em `licenses/`. Burner/Black/XboxKit foram inspecionados como referências; adaptações Burner e C# XboxKit não foram incorporados. A primeira entrega não inclui ISO, XBE, assets comerciais, código gerado do título ou APK do jogo.
