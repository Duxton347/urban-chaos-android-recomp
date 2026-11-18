# Renderer Vulkan ligado ao título no host de pesquisa

Em 2026-11-18, o backend Vulkan reaproveitado do Black apresentou o menu original e a abertura 3D de Hope Street com o executável e recursos do usuário. **Não é execução Android, missão jogável ou fidelidade gráfica qualificada.** Há defeitos visuais na cena 3D. [Recibo sanitizado](evidence/runtime-progress/vulkan-renderer.json).

## Código e ligação

O fonte MIT `native/vulkan.cpp` do Black `8e9c6fc7d570503ea68d039cff16c09339ee6d30` é verificado por SHA-256, copiado para diretório ignorado e adaptado com `patches/black/vulkan-headless-research.patch`. O checkout upstream permanece intacto. O bootstrap obtém somente os fontes públicos necessários, sem hooks ou recursos de Black. Avisos MIT permanecem em `licenses/Black-Android-MIT.txt`.

O patch troca a superfície SDL/Android por `VK_EXT_headless_surface` para permitir pesquisa na nuvem, retira o overlay ImGui/skins do host, mantém fog/bloom e desativa pós-processamento adicional e caches em disco. O HUD original continua sendo desenhado pelo próprio título. Texturas, shaders, targets, readback e a apresentação por swapchain usam o backend Vulkan real, sem renderer CPU como substituto. As opções do Black e seus endereços não são transplantados para Urban Chaos.

As queries próprias estão ligadas: `occlusionQueryPrecise` habilitado no device; reset antes do render pass; query ao redor de cada draw com contador ativo; normalização por escala do target; snapshot em GetReport; seal antes de submit; readback/entrega após conclusão real por fence/timeline. O writer valida páginas guest e publica status após os outros campos do report de 16 bytes. A capacidade força submissão antes de exceder os draws contados; o pool só é reutilizado após readback. A capability passou a ser verdadeira neste backend porque seus consumidores agora existem.

Draws contados sem geometria, textura, target ou pipeline suportado falham explicitamente com `UC-VK-008`, em vez de produzir reports zero por omissão. A otimização de cópia de profundidade é desativada quando o draw exige contagem. O teste autoral verifica a rejeição de um draw inválido.

O percurso inicial expôs uma ligação ausente: `harness::g_frames` não era atualizado pelo renderer Black. Os comandos automatizados ficavam parados no frame zero. O adaptador de pesquisa agora avança o relógio de input em cada apresentação. Não foi alterada a lógica de missão para contornar o problema. Também foi corrigido o Begin duplicado herdado da inicialização das imagens fixas: após Submit(true), o command buffer seguinte já está aberto e contém o reset de queries.

## Verificações executadas

- Host Linux real compilado e ligado ao arquivo privado com 29.049 funções originais recompiladas.
- Primeiro ensaio de 30 s: vídeo original em Vulkan, encerramento controlado, nenhum recurso solicitado ausente ou exceção registrada.
- Ensaio com relógio de input ausente: interrompido controladamente em 136 s para correção, sem afirmar que menus haviam sido testados.
- Ensaio corrigido de 180 s: menu original inspecionado no frame 1440, vídeo no 1680, cena 3D de Hope Street no 1800; último contador registrado 1860. Dezesseis conclusões de reports registradas, incluindo amostras positivas e zero. O logger limita os eventos individuais, portanto esse número não é uma contagem total de todas as queries. Sem nova exceção registrada ou pedido de arquivo ausente nesse intervalo. Saída SDL dummy, sem escuta do áudio.
- Fixture pública usa o mesmo backend e o contrato autoral de visibilidade do SDK, em modo direto e com render thread: dez amostras visíveis, oclusão/profundidade, alpha rejection, reset, contagem desativada, color-write desativado, reports/Flush. Uma apresentação real também verifica o avanço do contador de input.
- O renderer adaptado e suas queries compilaram em dois objetos x86_64 e dois AArch64. Isso não é link do jogo Android; o adaptador de superfície continua sendo o de pesquisa Linux.
- Após reforçar a rejeição de draws contados não suportados, um novo ensaio de 180 s voltou à abertura 3D de Hope Street, com dezesseis reports registrados e sem nova exceção ou pedido de arquivo ausente. A captura do frame 1800 foi inspecionada novamente. Esse build corresponde ao mesmo fonte do contrato público final.

Passaram também os 46 testes Python e oito CTests de regressão. A primeira compilação da fixture pública falhou porque faltava sua opção x86 SSE4.1; foi corrigida com o baseline `-march=x86-64-v2` já usado pelo host SDK, sem alterar as rotas ARM. Uma rodada Python encontrou a reserva mínima de espaço do importador; somente intermediários regeneráveis foram removidos e a repetição passou. Logs das tentativas permanecem privados, e os resultados estão no recibo.

O driver é SwiftShader por software, não a GPU Adreno do tablet. Não foram usadas camadas Khronos de validação. Capturas, logs brutos, código gerado e assets continuam privados. O contrato autoral passa a integrar a CI sem dados do título.

O coletor exportável também reconhece frames Vulkan (`UC-GPU-001`) e reports concluídos (`UC-GPU-002`), com campos numéricos. Foram exercitados por fixtures de journal/JNI/ZIP e 326 checks Java. Essa ligação será incluída no próximo build do runtime; os ensaios originais acima demonstram o renderer e seus logs privados, sem afirmar que esses novos eventos já foram coletados no tablet.

## Receita

```sh
python3 scripts/build_vulkan_reports.py
python3 scripts/build_vulkan_shaders.py --android
python3 scripts/build_vulkan_renderer.py --android
```

O build privado existente recebe `--render-backend vulkan-headless`, mantendo `reference` como opção padrão para comparação. O runner privado seleciona explicitamente o ICD de software e converte os parâmetros de captura para o backend Vulkan. As gravações ocorrem apenas na pasta privada de execução; não são exportadas pelo coletor do launcher.

## O que falta para um APK de teste

1. Ligar o runtime completo ARM64 ao host Android: memória/threads, SDL, janela, áudio e este renderer com a superfície Android real. Implementar coordenação de pausa, perda/recriação de superfície e encerramento; o adaptador headless não fornece esse lifecycle.
2. Integrar Activity e SAF ao leitor da ISO, controles reais de touch/gamepad, volumes privados e exportação de relatórios já preparada. As classes de diagnóstico existentes não são um launcher completo.
3. Empacotar e assinar um APK debug privado, verificar bibliotecas e fazer instalação manual no Pad 6. Confirmar menu, áudio e retomada pelo driver real antes de chamar o build de utilizável.

A primeira abertura no tablet pode ser testada antes de qualificar toda a campanha, com limitações explícitas. Ainda precisam de investigação a fidelidade da cena, fences/resource-busy e pipelines não suportados, progressão de missão e saves. Não há base para uma porcentagem de conclusão ou data confiável; gerar um APK agora não comprovaria essas ligações.
