# Investigação do teste 0.4 e candidato privado 0.5

## O que o relatório demonstra

O proprietário relatou novamente ausência de entrada na missão e enviou imagens de legendas em inglês com menus parcialmente traduzidos. O ZIP identifica o build 0.4 `experimental-05bb54fce75d0b35`. Não foi confundido com a sessão 0.3 também incluída no arquivo.

O último frame resumido é 4.620, aos 224,472 s. O intervalo final registra cerca de 4,924 apresentações/s; a captura completa vai até 228,924 s. Não há report de visibilidade nem exceção fatal registrada. Isso não comprova ausência de falhas: confirma que o log ainda não demonstra chegada à renderização 3D.

As leituras continuam aumentando até o último snapshot: 631 leituras, 49.578.256 bytes, nenhuma pendente, máximo de 3.923 µs. O ID por hash da última leitura corresponde a `D:\\12-01.ent`; o offset já chegou a 6.600.704 bytes. O arquivo da missão está sendo consumido lentamente, em vez de haver evidência de uma leitura bloqueada. O processamento Vulkan medido não explica sozinho a duração desse trecho. A causa no código original/integração continua em investigação.

Na captura 0.3 que entrou na missão, o primeiro report de visibilidade apareceu aos 470,186 s, depois do limite da captura 0.4. Essa diferença impede concluir apenas pelo ZIP que uma regressão de deadlock substituiu a lentidão. O relato do proprietário de que a missão não abriu permanece registrado como falha do teste. [Resumo sem textos ou conteúdo de recursos](evidence/runtime-progress/tablet-loading-04.json).

Reprodução da análise:

```sh
python3 tools/analyze_report.py private/reports/current.zip --reference private/reports/previous.zip
```

O analisador limita tamanho/quantidade de arquivos, não extrai o ZIP e só emite códigos, hashes e números. Não imprime mensagens, templates, caminhos do dispositivo ou textos do jogo.

## Falha concreta das legendas

As duas falas das imagens pertencem ao banco `99-01.exttxt` de `99-01.ent`, não ao banco da missão `12-01.exttxt`. O dicionário 0.4 deixava sem tradução `fmv-newsfmv-13` e `fmv-in12-1` a `fmv-in12-18`. Os textos parciais de menus não comprovavam cobertura do vídeo.

O dicionário privado 0.5 preenche essas **19 entradas**, além de mais rótulos do menu General. O gerador agora permite `required_keys` no manifesto e recusa criar um pacote se alguma legenda exigida estiver sem tradução. A CI testa essa recusa usando textos autorais. O dicionário e textos comerciais ficam fora do Git.

Os três bancos gerados por Java foram comparados à tradução Python. IDs, variantes, dados auxiliares e campos de controle foram preservados. Isso corrige a lacuna de conteúdo identificada; **a exibição dessas falas e sua sincronização no tablet ainda precisam de teste**. Não é tradução completa da campanha ou de todos os vídeos.

O relatório atual não contém confirmação `UC-LOCALE-003`. Isso não basta para afirmar que a opção estava desativada, especialmente diante das imagens com texto traduzido. A versão 0.5 registra a seleção na Activity e repete a contagem real de substituições a cada dez segundos, em `UC-LOCALE-005`, evitando depender somente de um evento de startup.

## Cache experimental sem alterar recursos descomprimidos

Foi investigado o custo da descompressão do contêiner da missão, sem substituir scripts, lógica, efeitos ou texturas. `SubtitleArchive.storedResources` valida índice, limites, streams e MD5 e recodifica os recursos em blocos DEFLATE armazenados, com cabeçalho zlib normal. Atualiza offsets/tamanhos/setores/MD5; preserva nomes, IDs, ordem, dados opacos e todos os bytes descomprimidos. Nomes duplicados continuam como entradas distintas.

O contêiner passa de 9.510.928 para 15.597.584 bytes. A comparação dos **12.338 recursos** conferiu seus hashes descomprimidos. Isso troca espaço por menos trabalho do descompressor guest, sem redução deliberada de qualidade. O cache é criado a partir da ISO do usuário em armazenamento privado; não acompanha o APK nem altera a ISO.

A opção **Cache de carregamento experimental** é independente de Português e fica **desativada por padrão**. Pode ser testada com textos originais ou traduzidos. A combinação de tradução e cache também foi executada no host. O cache da missão ocupa aproximadamente 15 MiB adicionais em disco/RAM; combinado com os demais arquivos traduzidos, cerca de 20 MiB. A preparação roda fora da UI. Desativar ambas as opções retorna aos arquivos originais, preservando saves.

## Ensaios Linux reais e seus limites

Mesmo host recompilado otimizado, script de entrada e backend Vulkan de software; áudio SDL dummy, que não comprova som audível. Cada execução terminou no limite programado de 120 s, SIGTERM/-15, sem recurso ausente ou exceção registrada; foram concluídos 16 reports de visibilidade. As capturas do frame 1.800 mostram a abertura 3D original. Não houve validação de missão completa.

| Fonte dos recursos | Frame 1.680 até primeiro report | Primeiro report desde boot |
|---|---:|---:|
| Contêiner original | 8,412 s | 73,194 s |
| Cache armazenado | 7,503 s | 70,069 s |

A diferença do intervalo medido é 0,909 s, cerca de 10,8%, **em um único par de execuções**, sujeito a variação do host. Esse intervalo não é uma medição de todo o loading. Não permite prometer FPS, melhoria no Adreno ou missão corrigida. O backend original continua com defeitos visuais conhecidos. [Recibos dos três ensaios](evidence/runtime-progress/stored-resources-05.json).

## Diagnóstico para o próximo teste Android

`UC-CPU-004` registra até oito candidatos a endereços de código encontrados nos primeiros 256 bytes da pilha atual, com limites da stack da própria thread. A amostragem ocorre a cada 1.024 passagens pela saída de critical section. Valores fora da faixa de código são descartados; nenhum texto ou palavra bruta da pilha é exportado. Não é uma pilha desenrolada nem prova da função causadora. As fixtures testam limites, descarte e cadência.

Esse registro deve localizar os consumidores do trecho lento antes de substituir rotinas originais. Não foi aplicado um retorno falso, timeout que avance a missão ou alteração de ABI para contornar o loading.

## Entrega privada e teste necessário

APK **0.5-experimental**, versionCode 5, build `experimental-737ee5c2bd1835cd`, 28.252.088 bytes. SHA-256 `ecd32919763be60ff96cb097a94a7e849ab97c73b0d203a797ca97c34791c6f8`. Mesma assinatura da versão 0.4; bibliotecas/alinhamento/assinatura e três partes de transporte verificados. [Recibo](evidence/runtime-progress/android-launcher-05.json).

Instalar por cima, sem desinstalar ou reimportar a ISO. Marcar Português e, para experimentar a preparação mais simples, o cache de carregamento. Ativar Legendas dentro de Options → General. Aguardar a abertura original e, se a missão permanecer no loading, exportar o relatório pelo ⋯ depois de aproximadamente um minuto nessa tela. Os registros novos informarão ativação efetiva, progresso das leituras e candidatos de código. Para jogar, o HUD completo pode ser selecionado em ⋯ → Modo de controle.

Validação local: 56 testes Python, 13 testes nativos de fonte/ISO, oito probes de runtime, fixtures Java/Python de legendas/cache, 78 checks de input/viewport, 17 de HUD, 326 de diagnóstico e 14 de cópia/JNI. A Activity/Java e a biblioteca real AArch64 compilaram. Os testes autorais não substituem o teste do jogo no dispositivo.

**Não testado no tablet nesta revisão. A missão ainda não pode ser declarada corrigida ou jogável.**
