# Gameplay e HUD mobile — 0.8

O proprietário confirma que 0.7 entra na missão mais rapidamente, mas continua lenta e com defeitos. O relatório mais recente identifica `experimental-c6d3b223cf275f48`, atividade de visibilidade aos 79,816 s e cerca de 3,25 apresentações/s no intervalo final. Em 0.6, a primeira atividade registrada foi aos 312,577 s. Esses eventos e amostras não equivalem a um benchmark pareado de carregamento ou à missão jogável. [Resumo sanitizado](evidence/runtime-progress/tablet-mission-07.json).

O relatório final mede 261 ms acumulados em IO, nenhuma leitura pendente e 534/534 APCs entregues. As métricas GPU acumuladas mostram aproximadamente 17,5 s de verificação de espera, 30,8 s de Draw, 16,6 s de readback e 0,5 s de criação de módulos. Há sobreposição entre as operações. O processamento guest continua dominante nas amostras de estado, sem identificar sozinho a função causadora. A capacidade do tablet e a leitura da ISO não explicam esses dados.

## Mira que não perdia movimentos antes de serem lidos

A versão 0.7 enviava valores temporários do analógico direito e os zerava após 90 ms. A cerca de 3 FPS, o guest podia ler somente depois do zeramento. Soltar o dedo também podia apagar movimentos anteriores à leitura.

Agora deltas touch são acumulados no host nativo sob o mesmo mutex da entrada. `XInputGetState` recebe o valor na próxima leitura; em seguida o acumulador é consumido e zera, sem fila de movimentos antigos. Descoberta de conexão e passagem de tempo não drenam o acumulador. Valores são limitados à faixa do analógico, descartando entradas não finitas. Pausa/perda de foco/editor/cancelamento limpam a entrada. Soltar normalmente preserva o último deslocamento ainda não lido.

O caminho completo Java → JNI → host real → Pad foi testado: arrastos curtos, várias atualizações coalescidas, consumo único e limpeza. `UC-INPUT-002` acrescenta `look_events` e `look_delivered`, sem registrar coordenadas. Isso não é uma implementação de mouse look específica do título: mantém o analógico Xbox e depende da sensibilidade, inversão e frequência de leitura do jogo.

## Layout mobile

Referência inspecionada: `vendor/black/native/touch.cpp`, no pin Black fixado pelo projeto. O Black acumula deltas até `TakeMouse`, separa movimento/mira/botões por dedo e organiza ações por contexto; seus hooks e associações do jogo Black não foram transplantados. COD Mobile foi usado como referência conceitual de ergonomia, sem copiar gráficos ou alegar auditoria de seu código.

O layout principal mostra joystick esquerdo, A/B/X/Y à direita, gatilhos LT/RT e pausa. RT recebe um símbolo vetorial original de mira; A/B/X/Y mantêm identificação/colorização. Um botão **+** ao lado do ⋯ abre/recolhe D-pad, cliques dos analógicos, BACK e auxiliares. Os controles ocultos não interceptam toques nem criam regiões mortas. A mira aceita arrasto em qualquer área livre, dos dois lados, com prioridade para botões/joystick já tocados.

Black e White eram botões do controle Xbox original. Saem da tela principal e aparecem como Aux 1/2 no painel extra; a ajuda explica a origem. Suas ações, assim como as de equipe/escudo, continuam vinculadas ao esquema original escolhido no jogo, sem inventar associações específicas. A inspeção dos textos identifica ações, mas ainda não valida cada código no esquema em uso. A/B/X/Y foram mantidos conforme solicitado.

Perfis celular/tablet e editor continuam disponíveis. O layout compacto usa arquivos separados `*-mobile8.json`, preservando os antigos. Não há detecção fictícia de missão ou troca automática de HUD baseada em tempo.

## Custo do código recompilado

A investigação anterior identificou exceções C++ numa rotina matemática separada artificialmente em dois fragmentos. O primeiro cria/alinha a pilha; o fragmento seguinte restaura a pilha e retorna. Como o fragmento guardava o slot de entrada intermediário, um retorno normal podia ser tratado como transferência não local.

`tools/fuse_tail_fragments.py` mantém as instruções, insere o corpo de um fragmento folha no único salto estático final e usa o contrato de retorno do chamador. A entrada dinâmica original continua existente e inalterada. Fragmentos com continuações, calls, kernel calls ou escopos de exceção são excluídos. Nenhuma verificação de destino/slot é desativada. A transformação revisada em `config/research/tail-fusion.json` afeta uma aresta e exige o hash exato da fonte; outros arquivos não foram transformados.

Uma fixture C++ autoral compila as formas separada e unida: prova o desenrolamento desnecessário antes e o retorno normal depois, com mesmo ESP/destino; também verifica que a entrada dinâmica independente conserva a transferência não local. O arquivo estático anterior foi preservado por receita/recibos e seu objeto alterado, e somente a unidade revisada foi substituída. A transição Android exige um recibo que corresponda ao hash anterior, novo hash e arquitetura, além das verificações anteriores de XBE/perfil/biblioteca. Não atribui qualificação de gameplay a um rebuild.

## Verificação e limitações

A primeira execução dos testes Python encontrou a reserva mínima de espaço do importador na nuvem: três erros e uma falha. Arquivos estáticos intermediários regeneráveis e a biblioteca 0.6 substituída foram removidos com hashes/receitas preservados, sem apagar ISOs, saves ou APKs entregues. A reserva do importador não foi reduzida. Os 60 testes Python passaram na reexecução.

O teste do host real confirma o consumo dos deltas; testes de HUD verificam que controles extras ocultos não interceptam a tela. Os testes Java/JNI/diagnóstico e de cópia passaram. As fontes da interface compilam contra Android SDK 34. Esses testes não validam o toque, a imagem ou a velocidade no tablet.

**Fluidez, fidelidade visual e missão plenamente jogável permanecem sem comprovação nesta versão.** A otimização evita trabalho desnecessário, sem reduzir resolução, retirar efeitos, pular desenhos ou substituir lógica original.

## Entrega privada

O rebuild em duas etapas pelo script público reproduziu byte a byte o arquivo estático Linux usado no ensaio. A biblioteca AArch64 foi ligada com o novo JNI de deltas, e o montador exige esse símbolo. O ensaio original limitado durou 180,074 s, apresentou o frame 1.800 e gerou uma captura da abertura 3D inspecionada. Não houve pedido de recurso ausente ou falha fatal registrada; o encerramento foi SIGTERM programado, não conclusão de missão. Houve compilação concorrente: **não é benchmark de FPS**, e a captura não comprova gameplay interativo. [Recibo](evidence/runtime-progress/tail-fusion-08.json).

A primeira montagem Android encontrou uma variável de lambda Java com nome duplicado no método de toque; o compilador recusou o APK, o erro foi corrigido e os logs preservados. A montagem final passou Gradle, ELF/JNI, assinatura, alinhamento e integridade do transporte.

APK **0.8-experimental**, versionCode 8, build `experimental-89c83955a1b1546a`, 28.252.088 bytes. SHA-256 `03d01c07e78e779eafa58f70b634059ca1e7cace7d17e6e883870e374132fa17`. Certificado igual a 0.7; três partes verificadas por concatenação/CRC e hash do APK interno. ISO e recursos originais não são incluídos. APK/código gerado/dicionário permanecem privados. [Recibo](evidence/runtime-progress/android-mobile-08.json).

Instalar por cima sem desinstalar, preservando saves/ISO. Desativar o cache experimental durante a comparação. A área livre da tela inteira aceita a mira; movimento e botões têm prioridade. O botão + abre os controles extras. Exportar pelo ⋯ permite verificar se deltas chegaram e foram consumidos. A versão não foi executada no Pad 6 nesta sessão.
