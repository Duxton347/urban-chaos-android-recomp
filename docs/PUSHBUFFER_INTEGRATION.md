# Próxima integração: push buffers gravados

**IMPLEMENTAÇÃO PARCIAL / REQUIRES_REVERSE_ENGINEERING.** Após as correções de contexto, DirectSound e conclusão de I/O, a execução da primeira missão alcança D3D `0x003EF870`, chamada por `0x000B4EB0`, e falha no estado de submissão Xbox ausente. O teste terminou com SIGSEGV/exit 139 após aproximadamente 72 s, sem arquivos solicitados ausentes. Não é mais a primeira falha de áudio.

## Evidência e limite atual

O consumidor passa um objeto de push buffer e fixup nulo. A rotina original consulta os campos de endereço/tamanho do buffer, interage com estado de submissão/fences do dispositivo e prepara comandos para a GPU Xbox. A cópia original do dispositivo HLE não contém a infraestrutura de hardware esperada.

O SDK MIT pinado não tem uma implementação de RunPushBuffer. `runtime/xdk/d3d8.cpp::Walk` percorre somente o scratch buffer que ele próprio aloca: trata vértices imediatos e visibilidade e ignora os demais métodos. Ele não é um decodificador completo de command buffers gravados. A rotina original não pode ser substituída por sucesso vazio nem pelo simples avanço do ponteiro de comandos: isso descartaria estados/draws originais.

O título e menu foram observados com draws traduzidos pelo caminho existente. Esse resultado não comprova que geometria, shaders, iluminação e efeitos da missão sejam consumidos corretamente.

## Subconjunto implementado e testado

Um observador que encaminha a chamada sem alterar memória confirmou no primeiro buffer 660 bytes, cinco pacotes, 160 palavras de parâmetros e fixup nulo. Os métodos eram BEGIN_END (`0x17FC`), ARRAY_ELEMENT16 (`0x1800`) e NO_OPERATION (`0x0100`), mais padding vazio. Valores dos parâmetros e o payload não foram publicados ou transformados em fixtures.

`native/gpu/recorded_push.h` implementa esse subconjunto com testes autorais. Cada par little-endian de índices de 16 bits é preservado; ARRAY_ELEMENT32 (`0x1808`) é convertido sem perda quando cada índice cabe em 16 bits, e valores maiores falham explicitamente enquanto o backend não tiver índices largos. As primitivas produzem planos de draws em ordem. O wrapper HLE sincroniza o estado existente e envia os índices reais a `gpu::Backend::Draw`. O parser valida o buffer inteiro antes de emitir qualquer draw. Tamanhos, alinhamento, intervalos mapeados, limites e estados Begin/End são verificados; comandos, subcanais, controle de fluxo e fixups ainda não modelados provocam falha explícita.

O retorno terminal do buffer também é reconhecido; retorno com primitiva aberta ou antes de outras palavras permanece não qualificado. Os testes autorais passaram no Linux e compilaram em AArch64. A primeira execução do wrapper no jogo parou explicitamente em um método não suportado, em vez de tentar acessar o estado de hardware Xbox. Esse resultado é limite do subconjunto, não sucesso do renderer da missão. O teste de diagnóstico confirmou que o primeiro buffer submeteu um draw real; o método seguinte era ARRAY_ELEMENT32. O suporte limitado e sem truncamento foi acrescentado e testado; a execução seguinte submeteu pelo menos oito buffers e encontrou controle de fluxo ainda não qualificado. A execução limpa com retorno terminal implementado repetiu pelo menos oito submissões e parou em um salto do formato antigo (bit 0 do comando), na palavra 165. O destino e sua relação com buffers, chamadas e ponto de retorno ainda precisam ser modelados; esse salto não foi ignorado nem tratado indiscriminadamente como fim.

## Continuação tecnicamente viável

1. Observar privadamente o objeto real, limites, formato dos pacotes, destinos e fixups sem alterar sua execução. Preservar recursos e logs fora do Git. Publicar somente conclusões e testes autorais.
2. Acrescentar ao SDK uma entrada de execução para o buffer original, com validação de intervalos, endereços guest/físicos e limites de comandos. Modelar controle de fluxo CALL/JUMP/RETURN, sem tratar um destino inválido como fim normal.
3. Traduzir os métodos de estado usados pelo jogo para `gpu::State`, recursos, texturas, constantes, shaders, índices e draws do backend existente. Não basta decodificar vértices de Begin/End; buffers gravados podem depender de estado anterior e escrever novos estados para as chamadas seguintes.
4. Implementar fixups e sincronização observados. O primeiro consumidor passa fixup nulo; isso não qualifica buffers com fixup, reutilização, recursos ocupados ou fence pendente.
5. Criar testes autorais de ordenação, estado persistente, draws, limites, fluxo de comandos e erros. Comparar capturas da execução real até a primeira cena da missão, depois validar movimento/câmera/armas/saves.
6. Levar a mesma representação de estado/draws ao backend Vulkan Android. Compilar o guest em ARM64, já feito, não implementa essa camada de GPU.

O código público específico da Burner confirma que há trabalho adicional de desenhos gravados/roteamento, mas suas implementações não foram copiadas. A licença geral das adaptações segue sem esclarecimento. A alternativa atual é desenvolver a integração sobre o SDK MIT com evidência do executável legítimo, sem transplantar hooks de Black.

Não há indicação de pacote externo a solicitar ao usuário para essa falha. Os assets requeridos pelas tentativas realizadas já foram obtidos da imagem fornecida. Recursos de etapas futuras podem ser extraídos progressivamente se forem solicitados pelo próprio jogo.
