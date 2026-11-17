# Progresso do runtime — 2026-11-17

**GO para pesquisa: intro, menus e primeira cena da abertura de Hope Street renderizados no Linux. REQUIRES_REVERSE_ENGINEERING para concluir a primeira missão e o runtime Android. Nenhum APK do jogo foi produzido.**

A variante Xbox do usuário (`c6f0c99a…044d1`) permanece privada. Este avanço usa o C++ gerado desse executável e o runtime MIT do SDK pinado. Não usa lógica substituta, hooks de Black ou adaptações Burner sem licença esclarecida. O renderer de teste/referência e o sink de áudio não constituem backend final de produção.

## Falhas investigadas e mudanças

| Falha real | Correção / evidência |
|---|---|
| AddRef DirectSound acessava objeto incompatível | Referências e vida útil do dispositivo geridas pelo runtime; oito chamadas HLE com criação repetida, liberação e balanço stdcall passaram |
| Shader FVF nulo na inicialização D3D | Binding `D3D_FvfShader=003F68B8` comprovado em duas rotinas originais; inicialização já existente do SDK passa a preencher o objeto correto |
| SetViewport recebia nullptr e era desreferenciado | Reaplica viewport atual, como o caminho original; teste preserva viewport customizado e seus dados espelhados |
| IsBusy acessava registradores de GPU inexistentes | HLE consulta ocupação real do backend; teste com worker bloqueado verifica idle, pendência e conclusão sem espera bloqueante |
| Vértices XYZRHW perdiam seu modo na expansão imediata | Modo pretransformado preservado no shader e chave de cache; teste verifica posição, RHW e diferenciação dos modos |
| GetVoiceProperties de buffer e SetPitch de buffer/stream seguiam código original sobre objetos host | Consulta de 92 bytes reflete mixer real; pitch altera frequência de resampling compartilhada; limites e escrita limitada testados |
| Restauração de contexto retornava ao chamador C++ errado | Geração optativa de continuações e dispatcher de retornos não locais; [decisão e regressão](CONTEXT_SWITCHING.md) |
| Criação de stream pelo dispositivo seguia código original sobre objeto host | Nova entrada HLE usa a mesma implementação real de buffers/packets/callbacks do SDK; criação, AddRef, liberação e argumentos inválidos passaram no probe |
| SetMode 3D seguia código original sobre objeto host no carregamento da missão | Operação implementada sobre modo espacial do mixer, validação dos argumentos e aplicação adiada no CommitDeferredSettings; teste verifica mudança real de atenuação |
| Push buffers gravados acessavam estado de GPU Xbox | Parser de BEGIN/END, índices de 16 bits, índices de 32 bits convertidos sem perda dentro do limite do backend e NOP; primeiro draw real confirmado, demais métodos/fixups ainda falham explicitamente |
| Conclusão de leitura assíncrona abortava no export de kernel 232 | NtUserIoApcDispatcher entrega o callback real via fila APC; teste verifica entrega adiada, exatamente uma vez, status, bytes, IOSB e preservação de pilha/registradores |
| Saída padrão de stream em `0x00426156` ainda acessava objetos Xbox | Entrada própria aceita destino nulo, comprovado no consumidor; buses não nulos retornam E_NOTIMPL até existir roteamento real |

As adaptações são patches explícitos em `patches/runtime/`. `vendor/runtime` permanece no commit original, sem alterações. Os bindings ficam em `config/research/` e valem exclusivamente para o hash documentado; não devem ser usados com outra variante.

A disposição binária de DSVOICEPROPS foi conferida contra as escritas/leitura originais e, como referência de interface, o header Cxbx listado em `dependencies.lock.json`. Nenhuma implementação GPL foi incorporada. O SDK ainda não implementa todo o DSP, cone, Doppler e efeitos I3DL2. A consulta reporta as contribuições efetivamente aplicadas; isso não comprova fidelidade completa do áudio.

## O que foi observado no título

Execuções privadas mostraram os vídeos originais, “PRESS START BUTTON”, MAIN MENU, opção Singleplayer e seleção de dificuldade. START e A foram enviados pelo harness de controle já existente do SDK, sem alteração de estado ou lógica guest. As capturas de referência foram inspecionadas privadamente; não foram publicadas como assets do projeto.

Na versão anterior à correção de contexto, o carregamento da missão falhava em `0x002012C0`. Após corrigir também o dispatcher de callbacks, o teste avançou até cerca de 69 s / frame 1740 e revelou SetMode em `0x0042601C` / interno `0x004254D1`, sem recursos solicitados ausentes. As correções seguintes passaram esses pontos: o teste com SetMode expôs a criação de stream pelo dispositivo; a execução com ambas as APIs chegou a cerca de 101 s / frame 1740 e falhou em `0x00426156`, interno `0x00425332`, ao acessar estado de saída de áudio que não existe no objeto host. Nenhum arquivo solicitado estava ausente. Um observador que apenas encaminha a chamada confirmou destino nulo: esse caso mantém a saída direta nos mix bins já implementada pelo mixer. Foi adicionada uma entrada de stream para esse caso; destinos de bus não nulos retornam E_NOTIMPL explícito, pois exigem grafo de roteamento real. Esse caso passou na execução real seguinte, que revelou a operação de pitch do stream em `0x0042503E` / interno `0x00424088`. O consumidor original `0x00251A50` passa o mesmo pitch à versão de buffer e à versão de stream, comprovando a semântica; a implementação compartilha o cálculo de frequência já testado. A execução seguinte passou o pitch e abortou no export de kernel 232, NtUserIoApcDispatcher, aos 70 s. Esse export foi implementado e passou no teste com fila/callbacks reais do SDK. Na execução posterior, o jogo passou essa entrega e revelou a execução de push buffer gravado em D3D `0x003EF870`, chamada por `0x000B4EB0`, aproximadamente aos 72 s. O SDK não fornece esse caminho completo de comandos de GPU. Foi implementado e testado um subconjunto de draws indexados gravados, que envia os índices ao backend real e rejeita comandos não modelados. O primeiro buffer submeteu um draw real; o seguinte expôs ARRAY_ELEMENT32. Esse método recebeu conversão sem perda dentro do limite do backend e novo teste, sem truncar índices maiores. Com isso, pelo menos oito buffers foram submetidos ao backend. Esse foi o bloqueio histórico do trailer, corrigido na investigação seguinte; a cena da missão ainda não foi comprovada. [Próxima integração](PUSHBUFFER_INTEGRATION.md). Não foi habilitado o stub de sucesso do buffer como se fosse suporte completo.

O mixer produziu milhões de amostras não nulas, sem valores não finitos nos testes observados. O sink de pesquisa descarta a saída após medir: **não houve audição, saída SDL3 ou validação de sincronismo**. Houve acesso a perfis e arquivos originais de save, mas isso **não valida checkpoint ou carregamento de progresso após reiniciar**.

94 recursos do disco foram extraídos progressivamente e tiveram SHA-256 conferido, incluindo os pacotes necessários às tentativas do menu/primeira missão. A ISO completa não foi baixada nem teve seu hash integral validado. Nenhum asset, captura, código gerado ou log bruto do jogo entrou no Git.

## Compilação e testes executados

- 46 testes Python e 11 checks Java: [log](evidence/runtime-progress/tool-tests.log).
- Dois testes nativos GPU e um teste de pacotes gravados autorais sem dados do título: [GPU](evidence/runtime-progress/gpu-probes.log) e [pacotes](evidence/runtime-progress/recorded-packet-tests.log). O fixture de pacotes também [compilou em AArch64](evidence/runtime-progress/recorded-arm64-contract.log).
- Fixture autoral de troca de contexto: controle negativo original retorna 2989; implementação corrigida e callback aninhado retornam 42. [Resultado](evidence/runtime-progress/context-result.json).
- Seis probes HLE/estado do SDK no host de pesquisa: áudio, viewport, busy, vértices, voice properties/streams e APC, todos exit 0. Não executam instruções do título; os logs desse host permanecem privados por usarem metadata da variante.
- Geração com continuações: 29.049 funções, 83.240 continuações, duas saídas idênticas; 129 unidades C++ compiladas em x86_64 e host ligado.
- Geração **baseline**: todas as 129 unidades também compilaram para Android ARM64 API 29, NDK `28.2.13676358`, CMake `3.31.6`. [Inventário](evidence/runtime-progress/baseline-arm64-result.json). Isso é um arquivo estático AArch64; não runtime Android ligado, APK ou execução no tablet. A geração posterior **com continuações** também compilou: 129 objetos AArch64, sem erro. [Inventário separado](evidence/runtime-progress/resumable-arm64-result.json). Nenhum dos dois arquivos está ligado a um host Android.

## Reprodução privada do host

```bash
python3 scripts/build_context_tools.py
# Gerar com o tool e configuração privada revisada, contendo [xbe] resumable=true.
scripts/compile_private_generated.sh build/title-generated build/title-linux linux build/context-tools-patched/sdk/runtime
python3 scripts/build_research_runtime.py \
  --xbe private/game/Default.xbe --generated build/title-generated \
  --archive build/title-linux/liburban_generated_compile_probe.a \
  --profile config/research/user-xbe-resumable.json --out build/title-host
# Executar somente com dados legítimos em ambiente privado; nunca no PR público.
build/title-host/cmake/urban_research private/game
```

A receita não cria configuração qualificada ou integration-status fictício. Os scripts exigem paths ignorados, pin limpo e saída nova; registram hashes de fontes, patches, profile e arquivo estático. Os arquivos scanner/configuração específicos precisam ser revisados antes da geração; não são automaticamente a configuração privada da Burner.

## Próximos gates

Validar a continuação da missão e os consumidores de input; ampliar o consumo real dos push buffers onde necessário, preservando estados e draws; concluir a carga original da missão e corrigir falhas seguintes com evidência; validar memória/ABI/callbacks além dos fixtures; executar missão, câmera, armas, escudo, áudio e save/load. Depois ligar Vulkan/SDL3/Android ARM64, validar lifecycle e importação, e testar no Pad 6. Windows permanece opcional como comparação, sem ser entrada do APK. Não há medição de desempenho Android ou promessa de FPS.

## Avanço: trailer XDK, reports reais e novo bloqueio

A análise do driver original demonstrou que a última palavra do buffer é reservada para linkage de submissão, reescrita pelo driver ou excluída no ramo de cópia. A integração passa apenas o payload ao parser; saltos dentro dele continuam rejeitados. Testes preservam a ordem de dois draws e todos os índices com diferentes valores de trailer, rejeitam saltos internos e truncamentos. [Evidência e decisão](GPU_LINKAGE.md).

Após essa correção, a thread esperava consultas de visibilidade nunca concluídas pelo backend headless. Foram implementadas contagens reais no rasterizador de referência e entrega de reports de 16 bytes; não se fabricou sucesso ou contagem. Quatro CTests passaram, incluindo geometria autoral visível/ocluída, profundidade, alpha/texture clip, máscara de cor, reset e execução com thread. [Detalhes](REFERENCE_VISIBILITY.md) e [resultado sintético](evidence/runtime-progress/reference-visibility-tests.json).

Testes privados de cena limitados a 360 s concluíram consultas reais, mas permaneceram sem a apresentação seguinte ao frame 1740. A amostragem posterior mostrou repetição na rotina original de inserção de árvore `0x000B9A10`, com contagem de um e encadeamento de pais repetido; a origem precisa ser rastreada. Não há prova de missão carregada, execução de armas/escudo, save/load ou runtime Android. O modo de referência sincroniza as consultas e executa rasterização na CPU: não é medição de desempenho no Pad 6.

Um host novo foi compilado de fontes e overlays públicos, sem observador privado, com receipt de fontes/patches/arquivo guest. Os seis selftests passaram nesse executável: [inventário sanitizado](evidence/runtime-progress/clean-reference-host.json). O binário e o arquivo guest derivam do XBE e permanecem privados. O arquivo estático Linux baseline antigo foi removido por falta de espaço; a versão com continuações usada no build foi preservada.

## Retenção de superfícies e primeira cena original

A investigação rastreou o sentinela duplicado até uma lista de heap corrompida após double free. O alocador x86 original repetiu o resultado do C++ com o mesmo estado capturado; não foi substituído. A busca no port PC identificou o contrato de retenção pública de superfícies. A rotina original foi conferida no XBE e ligada aos três getters: [diagnóstico, correção e controle negativo](RESOURCE_LIFETIME.md).

O jogo passou pelo travamento anterior e expôs cone de áudio e depois fatores espaciais ainda acessando objetos Xbox. Seis interfaces de buffer foram ligadas ao mixer; cone, rolloff, velocidades e Doppler agora têm efeitos reais e commit de atualizações adiadas. [ABI, modelos e limites](SPATIAL_AUDIO.md). O build limpo intermediário falhou por duas declarações duplicadas de lock; foram corrigidas, e o host limpo seguinte compilou e passou nos seis probes. O erro intermediário não foi classificado como sucesso.

Na execução limpa limitada a 180 s, o jogo chegou ao frame 1800. A captura 640×480 foi inspecionada: sequência original de Hope Street Police Station, cenário e policiais visíveis. Não houve nova exceção registrada ou recurso solicitado ausente nesse ensaio. A execução terminou por timeout controlado, não por conclusão de missão. Há defeitos visuais no renderer CPU. [Receipt sanitizado](evidence/runtime-progress/surface-spatial-host.json); imagem, logs e dados derivados permanecem privados.

Oito CTests autorais passaram (incluindo controle negativo do getter antigo e modelos de cone/fatores); os contratos de áudio também compilaram AArch64. O áudio continua descartado pelo sink, sem validação audível de músicas/vozes/efeitos. Input durante gameplay, checkpoint, morte/load, transição de missão e campanha não foram aprovados. [Rota de reaproveitamento PC](PC_REUSE_MATRIX.md) e [contratos/runner de ensaios privados](RUNTIME_CONTRACTS.md).

## Ensaio de continuação: 600 s

O host limpo avançou até o frame 1920, sem nova exceção registrada ou recurso solicitado ausente. A captura mostrou o menu original **PAUSED: HOPE STREET**, acionado pelo START do roteiro. O timeout ocorreu antes dos probes de movimento/trigger agendados para 2280/2400; eles não foram executados nem classificados como aprovados. [Registro sanitizado](evidence/runtime-progress/mission-progression.json).

Amostragem nativa em execução separada encontrou o maior grupo de amostras em `reference::Draw` e avaliação de Combiners/BlendFactor. As apresentações esparsas não foram atribuídas novamente ao antigo laço da árvore. Isso é diagnóstico do custo do renderer CPU, sem medição Android e sem promessa de FPS. [Amostras agregadas](evidence/runtime-progress/scene-cpu-sampling.json). A implementação de reports Vulkan e a compatibilidade de canais de áudio precisam ser resolvidas antes de aproveitar esses backends de Black: [contratos encontrados](COMPATIBILITY_MATRIX.md).

Para preservar o executável limpo enquanto se liga um observador privado separadamente, foi mantido um hard link com o mesmo SHA e seu receipt em diretório ignorado. O antigo arquivo estático baseline ARM64 foi removido para liberar espaço; o arquivo ARM64 com continuações continua preservado. Nenhuma entrada original foi removida.
