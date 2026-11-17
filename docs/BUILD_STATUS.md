# Build status — nova rota de recompilação

Atualizado em 2026-11-17. **GO: geração determinística, compilação Linux e baseline ARM64; intro, menus e primeira cena da abertura de Hope Street renderizados no host Linux de pesquisa. REQUIRES_REVERSE_ENGINEERING: gameplay, fidelidade gráfica/sonora, saves, APIs restantes e integração Android. Nenhum APK do jogo foi criado.**

[Estado atual e regressões](RUNTIME_PROGRESS.md) · [Trailer XDK](GPU_LINKAGE.md) · [Reports de visibilidade e novo bloqueio](REFERENCE_VISIBILITY.md) · [Correção de contexto/callbacks](CONTEXT_SWITCHING.md). Os relatos abaixo de falhas anteriores são históricos e mostram a progressão, não o ponto atual.

## Decisão de entrega: launcher baseado na ISO

O usuário definiu seleção/importação da ISO, reconhecimento da revisão e execução pelo launcher, seguindo o Black. O README e o código do HEAD Black foram rastreados: o aplicativo contém libmain.so ARM64 recompilada antecipadamente, importa os recursos externos e monta a imagem. [Decisão e pontos de integração](LAUNCHER_DESIGN.md). Essa escolha aproveita o runtime atual e não declara Vulkan/SDL3 ou o launcher Android prontos. Nenhum novo APK foi produzido nesta revisão.

## Histórico: investigação posterior da falha de áudio

[Diagnóstico e testes progressivos](AUDIO_INVESTIGATION.md): incompatibilidade do objeto DirectSound confirmada. Patch MIT experimental de referências passa AddRef e um probe de oito chamadas HLE. `AudioData.bin` e `Movies/dolby.xmv` foram extraídos da própria ISO e tiveram hashes verificados. O próximo SIGSEGV está em D3D `0x003E7F70`, com ponteiro de vertex shader zero; ele se repete com o vídeo disponível. Não é a única falha, nem falta de outro pacote. Windows não é requisito da compilação Android; o host Linux continua sendo a rota de depuração.

## Histórico: executável disponível e código compilado

[Inspeção da imagem fornecida e execução](USER_IMAGE_INSPECTION.md): 29.049 funções geradas duas vezes com saídas idênticas; 128 partições C++ e mapping compilados no Linux x86_64. Host de pesquisa ligado e executado com harness SDK: inicialização D3D e shaders observada, depois SIGSEGV em DSOUND `0x00423B41` com 79 recursos LST reais verificados. Nenhum frame/menu, missão, Windows ou Android validado. Hash da variante `c6f0c99a…044d1` difere da referência Burner; G1 qualificado permanece aberto. As ausências do título descritas na tentativa PC anterior são históricas, não indisponibilidade atual do XBE.

## Tentativa de recuperação PC executada

Ver [PC_BUILD_RECOVERY.md](PC_BUILD_RECOVERY.md): inventariadas 21 revisões, três ZIPs de source e o pacote PC publicado, todos os downloads conferidos contra hashes oficiais. Recuperados Title ID/identidade histórica, 247 seeds e a receita anterior baseada em `sp00nznet/xboxrecomp`. Nenhum código gerado ou XBE foi encontrado nas entradas inspecionadas.

O núcleo real do SDK compilou no Linux e o teste de memória virtual/aliases passou. O parser legado foi executado e falhou por `Default.xbe` ausente. O workflow Windows executou CMake/builder originais da versão 0.18: configuração passou (exit 0), build falhou (exit 1) por `integration-status.json` ausente, antes de compilar C++ do título. Run [43740071363](https://github.com/Duxton347/urban-chaos-android-recomp/actions/runs/43740071363), com log preservado em [PC_BUILD_RECOVERY.md](PC_BUILD_RECOVERY.md). Nenhum stub ou qualificação fictícia foi criado. Esses resultados não equivalem a boot do jogo.

## Primeira entrega

Estrutura local, `AGENTS.md`, os seis documentos solicitados e seis workflows iniciais estão preparados. Destino confirmado: `Duxton347/urban-chaos-android-recomp`, público, leitura/escrita verificadas após criação pelo usuário. Nenhum repositório alternativo criado. A integração inicialmente recusou `createRepository`; o usuário criou o destino exato e o acesso foi revalidado antes da publicação. Status de publicação/CI será registrado em [PUBLICATION.md](PUBLICATION.md).

## Auditoria de quatro referências

| Projeto | Revisão | Resultado / licença |
|---|---|---|
| Burner | `bdaef722ef1d589af30e5a58dd8543f64564c4fc` | Código público de patches/host; geração/configuração/fixtures privados ausentes. `COPYRIGHT` não concede licença geral explícita às adaptações. Não reutilizadas na implementação. |
| XboxXBERecomp | `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` | MIT; scanner/recompilador Linux compilados. Núcleo e host de pesquisa Linux compilados; runtime de produção do título não validado. |
| Black Android | `8e9c6fc7d570503ea68d039cff16c09339ee6d30` | MIT no projeto, assets/terceiros exigem revisão. Android CMake/SDL/Vulkan/áudio/input/HUD e histórico inspecionados; nenhum hook/endereço Black transplantado. |
| XboxKit | `e8c57ca4d8e15fba27c0d1d2f6a21d56b1f38819` | MIT declarado nos `.csproj`, texto LICENSE ausente. .NET/C# com NuGet; referência Redump/XDVDFS, não recompilador. Não compilado nem incorporado nesta sessão. |

Pins, URLs, árvores e dependências no `dependencies.lock.json`. O SDK observado em `cc9edda...` mudou o modelo para DLL/compilação no primeiro início; mantidos os pins originais. Entre os pins Burner/Black há nove caminhos diferentes, incluindo flags de áudio em `kernel/xboxkrnl.cpp`. Contexto CPU, arquivos de memória e interfaces HLE comparados são idênticos; isso não prova compatibilidade semântica. [Matriz](COMPATIBILITY_MATRIX.md).

## Comandos e resultados anteriores da rota

Os 21 testes abaixo pertencem à primeira auditoria. Após a inspeção de 2026-11-16 passaram 46 testes Python e 11 checks Java; [log sintético atualizado](evidence/user-image/tool-tests.log). Os testes do título foram privados e não são executados no CI público.

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

1. XBE Xbox fornecido e extraído; scanner e geração executados. O bloqueio atual é qualificar a variante de hash diferente e corrigir integração HLE/ABI, agora no carregamento da primeira missão e nas APIs restantes após corrigir gráficos, contexto e novas operações de áudio. Dados permanecem privados.
2. O build Burner parte de área privada qualificada. A nova geração privada foi reproduzida a partir do XBE fornecido, mas não recupera automaticamente a configuração completa qualificada da Burner. Recipes de algumas fixtures existem, mas requerem assets locais. [Entradas e consumidores](MISSING_INPUTS.md).
3. Reutilizar patches Burner exige licença aplicável. A intenção de reuso não esclarece direitos automaticamente. Não contactar terceiros sem instrução explícita.
4. Primeiro executar geração limpa e determinística e boot/menu PC, preferencialmente com backend de referência em runner privado. Windows PC na nuvem pode ser menor caminho para primeiro boot; Proton não é nativo Linux.
5. Depois adaptar backend Android compatível, sem transplante de endereços Black. Vulkan/áudio/memória/callbacks, missão, controles e saves precisam de evidência real.

XboxKit reduz incerteza sobre formatos de disco e offsets; não resolve a integração HLE/ABI e a qualificação pendente. Não há dados suficientes para afirmar que todas as entradas privadas podem ser recuperadas só com ferramentas públicas.

## Estado separado

| Item | Compila | Executa | Menu original | Missão | Campanha |
|---|---|---|---|---|---|
| Ferramentas host | Sim | CLI e fixture sintético testados | Não se aplica | Não se aplica | Não se aplica |
| Contrato CPU AArch64 | Sim | Não executado em dispositivo | Não se aplica | Não se aplica | Não se aplica |
| Diagnóstico histórico | Build anterior registrado | Tablet não testado | Não é jogo | Não é jogo | Não é jogo |
| Urban Chaos Linux de pesquisa | Sim: código + host de teste | Intro/título; depuração da carga da missão | Principal e dificuldade observados | Ainda não jogável | Não verificada |
| Código guest baseline Android ARM64 | Sim: 129 objetos AArch64 | Não ligado a host Android | Não testado em Android | Não testada | Não verificada |
| Urban Chaos Windows/Android | Build completo não demonstrado | Não demonstrado | Não demonstrado | Não demonstrado | Não demonstrado |

## CI inicial

Audit/static checks, host/native smoke, Android ARM64 contract, testes automatizados, artefatos de tooling sob allowlist e gate de release versionada. Cache limitado a dependências públicas. Nenhum job solicita ISO, segredos de jogo ou keystore. O workflow release falha fechado enquanto os gates estão incompletos; o workflow Android não fabrica APK diagnóstico como resultado do port. A configuração inicial ainda não implementa um build completo de runtime do jogo; isso está explicitamente bloqueado, não concluído.

## Primeira execução remota e correção

No commit `8fb039e...`, auditoria, host/native smoke e testes passaram no GitHub Actions. O job ARM64 falhou antes de compilar, com `sdkmanager: command not found` (exit 127). O workflow foi corrigido para baixar/verificar as command-line tools fixadas e usar SDK local do workspace, sem depender do PATH do runner. A reexecução dos quatro jobs passou no commit `4700b93...`, registrada em [PUBLICATION.md](PUBLICATION.md); a falha inicial permanece no histórico do GitHub.


O cache de auditoria foi limitado ao linter e o checkout Black passou a excluir entradas cifradas; o cache inicial de referências foi removido. Ver o registro de remediação e links de CI em [PUBLICATION.md](PUBLICATION.md). Esses checks continuam sendo validação de infraestrutura, não de jogo.
