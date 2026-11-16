# Recompilação e menor caminho executável

**G1 qualificado pendente:** o XBE fornecido já foi extraído, gerado deterministicamente e compilado no Linux x86_64. Um host SDK de pesquisa inicia parcialmente; a primeira falha DSOUND passou com patch experimental, expondo crash D3D antes de menu. Ver [testes posteriores](AUDIO_INVESTIGATION.md). Ver [execução atual e próximos passos](USER_IMAGE_INSPECTION.md). O roteiro abaixo preserva os requisitos da referência; a variante fornecida exige sua própria revisão.

## Pins auditados

| Referência | Commit utilizado |
|---|---|
| Burner Edition 0.18 | `bdaef722ef1d589af30e5a58dd8543f64564c4fc` |
| XboxXBERecomp base Burner | `c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7` |
| Black Android | `8e9c6fc7d570503ea68d039cff16c09339ee6d30` |
| SDK base Black (comparação) | `99078b119b5d5e0d1d6afb1390e29437c0c984c0` |
| XboxKit 0.7.0 | `e8c57ca4d8e15fba27c0d1d2f6a21d56b1f38819` |

O HEAD do SDK observado foi `cc9edda3a821a4204dc28783f429b88a27ade4f9`, somente para análise histórica. Commits `4256397`/`3010abc` introduzem geração/carregamento de DLL no primeiro início; `6ea2a13` remove sdk_sync e `70572da` remove harness. Não migramos o pin. O `CMakeLists.txt` de Black:24 já documenta a incompatibilidade dessa rota com seu pipeline estático. Burner e Black HEADs observados permanecem iguais aos pins.

## Cadeia Burner

O CMake raiz não compila diretamente o jogo: chama PowerShell e `scripts/build_xbe_game_trial.py`. Esse driver exige mutex proprietário do wrapper e lê `build/xbe-full-integration-20261014/integration-status.json`, exigindo `tools_qualified`. Depois aplica preparadores aos arquivos já gerados e constrói com ferramentas/caminhos Windows. Não é uma receita pública completa desde disco.

O conjunto ausente inclui configuração escaneada/revisada, unidades `generated/game/x86_recomp.*.cpp`, mapping/headers, backups `generated-input-original`, fixtures derivadas e assets. [MISSING_INPUTS.md](MISSING_INPUTS.md) distingue build de validação; [input-consumers.json](evidence/input-consumers.json) enumera consumidores literais. Não criar `tools_qualified=true`, stubs ou fixtures simulados para ultrapassar o driver.

Os hooks públicos pressupõem funções/instruções e corpos específicos do XBE, verificando hashes/endereços e alterando código gerado. Exemplos de áreas: `prepare_xbe_input.py`, correções de áudio, callbacks, recursos D3D, câmera, ragdoll e menus. Esses arquivos são evidência para localizar consumidores; sua implementação não foi copiada. `COPYRIGHT` não esclarece uma licença geral das adaptações próprias. Reutilização depende de autorização/licença documentada.

O SDK implementa famílias de kernel, memória, threads, arquivos, objetos, crypto/RTL, XAPI, XNet, CRT, D3D8, DirectSound e Bink. A existência de funções não comprova cobertura dos imports de Urban Chaos: o scanner real deverá apontar ausências. O runtime inclui ramos Win32 e ramos portáveis; CMake/links D3D12/XAudio2 e o driver Burner continuam Windows. Linux/Proton descrito pela Burner não é execução nativa Linux.

## Etapas mínimas

1. Disponibilizar a cópia Xbox legítima em volume privado na nuvem. Validar o candidato: 5.283.840 bytes, SHA-256 `b74ae76fdbf1ef5f95dcfabf4f94f3aa738e649ecaef4068d8531e07071b5dac`. Title ID `534300F7`/versão `3` foram confirmados no executável fornecido; seu SHA-256 é diferente da referência.
2. Executar scanner público pinado e conservar TOML/logs privadamente. Revisar símbolos XDK ausentes, funções, jump tables, ABI e hooks comprovados. A declaração de build Windows Burner não substitui esse trabalho.
3. Executar `tools/recompile_xbe.py` com configuração explícita em `private/config/urban-chaos.toml`. Gerar duas vezes em `build/`, exigir funções emitidas e comparar hashes. Recriar backups somente de saídas limpas.
4. Resolver licença dos patches Burner e separar implementação do jogo de backend. Compilar/rodar primeiro no PC da nuvem. O host Linux de pesquisa já existe; priorizar sua correção. Windows é comparação opcional e não entrada do build Android. Não chamaremos Proton de nativo.
5. Somente após boot/menu PC reais: compilar ARM64 com backend Vulkan/SDL3 revisado a partir das partes MIT de Black, testando memória/threads/callbacks/D3D8/áudio para Urban Chaos.
6. Menu, primeira missão, save/load e lifecycle têm gates separados. Não há promessa de FPS ou de data de conclusão.

## Executado agora

```bash
scripts/build_host.sh
scripts/test.sh
python3 scripts/native_smoke.py
scripts/build_portability.sh
```

Scanner e recompilador C++20 compilados. O novo smoke gera um XBE **sintético**, recompila `mov eax,42; ret`, compila a saída e verifica EAX e pilha executando nativamente no Linux. Não usa lógica do jogo. O primeiro smoke falhou porque a configuração antiga não declarava `code_sections`; o teste antigo comparava saída determinística sem comprovar funções. A configuração e os checks foram corrigidos. [Falha preservada](evidence/route-v2/native-smoke-initial-failure.log), [resultado corrigido](evidence/route-v2/native-smoke.log).

O wrapper agora rejeita saída sem funções. Esses testes reduzem falsos positivos de infraestrutura; não abrem G1. O contrato CPU também compila para AArch64; isso não é execução de guest, kernel completo ou APK do jogo.
