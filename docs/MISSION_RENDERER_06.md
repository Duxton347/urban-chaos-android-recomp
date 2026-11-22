# Renderer e sincronização da missão — candidato privado 0.6

## Relatório do tablet

O proprietário abriu a missão com 0.5 depois de um carregamento muito longo e relatou gráficos/execução defeituosos. O relatório confirma o build `experimental-737ee5c2bd1835cd`, três arquivos opcionais ativos, primeiro evento de visibilidade aos 419,367 s e cerca de **0,968 apresentações/s** no intervalo final. Não qualifica a missão como jogável.

As 1.399 leituras somam 121.256.208 bytes e cerca de 262 ms de tempo acumulado; a mais longa leva 3,296 ms. O snapshot final não tem leitura pendente. O callback de áudio chega ao máximo de cerca de 4,3 ms, sem bloco lento; a alimentação de streams pelo jogo apresenta esvaziamentos. As métricas não sustentam atribuir a demora somente à ISO, à capacidade do tablet ou ao mixer.

Há 78.098 chamadas de submissão Vulkan em 660 s. O tempo host acumulado dessa operação chega a aproximadamente 96 s, incluindo esperas; a operação de espera soma aproximadamente 62 s. **Esses tempos se sobrepõem e não devem ser somados.** Compilar shaders soma menos de 0,6 s. O processamento do código original também continua custoso. [Resumo sanitizado](evidence/runtime-progress/tablet-mission-05.json).

## Rotas corrigidas

| Consumidor | Caminho 0.6 e condição de conclusão |
|---|---|
| Leitura de visibilidade | Processa os comandos guest pendentes; consulta resultados reais e submete somente reports novos. Leituras repetidas não criam submissões vazias. Status permanece pendente até a GPU concluir. |
| Submissão parcial | Fecha o command buffer e avança o pool de buffers, respeitando fences/timeline antes de reutilizar memória, descritores ou queries. O contador de apresentações avança apenas no Present real. |
| Fence e espera explícita | Primeiro processam comandos anteriores da fila guest. O token corresponde à submissão real; consulta/espera usa conclusão GPU e publica reports concluídos. |
| Device busy | Processa comandos pendentes, encaminha trabalho real e consulta a conclusão. Não usa o retorno constante de pronto herdado do backend básico. |
| Bloqueio/consulta de recurso | Processa comandos anteriores antes de sincronizar, aguardar ou permitir acesso ao recurso. As fontes de vértices são copiadas para staging pelo renderer. |
| Lock 2D com NOOVERWRITE | Processa comandos anteriores, preserva o caminho de readback tardio e marca o recurso alterado para o cache perceber as escritas posteriores da CPU. |
| Bind de textura | Verifica também o epoch de escrita dentro da mesma apresentação. Alterações já registradas por locks não ficam presas ao conteúdo antigo até o próximo Present. |

Outros backends mantêm o comportamento síncrono por padrão. As esperas explícitas de idle, readback e reutilização segura de memória continuam necessárias. A otimização não inventa contagens de oclusão, não pula desenhos ou avança o relógio do jogo artificialmente.

## Falha de textura reproduzida

Uma fixture autoral desenha uma textura opaca e depois a torna transparente, marcando o recurso alterado, sem apresentar um novo frame. O renderer anterior continuava usando alpha antigo. O controle negativo falhou com `Texture lock update within one presentation reused stale alpha`. Com o epoch de escrita incluído na invalidação, o segundo desenho produz zero amostras, como exige o alpha test.

É uma falha gráfica comprovada do cache. Isso **não identifica todos os defeitos visuais relatados na missão**. A descrição/imagem específica desses defeitos continua necessária para separar geometria, textura, iluminação e efeito da baixa velocidade.

## Verificação executada

- Contagem real, profundidade, alpha rejection, textura alterada no mesmo frame, snapshots e oito reports enfileirados: modos direto e com render thread.
- 128 consultas/flushes sem novo trabalho não geram submissões vazias nem apresentações falsas.
- Fence submete o trabalho anterior, espera sua conclusão real e entrega o report de 16 bytes, preservando o DWORD de guarda.
- A mesma fixture executou com timeline/dynamic rendering e em uma cópia temporária forçada a usar fences/render passes. O caminho com fences é relevante porque o tablet anuncia Vulkan 1.1. O teste usa SwiftShader; não é validação do driver Adreno.
- Renderer compilado para x86_64 e AArch64, incluindo contrato da janela Android. O código do renderer no host real e na biblioteca Android foi comparado aos contratos correspondentes.
- Nove probes de runtime passaram, incluindo teste que compila os corpos HLE reais de sincronização contra spies autorais para verificar a ordem antes de recursos/fences/idle e a invalidação no lock NOOVERWRITE.

As falhas de montagem de patch e compilação das primeiras fixtures foram preservadas em logs privados; não foram tratadas como testes aprovados. O controle negativo de textura é uma falha intencional separada dos resultados positivos. [Recibos dos contratos](evidence/runtime-progress/vulkan-contracts-06.json).

## Medição Linux e limite da conclusão

No ensaio anterior ao último ajuste de ordem dos comandos, um snapshot aos 110 s mostra **63.189 consultas e 81 submissões novas para reports**. Chamadas de verificação de espera caem de 49.740 para 5.547 em comparação com a execução de referência. Essas contagens incluem verificações que retornam imediatamente.

O tempo acumulado de espera nesse snapshot foi de cerca de 32,0 s para 28,7 s. Ambos os testes limitados de 120 s apresentaram o frame 1.800; **não houve ganho geral de FPS demonstrado nesse ensaio com GPU de software**, nem melhoria comprovada no tablet. Houve remoção verificável de submissões/sincronizações repetidas. [Dados e ressalvas](evidence/runtime-progress/vulkan-sync-06.json).

O renderer mantém resolução, shaders e recursos originais. O restante do desempenho, defeitos de geometria/iluminação, progressão da missão, áudio sob carga e saves ainda precisam de validação.

## Próximo ensaio no Pad 6

Instalar 0.6 por cima, preservando dados e ISO. Neste teste, **desativar Cache de carregamento experimental**, para usar a compressão original dos recursos; Português pode permanecer ativo. Abrir a mesma missão e, após entrar, tentar movimentação/câmera por 30–60 s e exportar pelo ⋯. O modo Controle Xbox completo fica no menu do HUD.

`UC-GPU-006` registra consultas, submissões efetivas, flushes que evitaram submissão vazia e atualizações de textura dentro do mesmo Present. `UC-GPU-005` acrescenta operação 9, polling de visibilidade. Um flush explícito sem novos comandos ainda pode esperar trabalho anterior: o contador não significa que uma espera necessária foi omitida.

**O APK 0.6 ainda não foi executado no tablet. Fluidez e missão plenamente jogável não estão comprovadas.**


## Artefato privado e ensaio final

O ensaio final com a ordem de comandos corrigida durou 180,041 s e apresentou o frame 1.920, com abertura 3D original, nenhum pedido de recurso ausente e nenhuma exceção registrada. Os 16 eventos de visibilidade são limitados pelo logger; não são o total de queries. Houve compilação concorrente durante parte desse ensaio: é validação de execução sob carga, **não benchmark de FPS**. Encerramento programado por SIGTERM/-15, não término de missão. [Recibo](evidence/runtime-progress/game-ordered-06.json).

APK 0.6, build `experimental-bef2a30e191f2ccc`, versionCode 6: **28.252.088 bytes**, SHA-256 `b91a03bd8db270f5b0870b4cd25e3f5d615464411df5fdff86d25b5a9a2d0411`. Assinatura comparada com 0.5; ELF/bibliotecas, alinhamento e três partes de transporte verificados. Fontes/dicionário foram congelados durante a montagem. [Recibo do APK](evidence/runtime-progress/android-renderer-06.json).

A primeira montagem 0.6 não foi entregue: a revisão encontrou a necessidade adicional de ordenar comandos antes de recursos/fences, e a biblioteca/APK foram reconstruídos. Seu recibo e logs foram preservados como superseded. Nenhum APK ou recurso comercial foi publicado no repositório.
