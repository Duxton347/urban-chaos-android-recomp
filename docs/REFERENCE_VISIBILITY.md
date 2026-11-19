# Consultas de visibilidade no host de pesquisa

## Diagnóstico posterior ao retorno XDK

A correção de [GPU_LINKAGE.md](GPU_LINKAGE.md) eliminou a parada no trailer do push buffer. Um teste de 180 s passou pelo ponto, mas não apresentou o frame seguinte. A mesma ausência ocorreu sem captura de imagens; portanto, não foi atribuída automaticamente ao custo de escrever TGA.

Amostras nativas da thread que executa o jogo mostraram repetição em `0x003E2C30` e no consumidor `0x000A24C0`. O consumidor repete a consulta enquanto recebe `0x88760828` (resultado ainda pendente). A implementação original lê uma tabela de reports de 16 bytes, testa o status em +12 e entrega a contagem em +8, além de timestamp opcional. Ela não pode retornar sucesso com uma contagem inventada.

O backend headless do SDK contava draws e renderizava apenas frames selecionados, mas não implementava ClearReport/GetReport. Os slots ficavam pendentes. O backend D3D12 já possui relatórios reais; esse suporte não existia no caminho de referência Linux usado na pesquisa.

## Implementação

`patches/runtime/reference-visibility.patch` acrescenta:

- Contagem no rasterizador real, após rejeição por shader/alpha, stencil e profundidade, antes da máscara de escrita de cor. Uma primitiva invisível não produz uma contagem positiva artificial.
- Reset explícito da contagem e entrega no formato de 16 bytes: timestamp lógico de apresentação, contagem e status de conclusão. O status é publicado depois dos dados.
- Capability explícita no backend. D3D12 preserva seu suporte existente; o backend com thread encaminha a capability; o renderer headless só anuncia suporte com renderização completa habilitada.
- Wrapper em `D3DDevice_GetVisibilityTestResult` que processa os comandos pendentes e sincroniza o backend antes de chamar o leitor original, mantendo seus argumentos, slot e resultados.

O caminho sem escrita de cor evita interpolação e shading de pixels somente quando não há alpha test, alpha kill, texture clip ou observação de pixels. A contagem permanece após os mesmos testes de cobertura, stencil e profundidade. O teste compara suas contagens com o caminho de escrita de cor e verifica rejeição por alpha e texture clip.

A sincronização da consulta é bloqueante neste host de diagnóstico. Não constitui uma implementação assíncrona otimizada para produção.

O rasterizador de referência continua limitado às primitivas e shaders que implementa. Uma topologia contada que não possa ser montada como triângulos provoca erro explícito em vez de uma contagem zero falsa. Isso não qualifica toda a fidelidade da GPU Xbox.

## Uso privado

```bash
REF_ALL=1 REF=120 build/title-host/cmake/urban_research private/game
```

`REF_ALL=1` avalia todos os frames pela CPU; REF escolhe somente a frequência das capturas. É uma configuração de diagnóstico, lenta, sem promessa de desempenho de produção.

`REF_FROM_FRAME` permite começar a renderização contínua em uma fronteira de frame escolhida para a investigação. Uma consulta antes dessa fronteira, ou sem um clear de profundidade válido após ativar o modo, falha explicitamente. O modo não altera o código do jogo, sua IA, recursos, input ou progresso. A escolha da fronteira não comprova a apresentação dos frames anteriores e não substitui um renderer completo de produção.

O teste de cena usou comandos originais de START/A pelo harness, sem alterar o estado guest. Logs, capturas e dados do título permanecem privados. Os resultados reais posteriores são registrados em [RUNTIME_PROGRESS.md](RUNTIME_PROGRESS.md).

## Testes autorais executados

Quatro CTests passaram, incluindo a regressão de visibilidade. Ela usa triângulos e superfícies 8×8 criados pelo teste, sem imagem ou metadata do título:

- 15 amostras passaram para o triângulo visível; geometria totalmente ocluída produziu zero.
- Um triângulo mais próximo contribuiu com amostras reais; profundidade igual falhou no teste LESS.
- Rejeição por alpha, desativação da contagem e reset foram verificados.
- A contagem funcionou com escrita de cor desativada.
- Status e contagem foram entregues em 16 bytes, preservando o sentinela após a estrutura.
- O mesmo cenário passou diretamente e pelo backend com thread/flush.
- Topologia contada não suportada foi rejeitada explicitamente.

Os testes anteriores de ocupação do backend, vértices pretransformados e pacotes XDK também passaram. Nenhum resultado desses testes é classificado como missão jogável ou validação Android.

## Resultado no jogo e próxima falha

Execuções privadas de 360 s passaram pelo trailer e completaram reports reais, incluindo contagens 58, 28062, 168, zero e 112659. Não houve arquivo solicitado ausente nem nova exceção registrada; a apresentação permaneceu no frame 1740. Isso não comprova o primeiro frame da missão.

A amostragem posterior encontrou a thread guest em `0x000B9A10`, rotina de inserção/balanceamento de árvore. Em uma observação limitada a 160 s, o contador permaneceu em um e os ponteiros de pai/avô apontaram repetidamente para o mesmo nó. A origem do estado inválido ainda está em investigação. O laço original não foi removido nem substituído.
