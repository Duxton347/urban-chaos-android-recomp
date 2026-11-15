# Urban Chaos Android Recomp

Port nativo Android ARM64 de **Urban Chaos: Riot Response (Xbox original)** por recompilação estática. A meta é preservar o jogo original; o alvo prioritário é Xiaomi Pad 6 / HyperOS 2.0.16.0.

**Estado: auditoria e ferramentas funcionais; jogo bloqueado em G1. Não existe APK jogável nesta entrega.** O diagnóstico da fase anterior está preservado em `android/` como infraestrutura histórica e não representa o port.

## Primeira entrega e rota

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

Os seis workflows em `.github/workflows/` separam auditoria, ferramentas/runtime smoke, compilação ARM64, testes, artefatos de desenvolvimento e gate de releases. Actions são fixadas por SHA, ferramentas por versão/checksum e caches contêm apenas dependências públicas. O gate de release falha enquanto o jogo não estiver qualificado. `AGENTS.md` orienta mudanças futuras.

## Próximo bloqueio

Não foi fornecido um XBE Xbox nesta sessão. A Burner publicada ainda depende de configuração/unidades/fixtures privados; o XboxKit não recupera essa geração. Reuso das adaptações Burner também precisa de licença aplicável. O menor caminho é qualificar o executável legítimo e a geração original, executar no PC da nuvem e depois adaptar o runtime Android com as partes compatíveis e autorizadas de Black.

Não são exigidos PC local nem upload de jogo pelo chat. Futuras entradas legítimas ficam em volume privado na nuvem ou são selecionadas no tablet via SAF. `private/`, `generated/`, imagens, executáveis, saves e keystores nunca entram no Git ou nos artefatos públicos. Não serão usados executáveis criptografados de projetos de referência.

Importadores e inspetor existentes:

```bash
python3 tools/inspect_xbe.py private/Default.xbe
python3 tools/prepare_game.py private/own-disc.iso private/imported --require-candidate
python3 tools/recompile_xbe.py private/Default.xbe --config private/config/urban-chaos.toml --out build/title-attempt-001
```

O último comando só é aplicável após obter os dados legítimos e revisar a configuração. Importação ou hash correto não significam compatibilidade Android. O wrapper não aprova G1 automaticamente.

Licenças/avisos das dependências reutilizadas ficam em `licenses/`. Burner/Black/XboxKit foram inspecionados como referências; adaptações Burner e C# XboxKit não foram incorporados. A primeira entrega não inclui ISO, XBE, assets comerciais, código gerado do título ou APK do jogo.
