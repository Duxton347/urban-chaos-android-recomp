# Áudio: causa rastreada, correção experimental e falha seguinte

**Resultado de 2026-11-16:** a primeira falha foi localizada na integração de objetos DirectSound. Uma implementação experimental de referências passou pelo ponto original e revelou uma falha gráfica posterior. Não há evidência de que o áudio fosse o único impedimento. Nenhum novo arquivo externo é necessário neste momento: os dois recursos pedidos depois estavam na ISO já fornecida e foram extraídos/verificados privadamente.

## Por que falhou

O SDK cria um objeto de dispositivo no host e retorna um endereço guest para ele. O scanner não configurou a função `IDirectSound_AddRef`, export DSOUND `0xB0036`, na função guest `0x00423B41`. Por isso continuou executando o wrapper Xbox original, que procura uma tabela de métodos no objeto de console. O objeto simplificado do host não contém essa tabela.

Instrumentação que encaminha as chamadas sem alterar argumentos mostrou: criação HLE retornou sucesso; o objeto retornado tinha a primeira palavra zero; o wrapper aplicou deslocamento de −8 e leu outra palavra zero; a chamada indireta tentou acessar o endereço host `0x100000004`. Não houve tentativa de abrir recurso de áudio antes desse SIGSEGV. Baixar outro áudio não corrigiria esse defeito de interface.

O tratamento adequado é mapear a API/ABI correta e gerenciar o objeto HLE consistentemente, incluindo referências e descarte. O [patch experimental](../patches/runtime/directsound-device-lifetime.patch) acrescenta aquisição por criação, `AddRef` e `Release` com limpeza final, sob o mutex do SDK. Não substitui lógica original do jogo, não ignora a chamada e não injeta um retorno de sucesso para ultrapassar o crash. Atua no runtime MIT; nenhuma implementação Burner foi incorporada.

O patch publicado foi aplicado com `git apply` a uma cópia privada de `dsound.cpp`, compilado junto com o probe publicado e retestado (probe exit 0; título exit 139 na rotina gráfica); o checkout pinado permanece limpo. O endereço foi vinculado explicitamente após inspeção dessa variante. Não é correção universal de DirectSound nem integração Android aprovada.

## O que foi executado depois

| Teste privado Linux | Resultado |
|---|---|
| Instrumentação sem correção | Reproduz SIGSEGV no wrapper AddRef; objeto/tabela incompatíveis confirmados |
| Referências corrigidas; 79 LST locais | Passa AddRef, pede `D:\AudioData.bin`; timeout controlado de 12 s, exit 124 |
| Mesmo código; `AudioData.bin` extraído | Lê metadados de imagem de efeitos, reconhece nove entradas; pede `Movies/dolby.xmv`; SIGSEGV gráfico |
| Mesmo código; vídeo também disponível | Repete SIGSEGV gráfico em `0x003E7F70`, sem falta de recurso de disco registrada antes dele |
| Probe de ciclo de vida do dispositivo | Exit 0: oito chamadas HLE reais; aquisição, incremento/decremento, criação compartilhada, descarte, recriação e pilha stdcall verificados |

`AudioData.bin` tem 18.608 bytes e `Movies/dolby.xmv` 1.815.040 bytes. Ambos estavam na ISO e seus SHA-256 coincidem com o manifesto público. Foram mantidos privados. Isso não valida todo o conteúdo restante do disco.

O [probe](../host/audio-device/test_device_lifetime.cpp) usa memória sintética e imagem vazia; não executa instruções recompiladas do título. Sua montagem inicial falhou por preparação incompleta do harness e declaração do ponteiro de imagem; o build final corrigido passou. Os logs dessas tentativas permanecem privados. O probe não valida fidelidade de mixing, efeitos DSP, buffers, streams ou som ouvido. O harness descarta a saída de áudio.

## Falha gráfica posterior

A pilha passou a apontar para `0x003E7F70`, chamada por `0x003E5230`, na seção D3D. O dispositivo observado em `0x003F4240` tem seu campo de vertex shader em `+0x794` zerado. A instrução guest `0x003E82A6` lê o objeto por esse ponteiro e acessa a memória inválida.

O próprio SDK possui inicialização condicional desse campo, mas a configuração bruta do scanner não contém `D3D_FvfShader`. É preciso reconstruir e validar a inicialização/espelhamento do estado de shaders e as chamadas que o alteram. A função `SetVertexShader` do host também exige revisão de como mantém o objeto guest esperado pelo código D3D original. Não criar um ponteiro fictício para esconder a leitura inválida. Ainda não foi aplicada uma correção gráfica nem comprovado um frame.

Outros exports DSOUND continuam sem HLE e o kernel registra ordinal 232 (`NtUserIoApcDispatcher`) sem implementação. A execução ainda não demonstrou que todos serão chamados. Cada novo caminho poderá expor novas falhas; menu, missão, áudio, saves e lifecycle continuam sem validação.

## Rota recomendada para o APK

**Windows é opcional.** O C++ gerado do XBE pode ser compilado diretamente para Linux x86_64 ou Android ARM64. Um `.exe` Windows não é entrada da compilação Android. A rota Windows pode servir como comparação com a Burner, mas não precisa bloquear a depuração nem o port.

A sequência atual é: corrigir e testar HLE/ABI/objetos no host Linux da nuvem; completar recursos privados conforme solicitados; obter frames e menu com backend real; levar o código e runtime corrigidos para ARM64/Vulkan/SDL3; então testar no Pad 6. A geração já existe; não é necessário recriar o jogo para mudar de plataforma. Testes de portabilidade ARM64 independentes podem continuar em paralelo às correções, sem declarar boot ou APK funcional.

Não é possível descobrir todas as falhas posteriores a um crash na mesma execução. Devemos combinar execução progressiva, testes isolados de APIs e varredura dos símbolos/imports sem cobertura. O próximo trabalho concreto é a inicialização gráfica e a cobertura dos demais objetos de áudio, usando o XBE e a ISO disponíveis. Se surgir uma entrada realmente ausente, registrar seu caminho exato, consumidor e motivo antes de pedir qualquer busca ao usuário.

[Evidência sanitizada com resultados e limites](evidence/audio-investigation/summary.json). Não foram publicados dados comerciais, código gerado, executáveis derivados ou logs brutos.
