# Hope Street: relatório completo e controles por toque

2026-11-25. **Missão continua sem qualificação de jogabilidade. Nenhum APK novo entregue nesta investigação.** O proprietário relata conclusão de Hope Street, com lentidão e corrupção visual; isso comprova a progressão relatada, não qualidade aceitável.

## Relatório analisado

[Resumo sanitizado](evidence/runtime-progress/tablet-mission-11.json). SHA-256 do ZIP: `3ce7bcd53d819ef3480a7123eec60e5a310439f539c11807298bb52a2a3b34a8`. Build `experimental-315c00c19ca570e2` (0.10), duração registrada 2666,432 segundos. Há três sessões no ZIP: somente a sessão com UTC mais recente entra nas estatísticas. Logs rotacionados são ordenados pelo tempo decorrido.

O último intervalo, 55,71 apresentações/s, não representa a missão. Medianas de intervalos nos blocos de cinco minutos: 56,391; 6,817; 10,391; 11,749; 11,790; 10,826; 10,787; 10,803; 5,700. São apresentações, sem marcador validado que separe menu, vídeo, loading e gameplay. Os intervalos são agrupados pelo instante final; não constituem médias ponderadas dos cinco minutos completos.

A thread principal (slot 1) tem medianas de 87,2% a 94,6% de um núcleo por bloco. Entre o primeiro e último relógio válido, são 2364,994 segundos de CPU em 2650,581 segundos de parede (89,226% de um núcleo). Isso aponta para grande custo CPU na execução; não demonstra qual função é responsável, utilização total do SoC, frequência, throttling ou duração GPU. O estado guest frequentemente mostra o wrapper de início da thread; chamadas diretas executam abaixo dele sem atualizar essa etiqueta. Não tratar essa etiqueta como um perfil de instruções.

3283 leituras, 171741536 bytes, 903716 microssegundos acumulados, máximo de 8165 microssegundos, nenhuma pendente no snapshot. Todos os 2722 APCs enfileirados foram entregues. Não há evidência de que extrair outra ISO ou baixar arquivos extras resolva esses trechos lentos. Isso não valida cada recurso nem a correção de sua interpretação.

16675887 draws nativos e zero draws inteiramente convertidos confirmam que o caminho entregue foi exercitado. Foram enviados 58990768896 bytes de vértices durante a sessão. Houve 23111 conversões/binds depth→cor. A correção anterior não tornou a missão aceitável. O relatório não registra imagens intermediárias por pass ou tempos GPU por timestamp, portanto não determina sozinho a origem da corrupção.

Os totais de tempo host são cumulativos e sobrepostos: draw inclui texturas/pipeline; readback inclui submit/espera. Não somar essas categorias nem chamar seu total de tempo GPU. Também não interpretar o platô de aproximadamente 12 apresentações/s como limitador intencional sem localizar um limitador real no caminho executado.

## Alterações implementadas no HUD

- Escudo (LT) e agachar (L3) alternam ao tocar. Soltar o dedo mantém a ação; outro toque libera. Dois dedos simultâneos no mesmo botão não alternam duas vezes.
- Um botão de ordens começa em D-pad Down. Arrastar a partir do ponto inicial escolhe Up/Down/Left/Right, com uma direção por vez; soltar cancela. Mantém-se visualmente pressionado em qualquer setor.
- R3 mantém a ação original de mira/zoom. Aux 1 mantém o botão Black, identificado como lanterna/secundária conforme o contexto. Ícones vetoriais próprios identificam escudo, agachar, mira, lanterna e respirador; A/B/X/Y permanecem.
- Aux 2 é White, correspondente ao respirador térmico no esquema padrão descrito pelo port de PC. Evidências: tabela de ações em `xbe_controls_ui.inc`, identidade `action+1` em `xbe_controller_bindings.inc` e seis primeiras entradas analógicas A/B/X/Y/Black/White no contrato SDK. Os textos originais do usuário também identificam o respirador. Isso não comprova disponibilidade em todas as missões ou preservação da função em outro esquema de controle. Permanece entre os extras.
- TIRO continua enviando RT enquanto pressionado, inclusive para o ataque contextual do escudo. Arrasto de câmera continua independente; não foi alterada a sensibilidade original.
- START/BACK por toque, editor, perda de foco e cancelamento liberam estados. Não há novo detector de pausa acionada por controle físico. O comportamento deve ser confirmado no tablet, especialmente se o próprio jogo estiver configurado para agachar em toggle.
- Perfil `mobile11` separado preserva o arquivo `mobile8` anterior. Restaurar padrão usa as mesmas posições do perfil novo. Não se migram silenciosamente as posições antigas para controles com novos gestos.

As fontes Burner foram consultadas como evidência de função; sua implementação de controles não foi transplantada. O roteador e os vetores são implementações locais.

## Tiro automático: consumidor ainda não integrado

O port de PC possui testes de estados nativos da mira: neutro, inimigo, aliado e outra ação contextual. A região correspondente foi localizada também no código gerado do executável privado atual. Ela usa atualização recente de um estado do jogador para a indicação de inimigo; portanto, cor vermelha capturada da tela não é um detector adequado de inimigo atual.

Ainda falta confirmar o produtor desse estado, alvo/linha de visão, arma disponível, jogador ativo e contexto pausa/cutscene. Não foi instalado disparo baseado em pixels, intervalo fixo ou endereços de outra revisão. A opção solicitada continua pendente de integração real.

Contrato de aceitação: automático opcional, desativado por padrão; arma de fogo e alvo hostil atual/visível; sem ação em aliado, cenário, menu, cutscene, morte, perda de foco ou estado antigo. RT efetivo deve combinar solicitação automática e manual, sem um produtor apagar o outro. Escudo elevado permite ataque manual, mas não ativa golpe automático. Testar a transição de alvo, mudança de arma, munição, pausa, morte e retomada antes de apresentar a opção como funcional.

## Verificação executada

`python3 scripts/test_diagnostics.py`: 33 checks HUD, 326 checks de diagnóstico Java, swipe ownership/cancelamento e contratos nativos de touch passaram. Novos casos cobrem toggles, multitouch, cancelamento, setores de ordens e RT simultâneo. Testes de análise de relatório: quatro passaram, incluindo isolamento de sessões, ordenação de rotação, reset do contador e mediana de número par de intervalos.

Compilação Java Android com o runtime privado real: `bash scripts/build_launcher_base.sh :app:compileDebugJavaWithJavac --offline -PurbanNativeBuild=<diretório privado validado> -PurbanBuildId=experimental-315c00c19ca570e2`. A primeira tentativa sem a propriedade foi recusada pela guarda de runtime; nenhuma guarda foi removida. A compilação final, incluindo todos os ícones, passou. Nenhum pacote APK foi montado.

Perfil Linux privado separado iniciado com a execução atual e amostragem de CPU. Usa Vulkan por software, conteúdo e logs ignorados pelo Git; não é benchmark do Adreno nem validação de jogabilidade. Capturas de tela periódicas foram desativadas nesse ensaio para não acrescentar readbacks de diagnóstico. O diagnóstico do tablet continua necessário para confirmar uma correção real.

A primeira amostragem Linux de 180 s atingiu o contador 1980, mas usava um timer de processo e misturava trabalhadores do driver de software; foi descartada como perfil da thread principal. Uma tentativa de timer por thread não ativou seu ponto de entrada no host limpo e foi interrompida sem ser aceita como medição. O ensaio seguinte intercepta apenas a nomeação de threads guest pelo SDK para iniciar o relógio CPU nessas threads; eventos de ativação são exigidos. Nenhuma dessas sondas é incorporada ao APK.

## Custo de continuidade e mudança restrita no código gerado

O ensaio separado de 180 s com relógios CPU por thread guest confirmou ativação dos dois timers guest e chegou à cena 3D, sem incluir threads de trabalho Vulkan no timer. Na cena aparecem consultas da tabela de continuidade e trabalho das bibliotecas de desenrolamento C++. Uma segunda execução, com contagem de lançamentos, registrou 371234 eventos até o último snapshot, por volta de 170 s. Essa contagem é de controle de fluxo/lançamentos, não de erros do jogo, e não mede sozinha a fração de tempo consumida. A sonda de lançamento não identifica o tipo de cada exceção; a atribuição a um local foi conferida nos símbolos/código privados. O ensaio não equivale a medir no ARM64/Adreno.

Um fragmento de percurso de estrutura de dados tem 67120 lançamentos nesse snapshot. Ele foi separado do predecessor e restaura um registrador da pilha antes de retornar; entrar nele com outro frame C++ desencadeia a guarda de retorno. Não é seguro retirar essas guardas globalmente. Outra origem frequente restaura uma pilha guest e seus registradores: sua transferência é necessária, portanto não foi suprimida.

A ferramenta de fusão já existente recebeu correção de higiene de labels: labels C++ têm escopo de função, mesmo dentro de blocos. Um fragmento e seu proprietário podem conter o mesmo label guest. A inserção agora renomeia definições e referências locais, preservando os destinos. A fixture executa código anterior/posterior, com colisão de labels e retorno normal/não local; a entrada dinâmica original continua presente.

A primeira revisão uniu 12 arestas de único proprietário. O perfil posterior mostrou transferência deslocada para outro fragmento: ela não foi aceita como solução. A revisão atual `config/research/mission-iterator-fusion.json` limita a mudança ao hash exato de uma unidade e ao CFG fechado de uma função, com 13 blocos/14 arestas. Laços e junções usam saltos locais dentro do frame original, enquanto os fragmentos externos permanecem inalterados. O código derivado, objetos e arquivos permanecem privados. Foram compilados os objetos Linux e AArch64 com `-O2` e ligado o host Linux real. O renderer não foi alterado nesse experimento. Isso não demonstra ainda ganho no tablet ou correção gráfica.

Foram adicionados `transfers` e `resume_steps` por thread a `UC-CPU-003`, sem exportar instruções, memória ou recursos comerciais. `transfers` conta entradas no catch do executor, incluindo recapturas; não é um contador de falhas nem necessariamente de lançamentos únicos. `resume_steps` conta tentativas de despacho de continuidade. O relatório 0.10 não tem esses campos: a ausência não significa zero. A fixture executa o loop real do SDK com retornos normais, continuação, despacho de função, destino ausente e escape entre callbacks, mantendo profundidade e restauração de registradores. Overlays sem modo resumível não recebem esse patch.

Um novo ensaio privado compara a mesma sequência de navegação com o CFG reunido. [Contratos e compilação](evidence/runtime-progress/execution-contracts-11.json). O critério imediato é eliminar transferências evitáveis desse fragmento, preservar progressão e medir contadores por apresentação; não anunciar FPS a partir de Vulkan por software. O APK instalado ainda é 0.10 e não contém os novos controles/contadores.

O CFG completo passou nas fixtures de laço/junção, flags, entrada dinâmica e retorno não local. As duas compilações privadas (Linux/AArch64) passaram; o cabeçalho de contadores também compilou para AArch64 com atomics de 64 bits sem locks. As fontes SVG originais estão em `launcher/artwork/hud/`; o desenho Android usa vetores Canvas correspondentes, sem um interpretador SVG por toque.

No ensaio de 180 s do CFG, os dois pontos que somavam 67120 e 5552 lançamentos não aparecem mais como origem. Os totais declarados dos últimos snapshots foram 371234 na base e 303740 com o CFG, perto da mesma faixa de apresentação. Os snapshots não fixam exatamente o mesmo estado/frame e os contadores são lidos assincronamente; não usar esses números como percentual de ganho de FPS. A redução de trabalho de transferência é uma evidência restrita, não uma solução da renderização.

Passaram **66 testes Python**, incluindo as novas fixtures reais do loop do SDK e de CFG; a suíte JNI/touch também passou. Ainda faltam integração do tiro automático ao consumidor validado, fidelidade visual e medição no tablet. Não foi retirado nenhum desenho, filtro, efeito ou recurso para produzir esse resultado.

O ensaio final **sem LD_PRELOAD/sondas privadas** durou 135,057 s, chegou ao contador de apresentação 1860 e foi encerrado pelo SIGTERM programado. Não registrou pedidos de arquivo de disco ausente nem falha fatal. A captura privada do contador 1800 foi inspecionada: a cena 3D ainda apresenta corrupção/estouro de luminosidade e recortes. Não se aprovou fidelidade visual, missão interativa completa, áudio audível (driver dummy) ou desempenho no Pad 6. O contador de apresentações inclui menus/vídeos; não é FPS de gameplay.

Próximo trabalho de renderização: comparar os mesmos passes/estados do mesmo frame, incluindo todos os Clears e trocas de superfícies; a comparação anterior de cenas diferentes não serve de referência. Manter rastreáveis profundidade/representação, shaders, amostragens de textura e composição. A correção CPU aqui comprovada não fecha essa investigação nem justifica retirar passes ou diminuir qualidade para esconder o problema.
