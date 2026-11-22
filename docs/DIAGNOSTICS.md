# Relatórios exportáveis do launcher

O coletor é infraestrutura do launcher real. Não executa jogo nem substitui o runtime. Enquanto o ambiente privado está indisponível, a implementação é validada no GitHub Actions com dados de autoria própria; o jogo não foi executado novamente nesta revisão.

## O que é registrado

Cada evento JSONL tem código estável, nível, motivo legível, indicação de causa conhecida ou ainda não classificada, data UTC, tempo monotônico, sequência e identificador de thread do produtor. O manifesto identifica build, pin do SDK e revisão do XBE (ou fonte ainda não selecionada). Identificadores de thread Java e C++ pertencem a espaços distintos.

- Fonte: revisão diferente, PS2, arquivo comprimido, ausência do XBE, integridade, Title ID, entry e falhas de leitura.
- Runtime: boot solicitado/aceito/recusado, avisos/erros, export de kernel sem implementação, destino sem função recompilada, trap guest, recurso não encontrado.
- Gráficos/áudio: apresentações, draws/frames vazios, frames/amostras ativas, pico e valores não finitos do mixer. Essas métricas não demonstram que o áudio foi ouvido ou que uma missão está correta.
- Launcher: eventos tipados de importação, lifecycle, dispositivo, configuração de entrada e resultado de save. Os produtores ainda precisam ser ligados às operações reais do launcher Android; não são eventos inventados.
- Exceção Java: classe e até 16 posições de pilha, sem mensagem ou valores dos argumentos. O handler anterior é preservado.
- Encerramento: marcador de sessão incompleta e, no Android 11+, motivo/status/timestamp/uso de memória do encerramento anterior informado pelo sistema. Sessão sem fechamento pode significar crash, encerramento pelo sistema ou perda de energia; não se declara uma causa sem evidência.

O observer opcional do logger SDK preserva o checkout MIT. Ele funciona também em build release e não modifica o estado guest. As mensagens formatadas não entram no relatório: apenas templates estáticos de código e campos numéricos revisados. Isso evita copiar caminhos, conteúdo do jogo ou argumentos arbitrários. Templates podem ser correlacionados à fonte para investigação; alguns valores omitidos precisarão de instrumentação específica posterior.

## Normal e detalhado

O modo normal registra erros/avisos e resumos selecionados. O detalhado inclui templates das chamadas existentes ao logger. No host, ativar URBAN_LOG_DETAIL=1 antes de iniciar o processo.

Isso não registra cada instrução Xbox, cada sample ou cada draw individual. Chamadas que o próprio SDK limita com LOG_FIRST continuam limitadas. Sessões detalhadas podem ter custo de IO e rotacionar rapidamente: reproduzir a falha, exportar logo depois e retornar ao modo normal. Captura de registradores/memória, dumps, assets, ISO/XBE, saves, mensagens de exceção e logs SDK brutos não são exportados. Não há envio automático.

## Limites e exportação

Java mantém até três sessões, com quatro arquivos de até 256 KiB por sessão. O journal nativo mantém quatro arquivos de até 1 MiB por sessão. Na integração Android, iniciar o journal nativo no filho novo retornado por nativeDirectory(), dentro da sessão Java; o manifesto nativo tem sua própria identidade. O host de pesquisa escreve sessões independentes em logs/reports e não aplica retenção entre execuções; seu armazenamento privado deve ser administrado pelo runner.

ReportSession.exportZip copia somente nomes de metadados autorizados, no máximo três sessões. Rejeita symlinks nos arquivos autorizados e arquivos maiores que o limite. Não percorre game, saves ou outras pastas. Um snapshot durante execução não é globalmente atômico; o estado da sessão e eventuais últimas linhas incompletas devem ser considerados na análise.

AndroidReports fornece ACTION_CREATE_DOCUMENT para salvar ZIP por SAF, cancelamento separado de falha, cópia em executor de fundo e retorno de resultado no executor UI. Primeiro gera um ZIP privado temporário; só depois o copia para o destino. Em falha de escrita, tenta excluir o documento parcial. Não solicita acesso completo ao armazenamento. O botão/entrada de menu será ligado à Activity do launcher real; não se ampliou a Activity diagnóstica histórica para simular o port.

Criar uma única instância de AndroidReports no escopo do aplicativo; chamar lifecycle nas transições reais. A Activity deve usar sua API de resultado para createExportDocument() e encaminhar a URI para exportTo(). NativeReports.attach liga o observer via JNI depois de carregar a biblioteca real, compilando android_bridge.cpp na mesma libmain.so do logger SDK. Não carregar outra cópia do SDK numa biblioteca separada. Passar nativeDirectory() e o build/hash validados; a ponte cria seu filho privado. A Activity e o build do runtime Android ainda precisam chamar essa API. NativeReports.detach exige que as threads guest anteriores já tenham parado. onPause/onStop não são fechamento normal de sessão. closeNormally() só representa término deliberado do runtime.

Após reproduzir um erro no launcher integrado: usar Exportar relatório, salvar o ZIP e enviá-lo com a ação que provocou a falha. Hoje ainda não existe um APK do jogo com esse botão testado; não há arquivo novo para instalar.

## Códigos principais

| Código | Significado |
|---|---|
| UC-SOURCE-001 | Outra revisão requer escolha experimental |
| UC-SOURCE-002 a 010 | Estrutura, XBE ausente, digest, título, entry, PS2, arquivo comprimido, IO e rejeição não classificada |
| UC-CPU-001 / UC-KERNEL-001 | Destino sem função recompilada / export sem implementação |
| UC-CPU-002 | Trap guest; causa exige análise |
| UC-DISC-001 | Recurso solicitado ausente; pode ser probe opcional |
| UC-RUNTIME-001 / 002 | Aviso / erro do runtime; consultar template |
| UC-GPU-001 / UC-AUDIO-001 | Métricas de apresentação / mixer |
| UC-GPU-002 | Report de visibilidade Vulkan concluído após readback; contagem de amostras |
| UC-APP-003 | Sessão anterior sem fechamento confirmado |
| UC-JAVA-001 / 002 | Exceção Java / posição de pilha |
| UC-NATIVE-001 | Encerramento anterior observado pelo Android; não prova crash nativo |
| UC-LOG-001 / 002 / 003 | Falha de escrita / symlink rejeitado / relatório excede limite |
| UC-EXPORT-000 / 002 / CANCELLED | Exportação concluída / falhou / cancelada |

A definição exata está nos enums de journal.h e ReportSession.Event. Campo cause_known=false deve permanecer como incerteza explícita. Nem todo erro tem uma causa determinável automaticamente.

## Validação e próximos testes

scripts/test_diagnostics.py compila o journal com o SDK overlay em release, testa captura, classificação, modo detalhado, rotação, concorrência e saída abrupta em processo separado. Compila e executa os checks Java de exportação, exclusão de payloads/caminhos, pilha sem mensagem, retenção, symlink, arquivo excessivo e falha no destino. O ZIP gerado é aberto e seu JSON/JSONL é validado. Não há dados comerciais nas fixtures.

O teste JNI carrega uma biblioteca de fixture no JVM, emite eventos pelo mesmo logger SDK, grava no journal nativo e valida sua presença no ZIP Java. O workflow Android compila também a ponte JNI para AArch64 e o adaptador Java contra SDK34; isso não executa SAF ou JNI no tablet. O host original com logging, a ISO comercial completa, Vulkan, SDL3, suspensão e exportação em HyperOS aguardam o ambiente privado e a integração Android. Resultados do CI serão registrados na PR. Falha de gravação deixa healthy=false; UC-LOG-001 indica que o coletor não conseguiu persistir eventos, sem presumir falta de espaço como causa.

## Controle de exportação para a Activity

ReportExportController oferece a entrada de menu e o botão Exportar relatório. A Activity real recebe a instância de AndroidReports do aplicativo, chama addTo(menu) ou createButton(), encaminha onActivityResult para handleResult e fecha apenas o controller em onDestroy. Rotação da Activity não fecha a sessão global nem o runtime. Seleção usa SAF; geração/cópia ocorre em executor de fundo, com resultado e código no UI thread. Cancelamento e ausência do seletor são resultados distintos. O componente compilou contra SDK34; não se anexou um launcher fictício ao módulo diagnóstico histórico. A Activity definitiva e o teste manual permanecem pendentes.

UC-AUDIO-002/003 identificam falhas de operação e estado da saída SDL; [contrato e valores](SDL_AUDIO.md). O host com relatórios voltou a ser compilado e executado nesta sessão; consultar BUILD_STATUS.md para os ensaios reais. A indisponibilidade de ambiente relatada acima pertence à revisão anterior.

O parser também reconhece o formato de apresentação do renderer Vulkan de pesquisa. Exporta frame, backend numérico e indicação de headless, sem inventar contagens de draws ou frames vazios. Os reports de visibilidade exportam somente amostras numéricas. Fixtures autorais verificam esses eventos no journal e pelo caminho JNI/ZIP; não são relatórios coletados de um APK no tablet.

## Resultado da exportação

A preparação do ZIP e a entrega ao destino são etapas distintas. UC-EXPORT-005 confirma somente a preparação. UC-EXPORT-000 confirma cópia concluída no destino; UC-EXPORT-002 registra falha com motivo tipado, UC-EXPORT-003 ausência/recusa do seletor e UC-EXPORT-CANCELLED cancelamento. Mensagens arbitrárias do provedor não são copiadas. Eventos conhecidos somente após fechar o ZIP aparecem nos relatórios seguintes, não são inseridos retroativamente no arquivo já entregue.

## Primeiro host Android experimental

UC-GPU-003 guarda vendor/device/API/driver e precisão Vulkan (phase0: suportado; phase1: habilitado). UC-GPU-004 identifica draw contado recusado (op1 geometria; 2 depth target; 3 target; 4 textura; 5 shader; 6 pipeline). Frames Android usam backend2/headless0. UC-ANDROID-001 guarda páginas host; UC-ANDROID-002 recusa páginas diferentes de 4096 no alocador atual. Campos são numéricos, não conteúdo do título. Activities e ZIP estão ligados no APK experimental, ainda sem teste no tablet.
# Telemetria adicional após o primeiro teste no tablet

A versão 0.2 acrescenta `UC-CPU-003` (escopos/chamadas de threads), `UC-DISC-002` (leituras e stream ativo sem packets), `UC-AUDIO-004` (tempo do mixer) e `UC-GPU-005` (tempo host nas APIs Vulkan). [Campos, limites e interpretação](TABLET_LOADING_INVESTIGATION.md). A coleta é periódica; não copia conteúdo do disco ou logs brutos. Esses contadores não qualificam automaticamente gameplay, deadlock ou fidelidade sonora.

A versão 0.3 usa o evento Java existente `UC-INPUT-001` para perfil/modo/enquadramento, e o novo evento nativo `UC-INPUT-002` para tempo de entrega do toque ao poll guest. São códigos distintos: a métrica não inclui toda a latência até o frame visível. [HUD e interpretação](TOUCH_HUD.md).


## Investigação da missão após entrada no tablet (0.4)

`UC-GPU-005` acrescenta operações: 3 draw completo, 4 montagem de geometria (Android), 5 texturas, 6 consulta/criação de pipeline, 7 submissão e 8 readback. 0 espera, 1 aquisição e 2 módulo de shader permanecem. Tempos são host, cumulativos e inclusivos: operações aninhadas se sobrepõem, não somar para estimar tempo total ou FPS. Não são timestamps GPU.

`UC-CPU-003` inclui último ordinal kernel/caller e hash do nome público HLE/caller, filtrando endereços fora da seção de código. `UC-DISC-003` registra APCs enfileiradas/concluídas e última leitura: ID de recurso por hash, offset, quantidade pedida/concluída; não inclui nomes ou conteúdo. São snapshots independentes entre threads; não representam transação atômica da última leitura. Permitem procurar ausência de progresso sem confundi-la com fim de arquivo.

`UC-LOCALE-003`: substituições nativas de texto conferidas e instaladas antes do boot, com contagem de arquivos; não prova legendas visíveis/sincronizadas. `UC-LOCALE-002`: cache rejeitado antes do boot. `UC-LOCALE-001`: preparação Java não concluída, mensagem no launcher e classe da exceção no journal Java. `UC-LOCALE-004`: cache preparado, com duração em milissegundos; não prova exibição de legendas. O exportador não inclui cache, dicionários ou bancos de texto.


## Complementos 0.5

`UC-LOCALE-005` é repetido no journal nativo a cada dez segundos: `requested` informa que um cache opcional foi solicitado; `archives` informa quantos arquivos foram efetivamente substituídos. No journal Java, o mesmo código registra seleção `original` ou `cache_requested`, sem caminhos. A contagem não comprova idioma, exibição ou sincronização das legendas. O evento periódico conserva a confirmação se o startup inicial não estiver na exportação.

`UC-CPU-004` apresenta `pc0`…`pc7`, candidatos filtrados pela faixa de código a partir de uma varredura limitada da stack atual, a cada 1.024 passagens pela saída de critical section. Não exporta palavras brutas e não equivale a stack trace desenrolado. Contadores/candidatos são snapshots independentes. A limitação de faixa/cadência é coberta por fixture autoral.

`tools/analyze_report.py` resume o ZIP exportado sem extração e compara com uma referência opcional. Diferencia ausência de confirmação de idioma de confirmação explícita com zero arquivos; indica quando a captura atual termina antes do primeiro report de visibilidade da referência. Não atribui causas ou declara missão jogável automaticamente.
