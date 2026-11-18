# Reports Vulkan — componente de queries reais

**GO para o componente: contagem validada na API Vulkan por software e código compilado para AArch64. O renderer Vulkan do jogo e sua ligação ao Android continuam pendentes.** O jogo Linux continua usando o renderer de referência. Não se substituiu a lógica do título nem se apresentou a fixture como execução de Urban Chaos.

## Contrato auditado

No SDK c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7, Backend::ClearReport zera o intervalo de contagem. Cada draw com State::simple[kMCountEnable/4] ativo contribui com amostras que passaram pelos testes de fragmento/profundidade/stencil. GetReport captura o resultado acumulado desde o clear. O leitor original XDK observa uma estrutura de 16 bytes: timestamp baixo/alto em +0/+4, contagem em +8 e status de conclusão em +12. Status pendente não pode virar sucesso com uma contagem inventada.

O backend D3D12 do SDK envolve cada draw contado em uma query e normaliza escala/MSAA. O Vulkan do Black, pin 8e9c6fc7d570503ea68d039cff16c09339ee6d30, não sobrescreve ClearReport/GetReport. Seu Draw abre a renderização em native/vulkan.cpp e chama vkCmdDrawIndexed/vkCmdDraw nas linhas 2928/2930. Submit termina a renderização, submete comandos e acompanha timeline/fences. Esses são pontos de ligação a adaptar, não evidência de que os runtimes ou pipelines de títulos diferentes sejam intercambiáveis. Nenhum arquivo vendor foi alterado.

## Implementação própria

native/gpu/vulkan_visibility.* usa VK_QUERY_TYPE_OCCLUSION com VK_QUERY_CONTROL_PRECISE_BIT. O recurso occlusionQueryPrecise deve estar habilitado no VkDevice. Ausência do recurso provoca erro explícito; não se usa o resultado booleano impreciso como contagem de pixels.

- BeginCommands registra reset de um pool fora de render pass, com o serial da submissão futura.
- BeginDraw/EndDraw envolvem somente o draw real, dentro da mesma renderização. Draw desabilitado para contagem não emite query.
- Clear inicia outro intervalo lógico. Reports pendentes conservam o snapshot anterior.
- Report captura as amostras acumuladas e o serial da submissão. Intervalo vazio entrega zero somente após a conclusão exigida.
- Poll recebe o watermark de fences/timeline realmente concluídos. Usa resultados de 64 bits e disponibilidade, sem flag WAIT. Não chama o escritor antes de concluir o report.
- Resultados concluídos são copiados para objetos CPU antes de reutilizar/resetar pools. Um intervalo que cruza frames conserva o resultado anterior, mesmo que o slot GPU seja reutilizado.
- Escala e MSAA são removidos por draw; a soma é arredondada uma vez e segue o contador de 32 bits. O teste de normalização usa fatores autorais; não qualifica opções gráficas do jogo.

Há limites independentes para queries por submissão, submissões em voo, amostras vivas, reports pendentes e referências nos snapshots. Isso limita também o custo de muitos reports repetindo um intervalo grande. Exaustão é erro explícito, nunca wrap silencioso de slots ainda em uso. Poll não espera o dispositivo; a espera por device idle existe somente na destruição, antes de destruir pools. O dispositivo precisa continuar vivo nesse ponto.

visibility_report.h entrega os 16 bytes, verifica o tamanho do mapeamento e publica status após os dados. A integração deve validar o endereço guest e sincronizar o getter original com Flush. O timestamp é um valor lógico fornecido pelo host; não foi implementado um relógio GPU equivalente ao Xbox nesta etapa.

## Sequência exigida na integração do renderer

1. Habilitar o recurso preciso no device se realmente suportado. Anunciar SupportsVisibilityReports somente quando todas as ligações abaixo existirem.
2. No início do command buffer, fora de render pass, passar o serial que será sinalizado ao submeter. Poll deve também ocorrer na reciclagem de frames, mesmo sem novo GetReport.
3. Em cada draw contado, depois de BeginRender e imediatamente ao redor dos comandos vkCmdDraw/vkCmdDrawIndexed, chamar BeginDraw/EndDraw. Não contar passes de apresentação, filtros ou UI do host. Geometria/shader que o renderer não consiga executar deve falhar explicitamente.
4. Encaminhar ClearReport e GetReport do decoder sem substituir seus intervalos. Fornecer a escala real do target e o número de samples. Não presumir que todos os targets têm tamanho ou MSAA iguais.
5. Após fences/timeline, entregar os reports ao mapeamento guest. O Flush usado pelo hook GetVisibilityTestResult precisa submeter, esperar os comandos exigidos e publicar os resultados antes do leitor XDK original.
6. Coordenar encerramento, perda de superfície e threads do renderer. O componente tem um único owner e não é um wrapper concorrente para várias threads acessarem a fila.

Essa sequência ainda precisa ser ligada ao renderer completo. Não foi criado patch parcial de Black anunciado como backend funcional, e a capability do título não foi habilitada artificialmente.

## Teste real executado

scripts/build_vulkan_reports.py compila shaders GLSL de autoria própria com o glslc do NDK 28.2.13676358, usa seus headers Vulkan isolados dos headers libc Android e executa Vulkan 1.1 sem janela. O driver observado foi SwiftShader Device (Subzero), tipo CPU, API 1.3.0. Loader, ICD, driver, compiler, fontes e SPIR-V têm hashes no recibo. O driver é uma propriedade do ambiente testado; não é incluído no projeto ou distribuído em artefatos.

Foram entregues 21 reports reais:

| Caso | Resultado |
|---|---|
| Triângulo cobrindo 8×8, escrita de cor desativada | 64 |
| Profundidade igual sob LESS / geometria atrás | 0 / 0 |
| Shader descartando fragmentos com escrita de cor desativada | 0 |
| Draw posterior ao descarte | 64; descarte não atualizou profundidade |
| Pool reutilizado, preservando amostra anterior e nova escala | 96 |
| Snapshots repetidos e clear antes da entrega | contagens independentes corretas |
| Contagem desabilitada / intervalo vazio | 0 |
| Normalização acumulada e overflow de 32 bits | 2 / 7 |
| Duas submissões em voo | entrega separada após o fence correspondente |
| Capacidade de pool, memória e snapshots excedida | rejeição sem reutilização indevida |
| Escritor falha e é chamado novamente | report não perdido nem marcado como entregue |
| Estrutura e buffer sentinela | 16 bytes, timestamp/contagem/status e sentinela preservados |

Os testes não habilitaram a camada Khronos de validação; validation_errors é desconhecido, não zero inferido. Não comprovam stencil do título, MSAA físico, dynamic rendering, driver Adreno, swapchain Android, shaders/texturas Xbox ou frames do jogo via Vulkan. O teste AArch64 compila o mesmo componente e layout; não executa código no tablet.

[Recibo](evidence/runtime-progress/vulkan-visibility.json), [estado do port](BUILD_STATUS.md). O próximo trabalho é ligar o componente aos draws e à sincronização do renderer Vulkan completo, validar shaders/targets e então testar os frames originais. Missão e campanha permanecem sem qualificação.
