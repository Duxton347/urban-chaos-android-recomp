# Launcher Android com núcleo ARM64

**Runtime completo ARM64 ligado e APK experimental privado gerado/assinado; instalação e boot Android ainda não testados.** [Primeiro teste](FIRST_TABLET_TEST.md), [receita](ANDROID_BUILD.md) e [recibo](evidence/runtime-progress/android-launcher-apk.json). O host Linux apresenta menu e abertura 3D originais em Vulkan.

O produto é um launcher que importa e monta a ISO original, com o runtime incluído no pacote Android e os recursos do disco externos. O Black também usa esse modelo: sua biblioteca ARM64 é construída antes da instalação. [Fluxo e rastreio do código](LAUNCHER_DESIGN.md).

## Toolchain fixada

Java 17.0.17+10; Gradle 8.7; AGP 8.5.2; SDK 34; build-tools 34.0.0; NDK 28.2.13676358; CMake 3.31.6; SDL release-3.4.16 (`fa2c02bb...`); ABI exclusiva `arm64-v8a`, mínimo API 29. URLs/checksums/commits estão no lockfile. O alvo prioritário é Xiaomi Pad 6 / HyperOS 2.0.16.0, sem pressupor versão Android, driver ou capacidades Vulkan a partir do nome HyperOS.

## O que Black demonstra

No pin `8e9c6fc...`, `native/main.cpp` coordena SDL e lifecycle, `native/vulkan.cpp` implementa backend Vulkan, `native/vk_shader.cpp` usa glslang, `native/audio.cpp` fornece saída SDL, `native/input.cpp` integra controladores e `native/touch.cpp` implementa HUD/editor. CMake remove módulos Windows do SDK e os substitui por host Android e implementações D3D/DirectSound específicas.

É uma referência MIT útil, mas seus acoplamentos `src/common`, `src/game`, layouts XDK, hooks e símbolos de Black não se tornam válidos para Urban Chaos. `src/d3d` e `src/sound` precisam de análise função a função; igualdade de headers não comprova semântica ou sincronização GPU. Não copiar `europe.patch`, executáveis criptografados, assets ou a política de armazenamento amplo. Os patches e imagens do HUD também precisam de proveniência separada.

## Ordem de integração

1. Qualificar geração e integração do título; depurar boot/menu no host Linux da nuvem. Windows é uma referência opcional, não uma etapa de conversão para Android. O C++ gerado é compilado diretamente para ARM64; testes independentes de portabilidade podem avançar antes do menu. [Falhas atuais](RUNTIME_PROGRESS.md).
2. Memória guest e ponteiros: verificar mapeamento 32/64 bits, alinhamento, permissões, endianness, TLS, threads, ordinais e callbacks. Não truncar ponteiros host.
3. Host Android: SDL3, pausa/foco/retorno, destruição/recriação de superfície e orientação. Nenhuma thread continua usando swapchain destruída; estados de input devem ser liberados na perda de foco.
4. D3D8→Vulkan: preservar recursos, shaders/combiner, estados, render targets, fences/locks, leituras de memória e ordem observada de draws. Primeiro frames reais de logo/menu, depois primeira missão. O frame colorido do diagnóstico não serve como evidência.
5. DirectSound→SDL3: buffers/streams, codecs, ADPCM, canais, mixing, reverb e relógio conforme o título. Logs de inicialização não substituem áudio ouvido e sincronizado.
6. SAF e armazenamento privado; HUD sobre ações verificadas; saves do jogo e update preservando dados.
7. Testar Adreno/Vulkan reais no Pad 6 e registrar GPU, driver, extensões/limites, erros e sessões térmicas. Resolução interna configurável, aspecto 16:10 sem deformação e escalonamento são posteriores à fidelidade básica. Sem promessa de 60 FPS.

## Executado e pendente

`native/portability/cpu_contract.cpp` compila como objeto ELF AArch64 com o NDK fixado. Verifica little-endian, ponteiros de 64 bits, alinhamento/offsets de `X86Context` e acesso guest desalinhado compilável. [Log](evidence/route-v2/android-portability.log). **Não é** execução do runtime, teste de Vulkan, instalação ou APK de jogo.

A CI `android-arm64.yml` compila esse contrato e registra G1 bloqueado. O job completo de APK só será introduzido quando existirem fontes originais geradas e integração qualificada. A entrega é o launcher com execução real do jogo; uma interface de seleção isolada não satisfaz esse resultado.

Saves: planejar volumes Xbox dentro de `files/saves/`, reproduzir caminhos/semântica originais, escrita atômica quando compatível e isolamento por versão/perfil. Validar checkpoint, morte, load após fechar processo e update preservando dados. `config/hud.json` do diagnóstico é configuração de interface, **não progresso do jogo**.

[Testes manuais históricos](DEVICE_TESTS.md) continuam úteis para infraestrutura, mas o novo relatório do jogo deve separar boot/menu/missão/áudio/save/transições/lifecycle. No Android, todos permanecem pendentes; o menu observado pertence ao host Linux de pesquisa.

## Saída e relatórios compartilhados implementados

O backend estéreo SDL3 em native/audio/sdl_output.cpp está ligado ao host de pesquisa e compila para AArch64. [Contrato, lifecycle e limites](SDL_AUDIO.md). Isso remove a necessidade de adaptar diretamente o callback 5.1 do Black; não qualifica áudio ouvido ou o runtime Android completo. O controle Android de exportação está pronto para anexar à Activity real: [diagnósticos](DIAGNOSTICS.md). Activity/loop e renderer Android foram ligados no APK experimental; coordenação de threads, lifecycle, renderização e áudio no dispositivo ainda precisam de validação.

## Queries de visibilidade Vulkan

O componente próprio de queries precisas e entrega após fences foi testado na API Vulkan por software e compilado para AArch64. [Sequência obrigatória de integração](VULKAN_VISIBILITY.md). Ele já está ligado aos draws/targets, fences e Flush do backend de pesquisa, que apresentou menu e abertura 3D original. [Resultado e etapas restantes até o APK](VULKAN_RENDERER.md). Essa execução Linux não valida o jogo no Android. Não presumir suporte preciso a partir do nome Adreno: consultar e habilitar o recurso no dispositivo real.
