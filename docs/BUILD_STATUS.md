# Build status — nova rota de recompilação

Atualizado em 2026-11-15. **GO para ferramentas e auditoria. BLOCKED para jogo original (G1). Nenhum novo APK do jogo foi criado.**

## Primeira entrega

Estrutura local, `AGENTS.md`, os seis documentos solicitados e seis workflows iniciais estão preparados. Destino confirmado: `Duxton347/urban-chaos-android-recomp`, público, leitura/escrita verificadas após criação pelo usuário. Nenhum repositório alternativo criado. A integração inicialmente recusou `createRepository`; o usuário criou o destino exato e o acesso foi revalidado antes da publicação. Status de publicação/CI será registrado em [PUBLICATION.md](PUBLICATION.md).

## Auditoria de quatro referências

| Projeto | Revisão | Resultado / licença |
|---|---|---|
| Burner | `bdaef722ef1d589af30e5a58dd8543f64564c4fc` | Código público de patches/host; geração/configuração/fixtures privados ausentes. `COPYRIGHT` não concede licença geral explícita às adaptações. Não reutilizadas na implementação. |
| XboxXBERecomp | `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` | MIT; scanner/recompilador Linux compilados. Runtime completo do título não construído. |
| Black Android | `8e9c6fc7d570503ea68d039cff16c09339ee6d30` | MIT no projeto, assets/terceiros exigem revisão. Android CMake/SDL/Vulkan/áudio/input/HUD e histórico inspecionados; nenhum hook/endereço Black transplantado. |
| XboxKit | `e8c57ca4d8e15fba27c0d1d2f6a21d56b1f38819` | MIT declarado nos `.csproj`, texto LICENSE ausente. .NET/C# com NuGet; referência Redump/XDVDFS, não recompilador. Não compilado nem incorporado nesta sessão. |

Pins, URLs, árvores e dependências no `dependencies.lock.json`. O SDK observado em `cc9edda...` mudou o modelo para DLL/compilação no primeiro início; mantidos os pins originais. Entre os pins Burner/Black há nove caminhos diferentes, incluindo flags de áudio em `kernel/xboxkrnl.cpp`. Contexto CPU, arquivos de memória e interfaces HLE comparados são idênticos; isso não prova compatibilidade semântica. [Matriz](COMPATIBILITY_MATRIX.md).

## Comandos e resultados desta rota

| Comando | Resultado real | Evidência |
|---|---|---|
| `scripts/build_host.sh` | Scanner e recompilador públicos compilados com GCC/C++20; SDK não modificado | [host build](evidence/route-v2/host-build.log) |
| `scripts/test.sh` | 21 testes Python e 11 checks Java com dados sintéticos | [tests.log](evidence/route-v2/tests.log) |
| `python3 scripts/native_smoke.py` | Gera 1 função/2 instruções sintéticas; compila saída e executa nativamente no Linux; EAX=42, ESP+4 | [native-smoke.log](evidence/route-v2/native-smoke.log) |
| `scripts/build_portability.sh` | Contrato CPU do SDK compila em ELF64 AArch64 com NDK fixo | [android-portability.log](evidence/route-v2/android-portability.log) |
| `python3 scripts/audit_references.py` | Pins, fontes e ausências auditados | [reference-audit.json](evidence/route-v2/reference-audit.json) |
| `actionlint` 1.7.7 | Validação dos seis workflows; ver resultado final | [actionlint.log](evidence/route-v2/actionlint.log) |
| `scripts/runtime_gate.py --require-game` | Exit 78, BLOCKED intencional; nenhum check fabricado | [gate.log](evidence/route-v2/gate.log) |

**Correção de evidência antiga:** o teste determinístico anterior aceitava arquivos gerados sem funções porque faltava `code_sections`. A nova validação exige função emitida e o smoke executa o C++ gerado. A [falha inicial](evidence/route-v2/native-smoke-initial-failure.log) foi preservada; não foi tratada como sucesso. Isso corrige a força da evidência de infraestrutura, sem alterar o bloqueio do título.

## Bloqueios e menor caminho viável

1. Falta XBE Xbox legítimo: não há disco comercial nem executável no workspace. Sem esse arquivo não se executa scanner do título nem se conhece o conjunto real de símbolos/ABI faltantes. Não solicitar upload pelo chat; usar volume privado na nuvem quando disponível.
2. O build Burner parte de área privada qualificada. Não há código gerado nem configuração completa pública do Urban Chaos que permita reproduzi-la nesta sessão. Recipes de algumas fixtures existem, mas requerem assets locais. [Entradas e consumidores](MISSING_INPUTS.md).
3. Reutilizar patches Burner exige licença aplicável. A intenção de reuso não esclarece direitos automaticamente. Não contactar terceiros sem instrução explícita.
4. Primeiro executar geração limpa e determinística e boot/menu PC, preferencialmente com backend de referência em runner privado. Windows PC na nuvem pode ser menor caminho para primeiro boot; Proton não é nativo Linux.
5. Depois adaptar backend Android compatível, sem transplante de endereços Black. Vulkan/áudio/memória/callbacks, missão, controles e saves precisam de evidência real.

XboxKit reduz incerteza sobre formatos de disco e offsets; não remove os bloqueios 1–3. Não há dados suficientes para afirmar que todas as entradas privadas podem ser recuperadas só com ferramentas públicas.

## Estado separado

| Item | Compila | Executa | Menu original | Missão | Campanha |
|---|---|---|---|---|---|
| Ferramentas host | Sim | CLI e fixture sintético testados | Não se aplica | Não se aplica | Não se aplica |
| Contrato CPU AArch64 | Sim | Não executado em dispositivo | Não se aplica | Não se aplica | Não se aplica |
| Diagnóstico histórico | Build anterior registrado | Tablet não testado | Não é jogo | Não é jogo | Não é jogo |
| Urban Chaos PC/Android | Não demonstrado | Não demonstrado | Não demonstrado | Não demonstrado | Não demonstrado |

## CI inicial

Audit/static checks, host/native smoke, Android ARM64 contract, testes automatizados, artefatos de tooling sob allowlist e gate de release versionada. Cache limitado a dependências públicas. Nenhum job solicita ISO, segredos de jogo ou keystore. O workflow release falha fechado enquanto os gates estão incompletos; o workflow Android não fabrica APK diagnóstico como resultado do port. A configuração inicial ainda não implementa um build completo de runtime do jogo; isso está explicitamente bloqueado, não concluído.
