# Primeiro teste no Pad 6: desempenho e loading

Em 2026-11-18 o proprietário instalou o build `experimental-3ef6e0e968626f62` e forneceu um relatório exportado. Relatou vídeos lentos, áudio entrecortado e primeira missão presa no loading. O APK inicia no dispositivo, mas **a missão não está aprovada**. A observação Android substitui a ausência de teste da entrega anterior; não altera os resultados históricos do Linux. [Metadados sanitizados](evidence/runtime-progress/tablet-first-report.json).

O relatório confirma importação ativada, kernel iniciado, superfície Android e dispositivo Vulkan Qualcomm, páginas de 4096 bytes e queries precisas habilitadas. Modelo `23043RP34G`, API 34, ARM64. Entre os contadores 1980 e 2400 houve aproximadamente 4,65 apresentações/s. O mixer continuou entregando frames ao SDL, mas seu contador de amostras não nulas deixou de aumentar. Não há erro nativo registrado. Um aviso de recurso ausente não inclui sua identidade; pode ser uma sondagem opcional. Marcadores de sessão sem encerramento limpo não comprovam crash.

## Correção de build e nova instrumentação

O arquivo guest da primeira entrega foi construído pelo alvo de **compile probe**, que impunha `-O0` mesmo com CMake Release. O runtime usava `-O1`. O alvo agora aceita nível explícito 0/1/2; a nova entrega compila todas as 29.049 funções com `-O2`, além do host com `-O2`, sem fast-math, sem remover missões, recursos ou funções. O builder Android exige otimização explícita 1/2 no cache do arquivo guest e a registra no recibo. Nível zero permanece para probes de compilação. Isso corrige uma configuração inadequada para desempenho; não demonstra ganho de FPS no tablet ou correção do loading.

O próximo relatório inclui, a cada dez segundos:

- `UC-CPU-003`: slots estáveis de threads, contador de chamadas, escopo atual e última operação. `kind`: 0 fora de escopo, 1 entrada guest, 2 ordinal de kernel, 3 identificador HLE, 4 operação Vulkan. Não é amostragem de cada instrução e não detecta sozinho deadlock.
- `UC-DISC-002`: leituras concluídas, bytes, leituras curtas, transferências em andamento, tempo acumulado/máximo e blocos em que um stream ativo ficou sem packets. Leitura curta pode ser EOF normal; stream vazio é um sintoma, não uma causa classificada.
- `UC-AUDIO-004`: callbacks, tempo acumulado/máximo no mixer, blocos mais lentos que sua duração sonora e demandas limitadas. Não grava PCM nem faz IO no callback.
- `UC-GPU-005`: tempo host nas APIs de espera, aquisição da imagem e criação/consulta de módulos shader. Não são timestamps de execução da GPU. Operações 0, 1 e 2, respectivamente.

O identificador HLE é FNV-1a de 32 bits do nome público da implementação, por exemplo `hle_D3DDevice_Swap`. Pode ser resolvido contra os nomes do SDK; não contém ponteiros host, nomes de recursos, memória nem conteúdo do disco. Os 64 slots são processuais e nunca reciclados; a saturação é contada explicitamente. Escopos restauram o estado anterior inclusive após exceções/callbacks aninhados. Snapshots são concorrentes, sem atomicidade global.

## Verificações executadas

Passaram 46 testes Python, oito probes de runtime/HLE, 326 checks Java de relatórios, 14 checks de cópia retomável, fixture concorrente de progresso e testes JNI/ZIP. O oracle de contexto com `-O2` preservou o resultado 42 e a pilha em callbacks aninhados; o controle negativo legado continuou falhando como esperado. SDL real com driver dummy passou canais/lifecycle/erros, sem alegação de áudio ouvido. O renderer passou a fixture Vulkan autoral no Linux e os contratos AArch64/headless/superfície Android. O código ARM64 real e a biblioteca completa foram compilados e ligados.

Falhas desta revisão foram preservadas nos logs privados: instrumento HLE inicialmente exigia um símbolo ausente de API desabilitada; foi substituído pelo identificador de nome público. O patch de telemetria Vulkan inicialmente dependia do contexto de includes Android e falhou no overlay Linux; seu contexto foi reduzido e ambos os contratos passaram. A primeira montagem Gradle foi impedida pela restrição de sockets do sandbox; nova execução com acesso autorizado ao socket local passou. Nenhuma qualificação foi fabricada para contornar esses erros.

## Próximo teste necessário

Instalar a versão 0.2 por cima da anterior, com a mesma assinatura e pacote. Não desinstalar ou limpar dados: a ISO ativa e os diretórios privados podem ser reutilizados. Comparar o mesmo vídeo e a mesma tentativa de primeira missão. Se permanecer em loading, deixar por pelo menos 60 segundos antes de voltar e exportar o ZIP. O relatório deve identificar `experimental-16fce2d9b3011686`. Esse teste permitirá localizar leituras pendentes, esperas de kernel/HLE e esgotamento de packets; o relatório anterior não permite escolher com evidência entre essas causas.

Nova biblioteca: 82.095.128 bytes; novo APK: 28.178.216 bytes; SHA-256 `991c26e2c94a17895a146c4ca9f0cccb0e9c11256822fa83ceabad428a2611e9`. Assinatura/alinhamento e três bibliotecas ARM64 verificados. **Versão 0.2 ainda não executada no tablet; loading, desempenho e áudio permanecem a validar.** O binário fica privado, fora do Git e de artefatos públicos.
