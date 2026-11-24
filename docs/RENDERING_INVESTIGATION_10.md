# Missão 0.9 — z-buffer lido como cor e montagem de vértices

**Correção parcial de um contrato gráfico demonstrada; missão jogável e fluidez ainda não comprovadas.** O proprietário continua relatando corrupção e travamentos em 0.9. Não há evidência para atribuir todos os defeitos à mesma causa ou anunciar o port como pronto.

## O que o relatório demonstra

O ZIP identifica `experimental-6671d1a3586443d4`, captura de 391,699 s e aproximadamente 4,924 apresentações/s no último intervalo. A thread principal consumiu 83,872% de um núcleo no intervalo final de cerca de 10 s. Isso mede tempo de CPU, não utilização total do tablet ou tempo GPU. As leituras acumulam aproximadamente 286 ms; a leitura mais longa tem 7,524 ms, com nenhuma pendente no snapshot. Não é um benchmark pareado com 0.8.

`two_sided_draws` permaneceu zero, assim como as atualizações de textura dentro de um Present registradas por aquele contador. Portanto, a correção de faces anterior não explica o trecho. Fog também não estava ativo nos passes principais antes observados. [Resumo sanitizado](evidence/runtime-progress/tablet-mission-09.json).

## Falha estrutural confirmada

Um observador privado confirmou, em sucessivos frames da abertura 3D, uma textura linear `LIN_B8G8R8A8` (0x40), 640×480, sobre a mesma memória do z-buffer atualmente vinculado. O z-buffer já havia recebido escritas da GPU e não existia um target de cor para aquele endereço. As modalidades de textura desse desenho eram 513.

O Vulkan procurava somente um target de cor para essa textura. Como não o encontrava, carregava bytes da RAM guest. Escritas no depth attachment Vulkan não atualizam automaticamente essa RAM. Logo, o pass que deveria ler o resultado da GPU recebia armazenamento antigo. Isso é um erro de visibilidade/representação de memória, independente de reduzir resolução ou qualidade.

O patch `vulkan-depth-colour-alias.patch` materializa na GPU os bytes D24S8 que a textura espera: profundidade de 24 bits e stencil de 8 bits, com o mapeamento de canais do formato selecionado. O empacotamento ocorre antes da filtragem da textura; filtrar profundidade e depois separar seus bytes produz outro resultado. Não há readback de depth para a CPU.

Cada escrita GPU em depth invalida a representação derivada, incluindo Clear e desenhos. Reutilizar a mesma profundidade/formato reutiliza a conversão; mudar formato a refaz. Uma escrita CPU marcada ou uma superfície de cor mais recente retoma a autoridade sobre o armazenamento. A rota é limitada a um alias do depth atualmente vinculado, linear D24S8, pitch/dimensões compatíveis e cinco formatos de cor de 32 bits. Não presume que todo endereço de um antigo depth target ainda lhe pertença. Formatos incompatíveis nessa rota identificada são recusados explicitamente.

Uma fixture autoral coloca 0xA5 na RAM de depth e escreve valores conhecidos apenas pela GPU. O renderer anterior falha a comparação. Com o patch, por exemplo, depth `0x123456` e stencil `0xAB`, vistos como formato 0x40, produzem BGRA `18,52,86,171`. O teste confirma cinco formatos, alterações de depth/stencil, reutilização, atualização após desenho, retomada pela CPU e filtragem após empacotar. Não usa shader, textura ou geometria comercial. [Recibos](evidence/runtime-progress/rendering-contracts-10.json).

## Custo da montagem de vértices

O port forçava `XBE_RAW_VERTICES=0`, expandindo atributos para float4 e reconstruindo listas pela CPU, mesmo com suporte de entrada nativa no backend. A amostragem privada Linux encontrou montagem de vértices entre os custos relevantes da thread guest. O ensaio de software não mede o Adreno nem determina a fração exata desse custo no tablet.

O Vulkan passa a usar formatos nativos quando a declaração permite. Cada instância consulta `VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT` do dispositivo; um formato ausente refaz a montagem pela conversão portátil. Declarações desalinhadas continuam nessa conversão. O renderer CPU de referência permanece com vértices expandidos. Não há cache de vértices mutáveis sem invalidação ou retirada de desenhos.

A abertura também apresentou declarações `NORMSHORT3` com stride de 10 bytes. O requisito fixo de múltiplo de quatro herdado do caminho D3D12 continuaria forçando conversão desses buffers. O caminho Vulkan usa o alinhamento natural dos componentes: dois bytes para SHORT/SNORM16, um para componentes de oito bits e quatro para FLOAT32/packed32. Os demais backends mantêm o requisito antigo. Isso permite, por exemplo, SHORT3 com um atributo de quatro bytes no offset 6, sem considerar alinhado um FLOAT4 no offset 1.

O ensaio com apenas alinhamento natural expôs o segundo atributo desse layout real: uma normal `NORMPACKED3` de 32 bits no offset 6. Ela ainda exigia a conversão inteira. Agora somente os quatro bytes desse atributo são copiados, sem alterar seus bits, para um buffer alinhado de stride 4; a posição permanece compactada e o shader faz a decodificação. A cópia segue o mesmo intervalo/ordem de vértices que o desenho, incluindo base de vértice e gather esparso. Não é um cache de dados de outro frame. Layouts/formatos que não atendem ao contrato conservam o fallback.

Dezenove formatos autorais foram desenhados pelos caminhos nativo e convertido no mesmo GPU de software, com comparação de pixels (tolerância de uma unidade de 255). Há casos assinados, packed, FLOAT2H, cores e componentes ausentes. Três layouts adicionais de stride 10, incluindo o NORMPACKED3 no offset 6 observado no título, também comparam seus pixels à conversão portátil. Índices densos e esparsos, com base de vértice, mantêm os atributos associados à posição correta. O estado de endereçamento WRAP declarado pelo jogo é preservado: não se infere CLAMP apenas porque as UVs estão entre zero e um. Um teste de borda verifica isso. Isso não comprova um ganho de FPS no tablet.

## Limites e ensaios que não constituem prova

Quatro desenhos opacos isolados apresentaram cobertura Vulkan/reference próxima: 10385/10388, 1350/1347, 1494/1491 e 7332/7333 amostras. Isso limita a hipótese de uma projeção universalmente errada, mas não valida a cena completa. A primeira sonda só escolheu desenhos sem cobertura e foi descartada como comparação útil.

Uma tentativa posterior de replay de profundidade não espelhava todas as trocas de superfícies e Clears. Suas divergências não foram aceitas como ground truth. A leitura de comandos raw encontrou comandos ignorados, porém não demonstrou descarte dos estados de profundidade suspeitos na amostra. Não se aplicou uma sincronização indiscriminada de todos os registradores para contornar o problema.

O ensaio Linux com o primeiro patch de depth percorreu 300,085 s, apresentou o frame 2100 e exerceu conversões reais. START acionou a pausa de Hope Street. Capturas continuam mostrando problemas; não são uma aprovação visual nem comprovam toda a missão. A simulação gráfica via SwiftShader e a compilação concorrente impedem usar essa sessão como benchmark do Pad 6.

A pesquisa também encontrou correções MIT mais recentes do SDK para precisão de profundidade com far plane distante: `736608115966155115937723bce6b71764e2b6b5` e seus predecessores. Foram examinadas separadamente, sem atualizar o runtime inteiro ou incorporar as adaptações Burner. Uma experiência privada de 210,115 s com profundidade reversa gerou outra captura da abertura, sem demonstrar uma melhora visual conclusiva; shaders de sombra ainda não adaptados são recusados nessa experiência. Ela ficou fora do APK. Uma substituição global de profundidade sem esse contrato validado ampliaria o risco de regressões.

## Build e próxima validação

Os contratos Vulkan de pixels, profundidade, culling, fog, visibilidade e entradas nativas passaram nos caminhos normal e Vulkan 1.1 com fences/render passes. Passaram 61 testes Python. A primeira montagem Android recusou um contexto de patch diferente por causa da instrumentação de assembly Android; o contexto foi ajustado e a retomada ligou a biblioteca AArch64. A construção do patch de alinhamento também recusou uma alteração de pretransformed já aplicada por um patch anterior; a duplicação foi retirada e os contratos passaram. Os erros foram mantidos, sem usar um binário antigo como sucesso.

O próximo relatório precisa mostrar `native_vertex_draws`, `converted_vertex_draws`, `vertex_upload_bytes`, `depth_colour_binds` e `depth_colour_conversions`, além de CPU e tempos GPU. Os contadores verificam a rota efetivamente utilizada; não substituem observar fidelidade visual, comandos, missão e saves no tablet. O HUD e os recursos originais não foram modificados nesta correção.

## APK privado

APK **0.10-experimental**, versionCode 10, build `experimental-315c00c19ca570e2`, 28264376 bytes, SHA-256 `66675198f9b8d2c6e68eb5feefc684f6be93f3ede28c9bdc9038ba4f555f8cd6`. ELF AArch64, JNI, assinatura, alinhamento e transporte em três partes foram verificados; o certificado é o mesmo de 0.9. Instalar por cima, sem desinstalar ou apagar dados. Nenhum APK foi enviado ao repositório público. [Recibo sanitizado](evidence/runtime-progress/android-rendering-10.json).

Intermediários 0.10 não entregues foram substituídos após os testes de autoridade de memória e stride compactado; recibos/logs preservados. Não foram apagados APKs/partes anteriormente entregues ou arquivos do jogo. A entrega não inclui a experiência privada de profundidade reversa, profilers de PC ou observadores de comandos/shaders. Ainda exige avaliação visual e de desempenho no dispositivo.

Os workflows do commit de código `1c62f02` passaram auditoria, testes automatizados, ferramentas nativas e contratos ARM64. A reprodução original Windows continua falhando por ausência de `build/xbe-full-integration-20261014/integration-status.json`; essa falha foi inspecionada e mantida. Não é qualificação do APK nem uma falha escondida por esta correção. [Resultados CI](evidence/runtime-progress/ci-rendering-10.json).

O ensaio final com os streams packed durou 135,119 s e chegou ao contador de apresentação 1860, sem pedido de arquivo ausente ou falha fatal registrada; o encerramento foi SIGTERM programado. O snapshot final tem 244.384 draws nativos, zero desenhos totalmente convertidos, 832.778.182 bytes de upload e 163 conversões/binds de depth como cor. O trabalho de copiar somente atributos desalinhados continua ocorrendo nesse caminho nativo. Esses totais incluem a abertura e não demonstram FPS ou missão interativa aprovada.

A captura final ainda apresenta defeitos. O log também registra roteamento de stream de áudio para buffer não implementado; o driver SDL dummy não valida áudio audível. Essas limitações continuam abertas. Não há exigência de outra ISO nem alegação de que o único impedimento era o arquivo executável.
