# Auditoria executada — 2026-11-15

> Histórico da etapa anterior. A rota vigente está em [BUILD_STATUS.md](BUILD_STATUS.md). O diagnóstico não faz parte da entrega do jogo; o teste determinístico antigo não comprovava funções emitidas, corrigido na nova rota.

**GO** para ferramentas host e diagnóstico Android independente. **BLOCKED** para recompilação de Urban Chaos (G1) e gates dependentes. Nenhum boot ou frame do jogo foi produzido.

## Proveniência e reprodução

Os checkouts estão em `vendor/`, ignorados pelo Git. `dependencies.lock.json` registra URLs, commits, árvores Git, licenças, arquivos de toolchain e checksums. O runtime base é o da Burner. Os três repositórios foram clonados e fixados:

| Projeto | Origem | Commit |
|---|---|---|
| Burner | https://github.com/linkssy2/UrbanChaosRiotResponseBurnerEdition | bdaef722ef1d589af30e5a58dd8543f64564c4fc |
| Black Android | https://github.com/RanggaCasper/Black-Android | 8e9c6fc7d570503ea68d039cff16c09339ee6d30 |
| XboxXBERecomp | https://github.com/TGP482/XboxXBERecomp | c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7 |

Comandos executados: `git clone`, `git checkout --detach <pin>`, `scripts/build_host.sh`, `scripts/test.sh`, `scripts/build_android.sh assembleDebug lintDebug`, `python3 scripts/audit.py`. A primeira tentativa no sandbox não alcançou o proxy; os downloads públicos e builds com rede foram executados pela escalada autorizada. Nenhuma configuração de sandbox ou TLS foi desativada.

## O que foi comprovado

| Componente | Resultado | Evidência |
|---|---|---|
| `xbe_symbols`, `xbe_recomp` | Compilação Linux com GCC 14 e C++20 | [host-build.log](evidence/host-build.log), [receita de binários](evidence/host-tools.receipt.json) |
| Ferramentas Python | 19 testes, incluindo saída sintética determinística de duas execuções do recompilador real | [tools-tests.log](evidence/tools-tests.log) |
| Parser XDVDFS Java | 11 verificações, incluindo cancelamento, PS2, truncamento e caminhos | [tools-tests.log](evidence/tools-tests.log) |
| Diagnóstico Android | Build debug ARM64, Java 17, SDL3, Vulkan; resultado final no recibo de entrega | [android-build.log](evidence/android-build.log) |
| Android lint | Verificação executada; resultado e avisos preservados | [android-lint.txt](evidence/android-lint.txt) |
| APK | Assinatura, alinhamento ZIP, ELF64 AArch64, bibliotecas e ausência de conteúdo do jogo | [apk-verification.json](evidence/apk-verification.json) |
| Dispositivo físico | Não testado; nenhum acesso ao Pad 6 nesta sessão | [DEVICE_TESTS.md](DEVICE_TESTS.md) |

A configuração host **não inclui** `cmake/XbeRecomp.cmake`: esse módulo também configura o runtime Windows, ImGui Win32/DX12, XAudio2 e links D3D12. O projeto `host/CMakeLists.txt` compila somente os dois executáveis públicos, Zydis/Zycore e toml++. O scanner usa `std::erase_if`, exigindo C++20. O SDK original permanece limpo.

O diagnóstico contém uma janela SDL3, superfície/swapchain Vulkan e frames de cor variável usando operações de transferência. Trata background/foreground e recriação de swapchain. Ele não implementa D3D8→Vulkan, áudio do jogo ou execução de x86 recompilado. O teste de renderização em hardware permanece pendente.

## Entradas e direitos

`vendor/burner/docs/BUILDING.md:3–7` declara configuração escaneada, unidades geradas, backups e fixtures privados. `README.md:8–12` informa que um checkout limpo não reproduz o executável. A árvore Git não contém `build/*` nem `game_files/*`. O driver lê `integration-status.json` e exige `tools_qualified` antes de preparar/compilar (`scripts/build_xbe_game_trial.py:11–12`). Não executamos esse driver nem fabricamos esse status.

O inventário de consumidores públicos está em [input-consumers.json](evidence/input-consumers.json); as categorias e receitas estão em [MISSING_INPUTS.md](MISSING_INPUTS.md). É uma análise estática de referências literais, não uma prova de que todas as entradas dinâmicas foram enumeradas.

Nenhum XBE, ISO Xbox ou ISO PS2 foi disponibilizado no workspace desta execução. A informação de que a imagem anterior era PS2 vem do plano recebido; não foi reinspecionada aqui. A rejeição PS2 foi testada com dados sintéticos.

Burner `COPYRIGHT` preserva direitos sobre conteúdo original e avisos de terceiros; não concede uma licença geral explícita às adaptações próprias. **BLOCKED** para incorporar essas adaptações. Black é uma referência de arquitetura, sem copiar hooks, endereços, skins ou estruturas do título. XboxXBERecomp é MIT. SDL é zlib, com exceções locais (HIDAPI sob opção BSD, yuv2rgb e outros avisos preservados). Zydis/Zycore e toml++ são MIT. O runtime C++ do NDK tem avisos LLVM/Android incluídos no APK.

O inventário [source-license-inventory.json](evidence/source-license-inventory.json) registra hashes por arquivo e linhas de avisos dos conjuntos reutilizados, incluindo candidatos não compilados. `licenses/` preserva os textos; `assets/licenses/` contém os avisos pertinentes à distribuição Android. O código novo do diagnóstico é independente das adaptações da Burner e de Black.

## Ajustes e limites da auditoria

A comparação confirma nove arquivos diferentes, mas corrige a premissa de que todo o kernel seria idêntico: `runtime/kernel/xboxkrnl.cpp` muda flags estéreo de EEPROM. Contexto CPU, `kernel/memory.*`, `kernel/kernel.h` e `xdk/hle.h` comparados são idênticos. Ver [COMPATIBILITY_MATRIX.md](COMPATIBILITY_MATRIX.md).

A importação Android valida o tamanho e o SHA-256 exatos do candidato antes de ativar. Isso identifica um arquivo conhecido; não qualifica ABI, símbolos nem compatibilidade Android. O inspetor Linux enumera bibliotecas e ordinais kernel; preserva o ponteiro de importação não kernel como metadado, sem afirmar resolução nominal desses módulos. Não verifica assinatura RSA nem posse/licença do disco.

A geração sintética passou; a geração do título permanece **BLOCKED** sem o XBE. Identificar limites de função, símbolos não resolvidos, ABI, callbacks e hooks do título será **REQUIRES_REVERSE_ENGINEERING** quando o arquivo existir. Não há evidência para afirmar que toda a entrada privada da Burner pode ser recuperada com ferramentas públicas.
