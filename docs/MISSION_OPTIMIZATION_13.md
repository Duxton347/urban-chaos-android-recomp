# Missão — custo de CPU, cache e disparo sobre o alvo nativo

Investigação de 2026-11-26/27. As alterações abaixo preservam a ISO e a lógica
original; não qualificam ainda uma missão fluida no Pad 6. Não foi entregue outro
APK para pedir ao proprietário que repita um defeito conhecido.

## Referências e decisões

Black `8e9c6fc7d570503ea68d039cff16c09339ee6d30`: inspecionados
`native/vk_shader.cpp`, `native/vulkan.cpp` e `src/d3d/render_thread.cpp`.
O cache de módulos e a separação do trabalho gráfico são relevantes. Nosso overlay
mantinha `vkshader::Init({})`, portanto recompilava shaders em cada processo.
O worker do SDK também existe, mas estava desativado. Foi exposta uma opção
explícita `URBAN_GPU_WORKER`, **desligada por padrão**: copiar filas sem validar
locks, readbacks e vida dos recursos não comprova correção nem desempenho.

Burner `bdaef722ef1d589af30e5a58dd8543f64564c4fc`: a auditoria mantém os
limites de reprodução e licença registrados em RECOMPILATION.md. Endereços,
consumidores e contratos deste trabalho foram verificados no XBE privado do
proprietário e no guest gerado; nenhuma implementação própria da Burner foi
transplantada. O runtime continua no pin
`c1f7d60c8728c6151bcfbb1d05e397eaf0618dc7`.

O relatório anterior já separava IO da execução: leituras da ISO não explicavam
a thread principal ocupada e as baixas apresentações. Esta alteração atua no
custo de compilação de shaders e nas transferências do código recompilado;
não troca qualidade por resolução menor ou remoção de efeitos.

## Cache de shaders

`native/gpu/shader_cache.h` implementa um cache descartável em armazenamento
privado. A chave inclui SHA-256 do compilador/tradutor pinado, fonte HLSL exata,
estágio e entry point. O arquivo verifica tamanho, hash e estrutura SPIR-V antes
da leitura. Arquivos inválidos voltam à compilação; escritas usam temporário e
rename. Limites: 8 MiB por módulo, 64 MiB e 1024 arquivos por versão.

O cache do tradutor original continua desligado; o novo cache envolve a criação
dos módulos. Não contém imagens do disco e não integra o relatório exportado.
`UC-GPU-006` distingue acertos, rejeições e compilações. No Android a raiz fica em
`cache/shaders/modules`; no host só é ativada com `URBAN_SHADER_CACHE_ROOT`.

Resultado executado com shaders autorais e Vulkan real (SwiftShader): rodada
fria com **31 compilações e 16 acertos**; rodada quente com **zero compilações e
47 acertos**. Os contratos de pixels continuaram aprovados em timeline/dynamic
rendering e em fences/render passes. Isso comprova reaproveitamento de módulos,
não elimina a criação de pipelines do driver nem mede FPS no tablet.

## Função original de desenho

`config/research/mission-render-frame-fusion.json` delimita a função original
`0008FF30..00090530`. O perfil privado encontrou 79.851 transferências originadas
em `00090147` numa execução anterior. `tools/fuse_frame_cfg.py` reúne **36 blocos
e 61 arestas internas** com gotos locais, preservando chamadas reais, pilha,
flags, retorno revisado e propagação de transferências não locais.

As entradas externas e os mapeamentos de retomada permanecem disponíveis; só o
corpo da entrada original é substituído. Fonte, limites e grafo são verificados
antes de reconstruir a unidade 14. O script recusa estruturas não revisadas.
Foram compilados os arquivos reais Linux e AArch64. Nos ensaios privados que
alcançaram a abertura 3D, a origem `00090147` deixou de gerar transferências.
Outras origens continuam sendo medidas, sem suprimi-las globalmente.

Os contratos C++ autorais comparam chamadas, caminhos, retornos alterados,
entrada independente em fragmentos, propagação de retomadas e serialização
dos flags que a transferência removida gravava antes do unwind. Os ensaios reais
usam SwiftShader; parte deles também coleta perfis de exceções: não são um benchmark comparável de GPU Adreno.

## Disparo sem IA adicional

O campo que colore a mira em vermelho, `player+0x230`, **não indica inimigo**:
o produtor original registra dano recebido. A hipótese anterior foi descartada.

O jogo já faz o raycast e classifica o alvo em `000C1020`. A classificação usa
as propriedades nativas de personagem/incapacitação e a lista original de
aliados (`00039900`) ou equipe. O adapter não procura inimigos por imagem nem
movimenta a câmera.

O hook observa a entrada em `000C77D0` pelo retorno `000C7CB7`, imediatamente
após a classificação do raio exato. A assistência mais ampla ocorre depois;
seu candidato sozinho não autoriza disparo. O adapter acrescenta pulsos ao RT
no mesmo `GameInfo::pad` usado pelo controle real, somente se o mesmo alvo ainda
está classificado como hostil. A lista de bindings original deve confirmar RT
como tiro (ação 7) e LT como escudo (ação 6); esquemas desconhecidos ou conflitantes
são recusados. O jogo continua processando a entrada, munição, cadência e dano.

Guardas adicionais: hash exato da revisão, jogador local, identidade e geração
do registro de entidades, vida, ausência de destruição, arma de fogo revisada,
estado normal revisado, controles habilitados, ausência de escudo/pausa/modal e observação
de no máximo um tick anterior. Alvos aliados, incapacitados, explosivos e cenário
são recusados. A classificação e a identidade são conferidas novamente antes de entregar o
controle, sem conservar uma autorização antiga ao trocar de alvo.

O botão TIRO manual permanece e seu valor original tem prioridade. A opção é
persistente e começa desligada, acessível em ⋯ → Tiro automático. Não há novo
botão sobre a cena. Edição, diálogos e perda de foco cancelam a autorização.
`UC-INPUT-003` exporta contadores agregados de observações, pedidos e bloqueios;
um pedido de input não é contado como projétil efetivamente disparado.
`input_route=2` identifica a integração pelo Pad, e `queries` conta essas leituras.
Snapshots de alvo e pulsos são sincronizados entre threads, sem buscar entidades
por varredura; cada leitura confirma a identidade pelo registro original.

Uma primeira tentativa de observar o retorno de `000C1020` foi rejeitada: as
retomadas não locais podiam contornar o wrapper. Observar o getter do HUD evitou
isso, mas lia o alvo já influenciado pela assistência. A observação na entrada
da etapa seguinte ao raio elimina ambas as dependências.

A ligação anterior ao retorno da consulta de ações também foi descartada:
um RT manual consumiu uma bala (15→14) sem incrementar seu contador de entrada
manual. A rota atual entrega o mesmo RT pelo Pad, preserva o valor manual e deixa
a consulta e suas retomadas completamente originais. O tiro manual observado
não é apresentado como prova de um disparo automático contra inimigo.

## Verificação e limites de entrega

Comandos principais:

```sh
python3 -m unittest discover -s tests -v
python3 scripts/test_mission_controls.py
python3 scripts/test_diagnostics.py
python3 scripts/build_vulkan_renderer.py --android-window
python3 scripts/publication_check.py
```

Os ensaios de política usam memória e funções autorais: aprovam bloqueio de
aliados, alvo antigo, assistência sem acerto exato, estados incompatíveis,
escudo, bindings incompatíveis, prioridade manual, entrega concorrente e pulsos. Não equivalem a jogar a campanha.
O script de pesquisa agora recebe a extensão de joystick direito por um patch
reproduzível do SDK, com teste de direção, duração, consumo e ausência de tiro
implícito. Ensaios limitados por tempo que não alcançam a sequência de mira não
qualificam o gatilho.

Os registros privados preservam receitas, hashes, falhas e capturas; dados
comerciais, código gerado e capturas não são publicados.

A renderização e o desempenho da missão ainda exigem validação comparável.
Esta investigação não declara eliminados os defeitos visuais relatados pelo
proprietário nem promete FPS. O worker continua experimental e desligado.
