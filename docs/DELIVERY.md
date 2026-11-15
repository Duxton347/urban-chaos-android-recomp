# Entrega — 2026-11-15

**GO: diagnóstico independente. BLOCKED: Urban Chaos Android em G1.**

APK: [UrbanChaos-Diagnostic-0.1-arm64-debug.apk](../artifacts/UrbanChaos-Diagnostic-0.1-arm64-debug.apk)

- Pacote: `org.urbanchaos.diagnostic`; versão `0.1-diagnostic`, código 1.
- ARM64 exclusivamente; Android API 29+; target/SDK 34.
- Tamanho: 5,022,371 bytes.
- SHA-256: `917066c439a3421067a43a614a7926c66a1ff1ff6b4fbe71010b6a027a10ffa4`.
- Assinatura debug própria do projeto; chave privada fora da distribuição. Release não produzida.

## Verificação executada

Build debug e Android lint concluídos. Lint: zero erros, avisos preservados em [android-lint.txt](evidence/android-lint.txt), com filtros específicos do fornecedor explicados em [BUILD_AND_TEST_NOTES.md](BUILD_AND_TEST_NOTES.md). Assinatura APK verificada, alinhamento ZIP aprovado, três bibliotecas ELF64 AArch64 (`libmain.so`, `libSDL3.so`, `libc++_shared.so`), avisos de licença embarcados. Sem XBE, ISO, código gerado do título ou chave privada no APK. [Recibo completo](evidence/apk-verification.json).

Ferramentas host compiladas; **19 testes Python e 11 verificações Java passaram**, usando apenas fixtures sintéticos. Saída determinística refere-se a esse fixture e não ao jogo. [Log](evidence/tools-tests.log).

## Uso e próximo bloqueio

Baixar o APK e instalar manualmente no tablet. Abrir **Urban Chaos · Diagnóstico**, testar Vulkan, SAF e HUD conforme [DEVICE_TESTS.md](DEVICE_TESTS.md). O app não executa o jogo; suas telas são ferramentas diagnósticas.

**Não testados no dispositivo:** instalação, abertura, frames Vulkan, suspensão/resumo, SAF real, multitouch/gamepad e atualização preservando dados. GPU/driver reais do Pad 6 permanecem desconhecidos nesta execução.

**Não demonstrados para o jogo:** compila, inicia, menu funciona, missão jogável, campanha verificada. G1 exige XBE Xbox legítimo e geração/configuração reproduzíveis; G2–G7 dependem disso. [Status por gate](PLAN.md).

[Instruções de build/importação/HUD](../README.md). O arquivo de fontes em `artifacts/UrbanChaos-Android-source.tar.gz` contém código, lockfile, documentos e evidências; exclui fornecedores clonados, toolchains, builds, conteúdo comercial e keystores. Os clones e ferramentas ficam disponíveis neste workspace e podem ser restaurados pelos scripts/pins.
