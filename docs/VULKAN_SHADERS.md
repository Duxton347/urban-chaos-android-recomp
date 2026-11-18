# Reutilização do compilador de shaders do Black

Estado em 2026-11-18: **GO para o componente HLSL → SPIR-V em Linux e sua compilação ARM64; integração do renderer do jogo pendente.** Legendas/tradução foram adiadas por orientação do usuário.

O teste usa diretamente `native/vk_shader.cpp` e `.h` do Black `8e9c6fc7d570503ea68d039cff16c09339ee6d30`, sob MIT, e glslang 16.0.0 `a57276bf558f5cf94d3a9854ebdf5a2236849a5a`. Não copia hooks, endereços, executável nem recursos de Black. A origem dos arquivos e seus SHA-256 estão em `dependencies.lock.json`; as licenças completas estão em `licenses/`. O bootstrap baixa somente esses dois fontes e a licença do Black, além do repositório público glslang. Os checkouts originais permanecem preservados.

## Executado

Com SDK/NDK e runtime público já preparados:

```sh
python3 scripts/build_vulkan_reports.py
python3 scripts/build_vulkan_shaders.py --android
```

O segundo comando também prepara as dependências específicas. Usa CMake 3.31.6 e NDK 28.2.13676358, Android 29, `arm64-v8a`. Ferramentas opcionais, otimizador SPIRV-Tools e testes externos de glslang ficam desativados. A CI Android executa os dois comandos e falha se não houver driver de software ou se qualquer teste falhar. Logs ficam em `build/vulkan-shaders/`, fora do Git.

Foram compilados dez shaders: sete gerados pelo SDK pinado (vértice fixo, RHW, iluminação, pixel, textura, sombra e alpha kill), um helper autoral com textura/sampler e um par autoral de fullscreen. O teste inspeciona estágio, locations de entradas, descriptor set zero, bindings `b0/b1=0/1`, `t0..3=2..5`, `s0..3=6..9` e ausência de ponteiros Function para tipos opacos de imagem/sampler. Isso verifica a especialização reaproveitada do Black, necessária para evitar VUID 06924; não constitui validação completa de SPIR-V.

O par HLSL autoral compilado foi executado pelo fixture Vulkan existente, passando seus 21 reports: 64 amostras visíveis, zero oclusas/descartadas, snapshots, fences e reutilização de pools. Driver real de software SwiftShader, sem camadas de validação; não é teste de Adreno nem frame de Urban Chaos. Os demais shaders foram compilados/inspecionados, sem executar suas combinações de pipeline.

O tradutor e suas dependências foram compilados em três arquivos estáticos Android: 1, 48 e 2 objetos, todos AArch64 verificados por `llvm-readelf`. Isso não demonstra link do jogo nem execução Android. [Recibo sanitizado](evidence/runtime-progress/vulkan-shaders.json).

Os 46 testes Python também passaram. A primeira execução teve três erros e uma falha porque o volume ficou com menos que os 64 MiB exigidos pelo importador; após remover somente uma geração C++ antiga regenerável, a repetição passou integralmente. A proteção de espaço foi preservada, assim como os dados originais e a geração atual.

## Integração seguinte e limites

O caminho reaproveitável é `SDK gpu::FixedVertexHlsl/PixelHlsl → vkshader::Compile → VkShaderModule`, já exercitado aqui. O backend completo do Black ainda depende do host Android/SDL, ImGui, configurações de efeitos e swapchain; esses consumidores precisam de adaptação explícita, preservando as opções originais de Urban Chaos. Não basta trocar o nome do jogo nem copiar o backend inteiro sem verificar essas ligações.

Os reports Vulkan próprios precisam entrar no ciclo de command buffers desse backend: reset fora do render pass, Begin/End ao redor do draw contado, seal antes de submit, entrega só após fence/timeline e readback antes de reutilizar o pool. O Black pinado não implementa esse contrato do SDK; herdar métodos vazios bloquearia novamente a progressão do título.

O cache de shaders fica desativado nesta qualificação. O leitor upstream não limita tamanho nem valida integridade/estágio dos módulos em disco; deve ser endurecido antes de ativar cache no aplicativo. Falhas upstream também podem imprimir HLSL: esse conteúdo não deve ir para relatórios públicos/exportáveis do jogo. Não foi alterado o checkout para esconder essas limitações.

O próximo marco é o primeiro frame **original** pelo backend Vulkan, comparado à captura privada do renderer de referência, com reports e diagnóstico ligados. Nenhum gate de missão, fidelidade ou APK mudou por causa deste teste.
