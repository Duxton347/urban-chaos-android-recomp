# Retorno dos push buffers XDK

## Causa confirmada

A parada anterior em `D3DDevice_RunPushBuffer` (`0x003EF870`) interpretava a última palavra do objeto como um comando original da cena. Um observador privado preservou o buffer que falhava: o valor terminava em bit 0 e apontaria ao endereço 8, não mapeado, exatamente na última palavra do intervalo. Nenhum payload foi publicado ou reutilizado em testes públicos.

A análise do executável legítimo confirmou o contrato do XDK:

- Em `0x003EF894`–`0x003EF8A1`, a rotina calcula `Data + Size - 4`, o endereço do campo reservado para o retorno da submissão.
- Em `0x003EF943`, o descriptor enviado ao helper contém esse endereço e o ponto de continuação da fila do dispositivo. O helper `0x003EF7D0` grava nesse campo a continuação com o bit de salto (`0x003EF7F7`). O valor anterior não é o destino executado pelo hardware.
- No outro caminho, `0x003EFAE0`–`0x003EFAF7`, a rotina copia exatamente `(Size - 4) / 4` palavras para a fila do dispositivo, excluindo o mesmo campo.

Portanto, o salto para 8 não demonstrava um salto interno legítimo entre dados da cena. Era um campo de ligação que a implementação HLE anterior confundia com o payload. A lista completa de comandos anteriores deve ser executada, seguida de retorno ao submitter.

## Implementação

O adaptador `DecodeXdkRecordedIndices` exige ao menos uma palavra e entrega os `Size - 4` bytes de payload ao decodificador genérico. O wrapper valida Size, alinhamento de tamanho, intervalos e limites antes da leitura. O backend recebe os mesmos índices e a mesma ordem de draws do payload; o retorno ao código host representa a continuação da submissão.

A regra pertence ao formato do objeto XDK. Não foi acrescentada uma regra para ignorar saltos no decodificador de métodos NV2A: saltos desconhecidos dentro do payload, comandos não modelados, índices largos fora do limite atual e fixups continuam falhando explicitamente.

O SDK MIT pinado permanece limpo. A adaptação é registrada em `patches/runtime/d3d-device-initialization.patch`; o código próprio do adaptador está em `native/gpu/recorded_push.h`. Nenhuma implementação de hooks da Burner ou de Black foi transplantada.

## Regressões autorais

O teste de pacotes mantém dois draws e verifica todos os seus índices e a ordem com diferentes valores no campo de ligação. Também verifica que:

- O mesmo salto antes do campo reservado ainda falha.
- Um pacote truncado não pode consumir o campo de ligação como parâmetro válido.
- Buffer sem palavra de ligação falha antes de qualquer submissão.
- Os testes existentes de índices 16/32, limite sem truncamento, retorno NV2A, estados de primitiva e comandos desconhecidos permanecem ativos.

```bash
python3 scripts/build_runtime_probes.py
scripts/build_portability.sh
```

Três CTests passaram no Linux; o fixture/adaptador compilou em AArch64. Essas regressões não executam o jogo e não qualificam missão ou fidelidade visual. O teste real posterior, com a imagem privada, é registrado em [RUNTIME_PROGRESS.md](RUNTIME_PROGRESS.md).
