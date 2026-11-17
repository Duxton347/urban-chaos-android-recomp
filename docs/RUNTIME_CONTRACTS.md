# Contratos a preservar até Android

Este índice liga produtores e consumidores aos testes existentes. Não qualifica gameplay ausente. [Estado atual](RUNTIME_PROGRESS.md), [reaproveitamento PC](PC_REUSE_MATRIX.md) e [HUD e ações](HUD_SPEC.md) evitam repetir a auditoria a cada build.

| Fronteira | Contrato | Evidência atual / teste que falta |
|---|---|---|
| XBE → gerador | Hash da variante, limites de funções, imports e ABI explícitos; geração determinística | 29.049 funções e 83.240 continuações; qualificação completa G1 aberta |
| CPU → chamadas/callbacks | Ponteiros guest de 32 bits; balanço de pilha; registradores não voláteis; retorno não local pelo endereço e slot guest | Fixture autoral e APC passam; rastreamento do alocador revela devolução do sentinela ainda utilizado na carga |
| Memória → objetos do título | Um bloco vivo não pode ser entregue novamente; liberar somente pelo dono e depois do último consumidor | Primeira corrupção observada após double free; contrato de retenção de superfícies corrigido, com controle negativo e nova execução. [Registro](RESOURCE_LIFETIME.md) |
| D3D → fila/backend | Ordem de estados e draws; dados da fila sobrevivem ao retorno do produtor; linkage XDK separado do payload | Draws gravados e reports reais passaram; primeira cena não apresentada; métodos não suportados falham explicitamente |
| GPU → consulta original | Contar amostras após rejeições; status publicado depois dos dados; profundidade válida | Quatro CTests incluindo visibilidade; CPU de referência sincroniza para diagnóstico. Vulkan ainda precisa produzir resultados reais |
| Input host → XInput original | `input::Pad`: botões digitais, oito botões analógicos 0–255, sticks `int16_t`, conexão por porta | START/A chegaram aos menus. Movimento, câmera, tiro, escudo e ordens exigem execução em missão; slots lógicos em HUD_SPEC não são ainda bindings qualificados |
| Mixer → saída | PCM estéreo real; pitch/frequência e callbacks coerentes; mesmo relógio de amostras; não misturar saves/configuração | Mixer fornece amostras finitas e não nulas; cone, rolloff e Doppler implementados/testados em pesquisa. Sink atual descarta som. [Limites](SPATIAL_AUDIO.md). Música, vozes, efeitos, sincronismo e SDL3 ainda precisam de validação audível |
| Arquivos → recursos | Preservar caminhos Xbox, offsets, tamanhos e conclusão assíncrona; nunca publicar assets | Recursos solicitados nos últimos testes presentes; importador validado separadamente. Novas missões podem solicitar outros arquivos |
| Progresso → armazenamento | Checkpoint original persistente após encerrar/reabrir; atualizações preservam saves | Escritas de perfil não equivalem a save de missão. Save/load de progresso ainda não validado |
| Runtime → Android | Guest mantém ABI 32 bits sobre processo AArch64; Vulkan/SDL3/SAF/lifecycle recebem os mesmos contratos | 129 unidades guest compiladas AArch64; sem runtime Android ligado, APK do jogo ou teste no Pad 6 |

## Registro de cada ensaio privado

Guardar em diretório ignorado: hashes de XBE, host, arquivo guest, profile e patches; comando e variáveis explícitas; script de input; limite e duração; motivo de saída; recursos solicitados ausentes; último frame observado; primeira chamada divergente; reprodução e próxima hipótese. Logs brutos, capturas e saves ficam privados. Publicar apenas metadados e regressões autorais.

## Ordem de aceitação

1. Cena original apresentada e câmera coerente, sem ignorar falhas.
2. Movimento, mira, fogo/recarga, escudo, interação e ordens pelos consumidores originais.
3. Música, vozes e efeitos audíveis, sem amostras inválidas ou regressões de pitch/loops.
4. Checkpoint, morte e load, encerramento/reabertura e transição de missão.
5. Repetir os mesmos cenários no Android com Vulkan/SDL3, incluindo suspensão e desconexão de controle.

A ausência de exceção ou um timeout não prova nenhuma dessas etapas. Testes de cena lentos no renderer CPU não são medição de FPS Android.

## Receita de execução com registro

```bash
python3 scripts/run_private_research.py \
  --host build/title-host/cmake/urban_research \
  --game private/game --profile config/research/user-xbe-resumable.json \
  --out build/private-runs/scene-001 --seconds 180 \
  --pad-script private/campaign-select.pad --reference-from 1720 --capture-every 120
```

A fronteira 1720 pertence ao ensaio atual e deve ser revista se o fluxo de menus mudar; não é configuração universal. O runner exige saída nova, entradas ignoradas, hash do XBE compatível com o profile e receipt de build correspondente; recusa hosts com observador privado configurado. Ele registra hashes, ambiente, timeout, saída, último contador de apresentação e reports concluídos em `run.json`. O log integral e as capturas continuam no diretório privado/ignorado. Timeout retorna 124 e nunca marca missão como validada. Um teste real de 3 s verificou encerramento controlado e receipt; um caminho de host fora das áreas ignoradas foi rejeitado antes da execução. Esses são testes do runner, não de gameplay.
