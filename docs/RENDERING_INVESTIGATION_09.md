# Missão 0.8 — renderização, desempenho e investigação 0.9

**Estado: missão continua injogável segundo o proprietário. Não foi comprovada uma correção completa dos gráficos ou da velocidade.** A captura enviada mostra defeitos graves; sua tela não foi correlacionada a um frame específico do ZIP. A data/HUD da imagem não provam, isoladamente, o build do frame. O ZIP identifica 0.8 (`experimental-89c83955a1b1546a`).

## Relatório real

Captura de 487,332 s, primeiro evento de visibilidade aos 48,320 s e cerca de 3,951 apresentações/s no último intervalo. Isso não é benchmark pareado com versões anteriores ou confirmação de interação normal na missão. [Resumo sanitizado](evidence/runtime-progress/tablet-mission-08.json).

As 1.198 leituras somam aproximadamente 289 ms, sem leitura pendente no snapshot; 728 APCs foram entregues para 728 enfileiradas. As operações GPU host acumulam cerca de 22,9 s em verificação de espera, 35,5 s em Draw, 11,7 s em montagem de vértices, 6,0 s em texturas, 20,3 s em readback e 0,6 s em módulos. Há sobreposição entre operações; não somar os tempos. Esses valores não medem toda a execução GPU.

Entraram 7.108 eventos de arrasto, consumidos em 845 leituras do acumulador; coalescer eventos é esperado. Isso comprova trânsito de movimentos até o Pad, não câmera correta ou baixa latência visual. A maior latência de botão registrada foi aproximadamente 1,26 s. O relatório não possuía tempo de CPU por thread: amostras de estado guest não permitem concluir, sozinhas, quanto tempo houve de processamento, espera ou falta de escalonamento.

## Duas falhas gráficas reproduzidas

### Fator de neblina de um vertex program

O Vulkan ignorava `oFog` como fator de mistura quando o shader era programável e não havia tabela de fog. O D3D12 do SDK já distinguia esse contrato. O patch genérico `vulkan-program-fog.patch` faz essa distinção, preservando as outras modalidades.

Uma fixture autoral executa três instruções MOV NV2A, desenha vermelho, usa fog azul e fator 0,25. No backend anterior o pixel foi B=0/G=0/R=255; corrigido, B=191/G=0/R=64. O controle negativo falhou pelo motivo esperado; não é um teste textual do shader.

**Não é a causa demonstrada da captura:** os estados da abertura 3D observados nesta sessão tinham fog desligado. A correção não pode ser apresentada como explicação de todos os artefatos.

### Orientação de face independente do culling

O Vulkan inferia a orientação frontal a partir do descarte de polígonos, ignorando o estado `FRONTFACE` quando o descarte estava desligado. O estado Xbox permite controlar essas propriedades de forma independente; a orientação também escolhe as cores frente/verso em iluminação de duas faces.

O patch `vulkan-independent-facing.patch` conserva a orientação declarada e escolhe o bit Vulkan de descarte de frente/verso conforme a direção solicitada. Não desliga culling nem o teste de profundidade.

Na fixture autoral, a frente é vermelha e o verso verde, com culling desligado. No controle negativo, mudar FRONTFACE continuava produzindo vermelho nos dois casos. Corrigido, CW produz vermelho e CCW produz verde. As fixtures também confirmam que descartar CW/CCW continua obedecendo à orientação da geometria em coordenadas com Y para baixo.

`UC-GPU-006` passa a contar `two_sided_draws`, para verificar se essa rota foi usada durante o teste real. A reprodução comprova o defeito do backend; o efeito em cada personagem/cena ainda depende de validação no tablet.

## Inspeção do jogo e limites dos ensaios

Um observador privado registrou apenas enumerações/formatos/tamanhos durante a abertura 3D. Confirmou z-buffer 640×480, depth test/write nos passes principais, comparação LEQUAL e filtros lineares nas texturas principais. Outras passagens usam configurações diferentes legitimamente. Isso não prova que todos os vértices, shaders ou estados estejam corretos.

Foram observadas modalidades de textura 0, 1, 15 e 16 no ensaio limitado, todas presentes no tradutor. Não foi demonstrado uso das modalidades ausentes 8/18 nesse trecho. Ausência na amostra não elimina seu uso posterior. Os observadores privados foram removidos do host antes da montagem final; nunca foram incluídos no APK.

A tentativa inicial de comparar renderers reutilizou um overlay antigo, falhou na compilação por falta de `ReplaceTextArchive` e executou um binário antigo que não inicializou Vulkan. Esses resultados foram mantidos como falhas, não aceitos como comparação. O segundo host de referência compilou com o overlay compatível, mas a mesma sequência de botões percorreu telas diferentes: no frame 1.800, o renderer de referência mostrava salvamento de perfil e o Vulkan já mostrava a abertura 3D. Portanto, não há comparação de imagens da mesma cena nem benchmark entre esses ensaios. Capturas antigas também não foram tratadas como ground truth do Xbox.

As sondas privadas Linux de PC primeiro incluíram threads do renderer de software. Uma segunda sonda usou o relógio de CPU da thread guest e distinguiu módulos nativos; observaram-se montagem de vértices e rotinas de vídeo durante suas fases. Isso não identifica o gargalo do Adreno nem mede o Pad 6. Não se retirou qualidade gráfica com base nesses perfis.

## Informação necessária para a próxima otimização

`UC-CPU-003` agora acrescenta `cpu_valid` e `cpu_ns`, tempo de CPU real da thread desde seu início. Em Linux/Android, o relógio é obtido uma vez ao registrar a thread e consultado na coleta periódica; não há syscall extra por instrução ou draw. Falha na consulta/relógio indisponível emite `cpu_valid=0`, sem inventar zero como ociosidade. Outras plataformas mantêm a coleta desabilitada.

O analisador calcula o percentual de um núcleo entre as duas últimas leituras válidas de cada thread. 100% significa um núcleo ocupado, não a utilização total do tablet. Pausa, falta de escalonamento e outras esperas também reduzem o valor; ele não deve ser interpretado isoladamente como tempo GPU. Junto dos tempos GPU e lifecycle, permite separar o próximo alvo de CPU/espera no dispositivo real.

A build 0.9 é de **investigação**, com duas correções de renderização verificadas, não uma missão aprovada. Ainda falta demonstrar causa e correção do conjunto de artefatos, desempenho interativo, progressão e saves no tablet. O HUD, resolução interna e recursos originais não foram alterados nesta sessão.

## Verificação

Passaram 61 testes Python, incluindo cálculo de CPU com relógio inválido excluído, e os testes Java/JNI/input/importação. A fixture nativa comprova avanço do relógio sob trabalho autoral. Os contratos Vulkan executaram fog, orientação frontal, culling, profundidade, textura alterada e reports nos caminhos timeline/dynamic rendering e fences/render passes, com SwiftShader. Isso não valida o driver Adreno.

Uma montagem do contador de faces falhou por contexto incorreto no patch; a primeira correção ainda usava o nome errado da variável. Os erros foram preservados, o patch foi regenerado sobre o arquivo real e a retomada compilou. Um controle negativo de faces inicialmente recebeu a fonte Android em vez da headless; a falha de include foi preservada e o ensaio correto usou uma cópia headless. Nenhuma dessas tentativas foi contada como sucesso.

## Artefato privado de investigação

APK `0.9-investigacao`, versionCode 9, build `experimental-6671d1a3586443d4`, 28.252.088 bytes; SHA-256 `c2530e0281ddefd53b497834c0bfc1a0cf24f8fa859179655f99876de5e41a43`. Inclui as duas correções e os contadores, com o HUD 0.8 preservado. Biblioteca AArch64 ligada, Gradle, assinatura/alinhamento e três partes de transporte verificados; certificado igual ao 0.8. [Recibo](evidence/runtime-progress/android-investigation-09.json).

Uma montagem anterior 0.9, sem a correção de orientação independente, foi substituída antes da entrega; recibo/logs preservados e APK intermediário regenerável removido. Nenhum APK foi publicado no GitHub. As medições privadas não incluem instalação ou execução no Pad 6. [Contratos, pixels e controles negativos](evidence/runtime-progress/rendering-contracts-09.json).

O teste no dispositivo precisa manter a mesma missão por 30–60 s após entrar e exportar o relatório. Instalar por cima, sem remover dados; manter o cache experimental desativado para comparação. O próximo relatório terá CPU por thread e uso real de iluminação de duas faces, necessários para escolher a próxima intervenção. Esta entrega não declara o jogo jogável.


O ensaio final sem observadores privados durou 120,069 s, apresentou o frame 1.800 e registrou 16 reports (logger limitado), sem pedido de recurso ausente ou falha fatal; terminou por SIGTERM programado. O relógio CPU esteve válido, e `two_sided_draws` permaneceu zero na abertura observada. Essa constatação limita o efeito esperado das correções nesse trecho. Não foi uma missão completada nem benchmark; a fonte do renderer final corresponde exatamente à fonte exercitada pelos contratos.
